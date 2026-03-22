#pragma once

#include <string>
#include <vector>

#include "soundcloud/core/domain/track.h"

namespace soundcloud::api {

/**
 * Преобразует JSON-ответ поиска SoundCloud в доменные модели треков.
 * Вынесен отдельно, чтобы HTTP и mapping не смешивались в одном классе.
 */
class soundcloud_track_search_response_parser {
public:
    explicit soundcloud_track_search_response_parser(std::string client_id);

    /**
     * Разбирает payload search/tracks и возвращает доменные треки.
     */
    std::vector<core::domain::track> parse(const std::string& payload) const;

private:
    std::string client_id_;
};

}  // namespace soundcloud::api
