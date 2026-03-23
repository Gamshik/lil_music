#include "soundcloud/api/soundcloud_api_client.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace soundcloud::api {
namespace {

int score_transcoding(const track_transcoding_reference& transcoding) {
    // Текущий backend построен на MFPlay.
    // Для него прямой MP3-поток надёжнее HLS-плейлистов, поэтому
    // сначала пробуем progressive audio/mpeg, а уже потом HLS-варианты.
    if (transcoding.protocol == "progressive" && transcoding.mime_type == "audio/mpeg") {
        return 0;
    }

    if (transcoding.protocol == "hls" && transcoding.mime_type == "audio/mpeg") {
        return 1;
    }

    if (transcoding.protocol == "hls" &&
        transcoding.mime_type == "audio/mpegurl" &&
        !transcoding.is_legacy_transcoding) {
        return 2;
    }

    if (transcoding.protocol == "hls" &&
        transcoding.mime_type.find("audio/ogg") != std::string::npos) {
        return 3;
    }

    return 4;
}

}  // namespace

soundcloud_api_client::soundcloud_api_client(soundcloud_api_configuration configuration)
    : http_client_(configuration),
      track_search_response_parser_() {}

std::vector<core::domain::track> soundcloud_api_client::search_tracks(
    const core::domain::track_search_request& request) const {
    const std::string payload = http_client_.fetch_search_tracks_payload(request);
    parsed_track_search_payload parsed_payload = track_search_response_parser_.parse(payload);

    {
        std::scoped_lock lock(playback_reference_mutex_);
        playback_reference_by_track_id_.clear();
        for (track_playback_reference& playback_reference : parsed_payload.playback_references) {
            playback_reference_by_track_id_.insert_or_assign(
                playback_reference.track_id,
                std::move(playback_reference));
        }
    }

    return parsed_payload.tracks;
}

std::string soundcloud_api_client::resolve_stream_url(const std::string& track_id) const {
    track_playback_reference playback_reference = require_track_playback_reference(track_id);

    std::ranges::sort(
        playback_reference.transcodings,
        [](const track_transcoding_reference& left, const track_transcoding_reference& right) {
            return score_transcoding(left) < score_transcoding(right);
        });

    std::string first_error_message;

    for (const track_transcoding_reference& transcoding : playback_reference.transcodings) {
        try {
            const std::string payload = http_client_.fetch_transcoding_payload(
                transcoding.url,
                playback_reference.track_authorization);
            return transcoding_response_parser_.parse(payload);
        } catch (const std::exception& exception) {
            if (first_error_message.empty()) {
                first_error_message = exception.what();
            }
        }
    }

    std::ostringstream message;
    message << "Не удалось разрешить playback URL для трека " << track_id;
    if (!first_error_message.empty()) {
        message << ". Первая ошибка: " << first_error_message;
    }

    throw std::runtime_error(message.str());
}

track_playback_reference soundcloud_api_client::require_track_playback_reference(
    const std::string& track_id) const {
    std::scoped_lock lock(playback_reference_mutex_);
    const auto playback_reference_iterator = playback_reference_by_track_id_.find(track_id);
    if (playback_reference_iterator == playback_reference_by_track_id_.end()) {
        throw std::runtime_error(
            "Playback metadata для трека не найдена. Сначала выполните поиск заново.");
    }

    return playback_reference_iterator->second;
}

}  // namespace soundcloud::api
