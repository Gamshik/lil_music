#include "soundcloud/api/soundcloud_api_client.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace soundcloud::api {
namespace {

bool is_supported_by_media_foundation_backend(const track_transcoding_reference& transcoding) {
    // Текущий backend построен на MFPlay и стабильно воспроизводит только
    // прямой progressive MP3. HLS-плейлисты для части треков приводят к
    // HRESULT 0xc00d36c4, поэтому отбрасываем их заранее.
    return transcoding.protocol == "progressive" && transcoding.mime_type == "audio/mpeg";
}

}  // namespace

soundcloud_api_client::soundcloud_api_client(soundcloud_api_configuration configuration)
    : http_client_(configuration),
      track_search_response_parser_() {}

std::vector<core::domain::track> soundcloud_api_client::search_tracks(
    const core::domain::track_search_request& request) const {
    return parse_and_cache_tracks(http_client_.fetch_search_tracks_payload(request));
}

std::vector<core::domain::track> soundcloud_api_client::get_featured_tracks(const int limit) const {
    return parse_and_cache_tracks(http_client_.fetch_featured_tracks_payload(limit));
}

std::string soundcloud_api_client::resolve_stream_url(const std::string& track_id) const {
    track_playback_reference playback_reference = require_track_playback_reference(track_id);
    std::vector<track_transcoding_reference> supported_transcodings;
    supported_transcodings.reserve(playback_reference.transcodings.size());

    for (const track_transcoding_reference& transcoding : playback_reference.transcodings) {
        if (is_supported_by_media_foundation_backend(transcoding)) {
            supported_transcodings.push_back(transcoding);
        }
    }

    if (supported_transcodings.empty()) {
        throw std::runtime_error(
            "Для этого трека SoundCloud не отдал совместимый MP3-поток. "
            "Текущий backend пока не умеет воспроизводить его HLS-варианты.");
    }

    std::string first_error_message;

    for (const track_transcoding_reference& transcoding : supported_transcodings) {
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

std::vector<core::domain::track> soundcloud_api_client::parse_and_cache_tracks(
    const std::string& payload) const {
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

}  // namespace soundcloud::api
