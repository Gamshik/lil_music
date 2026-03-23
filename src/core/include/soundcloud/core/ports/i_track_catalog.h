#pragma once

#include <string>
#include <vector>

#include "soundcloud/core/domain/track.h"
#include "soundcloud/core/domain/track_search_request.h"

namespace soundcloud::core::ports {

/**
 * Порт каталога треков.
 * Позволяет core искать публичные треки, не зная о деталях SoundCloud API.
 */
class i_track_catalog {
public:
    virtual ~i_track_catalog() = default;

    /**
     * Выполняет поиск треков по пользовательскому запросу.
     */
    virtual std::vector<domain::track> search_tracks(
        const domain::track_search_request& request) const = 0;
};

}  // namespace soundcloud::core::ports
