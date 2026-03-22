#pragma once

#include <string>
#include <vector>

#include "soundcloud/core/domain/track.h"

namespace soundcloud::api {

/**
 * Ссылка на конкретный transcoding SoundCloud.
 * Нужна для позднего разрешения финального playback URL через transcoding endpoint.
 */
struct track_transcoding_reference {
    std::string url;
    std::string protocol;
    std::string mime_type;
    bool is_legacy_transcoding = false;
};

/**
 * Полный playback reference для найденного трека.
 * Хранит минимальный набор данных, достаточный для получения реального stream URL.
 */
struct track_playback_reference {
    std::string track_id;
    std::string track_authorization;
    std::vector<track_transcoding_reference> transcodings;
};

/**
 * Результат разбора search payload.
 * Позволяет сохранить доменные треки и playback references без повторного парсинга.
 */
struct parsed_track_search_payload {
    std::vector<core::domain::track> tracks;
    std::vector<track_playback_reference> playback_references;
};

}  // namespace soundcloud::api
