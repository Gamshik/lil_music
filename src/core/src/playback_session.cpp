#include "soundcloud/core/services/playback_session.h"

#include <algorithm>

namespace soundcloud::core::services {

void playback_session::replace_active_listing(std::vector<domain::track> tracks) {
    // Активный список отражает последнюю витрину или результат поиска.
    // Он нужен, чтобы next/prev и lookup могли опираться на один и тот же набор треков.
    std::scoped_lock lock(state_mutex_);
    active_listing_ = std::move(tracks);
}

bool playback_session::enqueue_track(const std::string& track_id) {
    std::scoped_lock lock(state_mutex_);
    if (track_id.empty()) {
        return false;
    }

    // Не дублируем трек в очереди, даже если пользователь нажал кнопку несколько раз.
    const auto queued_track_iterator = std::ranges::find(
        queued_tracks_,
        track_id,
        &domain::track::id);
    if (queued_track_iterator != queued_tracks_.end()) {
        return false;
    }

    const std::optional<domain::track> track = find_known_track_locked(track_id);
    if (!track.has_value()) {
        return false;
    }

    queued_tracks_.push_back(*track);
    return true;
}

bool playback_session::remove_queued_track(const std::string& track_id) {
    std::scoped_lock lock(state_mutex_);
    const std::size_t previous_size = queued_tracks_.size();
    remove_from_queue_locked(track_id);
    return queued_tracks_.size() != previous_size;
}

void playback_session::mark_track_started(const std::string& track_id) {
    std::scoped_lock lock(state_mutex_);
    const std::optional<domain::track> track = find_known_track_locked(track_id);
    if (!track.has_value()) {
        return;
    }

    // Как только трек стартовал, фиксируем его как текущий playback listing.
    // Это делает next/prev стабильными даже если пользователь ушёл на другой экран.
    update_playback_listing_locked(track_id);
    remove_from_queue_locked(track_id);

    if (!playback_history_.empty() && playback_history_.back().id == track_id) {
        return;
    }

    playback_history_.push_back(*track);
}

std::optional<domain::track> playback_session::peek_next_track() const {
    std::scoped_lock lock(state_mutex_);

    // Очередь всегда имеет приоритет над автоматическим переходом по списку.
    if (!queued_tracks_.empty()) {
        return queued_tracks_.front();
    }

    if (playback_history_.empty()) {
        return std::nullopt;
    }

    const std::string& current_track_id = playback_history_.back().id;
    const auto current_track_iterator =
        std::ranges::find(playback_listing_, current_track_id, &domain::track::id);
    if (current_track_iterator == playback_listing_.end()) {
        return std::nullopt;
    }

    const auto next_track_iterator = std::next(current_track_iterator);
    if (next_track_iterator == playback_listing_.end()) {
        return std::nullopt;
    }

    return *next_track_iterator;
}

std::optional<domain::track> playback_session::peek_previous_track() const {
    std::scoped_lock lock(state_mutex_);
    if (playback_history_.size() < 2) {
        return std::nullopt;
    }

    // Предыдущий трек берём из истории, а не из текущего экрана.
    // Это позволяет корректно возвращаться назад даже после смены вкладки.
    return playback_history_[playback_history_.size() - 2];
}

void playback_session::commit_previous_track_started() {
    std::scoped_lock lock(state_mutex_);
    if (playback_history_.size() < 2) {
        return;
    }

    playback_history_.pop_back();
}

std::optional<domain::track> playback_session::find_known_track(const std::string& track_id) const {
    std::scoped_lock lock(state_mutex_);
    return find_known_track_locked(track_id);
}

domain::playback_queue_state playback_session::get_queue_state() const {
    std::scoped_lock lock(state_mutex_);
    // UI получает только компактный снимок состояния, а не все внутренние списки.
    return domain::playback_queue_state{
        .current_track_id = playback_history_.empty() ? std::string{} : playback_history_.back().id,
        .queued_tracks = queued_tracks_,
        .can_play_previous = playback_history_.size() >= 2,
        .can_play_next = has_next_track_locked(),
    };
}

std::optional<domain::track> playback_session::find_known_track_locked(
    const std::string& track_id) const {
    if (track_id.empty()) {
        return std::nullopt;
    }

    auto find_track = [&track_id](const std::vector<domain::track>& tracks) -> std::optional<domain::track> {
        // Ищем трек по всем локальным спискам в порядке их "свежести".
        const auto iterator = std::ranges::find(tracks, track_id, &domain::track::id);
        if (iterator == tracks.end()) {
            return std::nullopt;
        }

        return *iterator;
    };

    if (const std::optional<domain::track> track = find_track(active_listing_); track.has_value()) {
        return track;
    }

    if (const std::optional<domain::track> track = find_track(playback_listing_); track.has_value()) {
        return track;
    }

    if (const std::optional<domain::track> track = find_track(queued_tracks_); track.has_value()) {
        return track;
    }

    if (const std::optional<domain::track> track = find_track(playback_history_); track.has_value()) {
        return track;
    }

    return std::nullopt;
}

bool playback_session::has_next_track_locked() const {
    if (!queued_tracks_.empty()) {
        return true;
    }

    if (playback_history_.empty()) {
        return false;
    }

    const std::string& current_track_id = playback_history_.back().id;
    const auto current_track_iterator =
        std::ranges::find(playback_listing_, current_track_id, &domain::track::id);
    if (current_track_iterator == playback_listing_.end()) {
        return false;
    }

    return std::next(current_track_iterator) != playback_listing_.end();
}

void playback_session::remove_from_queue_locked(const std::string& track_id) {
    // Удаление делаем через erase_if, потому что queue хранится как обычный список треков,
    // а не как отдельная структура с индексами.
    std::erase_if(queued_tracks_, [&track_id](const domain::track& track) {
        return track.id == track_id;
    });
}

void playback_session::update_playback_listing_locked(const std::string& track_id) {
    if (track_id.empty()) {
        return;
    }

    // Playback listing обновляем только если трек действительно пришёл из active listing.
    // Это защищает navigation flow от чужих/устаревших идентификаторов.
    const auto active_track_iterator = std::ranges::find(
        active_listing_,
        track_id,
        &domain::track::id);
    if (active_track_iterator != active_listing_.end()) {
        playback_listing_ = active_listing_;
    }
}

}  // namespace soundcloud::core::services
