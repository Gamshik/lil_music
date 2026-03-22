#pragma once

#include "soundcloud/api/soundcloud_track_playback_reference.h"

namespace soundcloud::api {

/**
 * Преобразует JSON-ответ поиска SoundCloud в доменные модели треков.
 * Вынесен отдельно, чтобы HTTP и mapping не смешивались в одном классе.
 */
class soundcloud_track_search_response_parser {
public:
    soundcloud_track_search_response_parser() = default;

    /**
     * Разбирает payload search/tracks и возвращает доменные треки вместе с playback references.
     */
    parsed_track_search_payload parse(const std::string& payload) const;
};

}  // namespace soundcloud::api
