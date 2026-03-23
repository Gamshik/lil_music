#pragma once

#include <vector>

#include "soundcloud/core/domain/track.h"
#include "soundcloud/core/domain/track_search_request.h"
#include "soundcloud/core/ports/i_track_catalog.h"

namespace soundcloud::core::use_cases {

/**
 * Use case поиска треков.
 * Держит прикладное правило в одном месте и не раскрывает наружу API-адаптер.
 */
class search_tracks_use_case {
public:
    explicit search_tracks_use_case(const ports::i_track_catalog& track_catalog);

    /**
     * Выполняет поиск по нормализованным параметрам запроса.
     */
    std::vector<domain::track> execute(const domain::track_search_request& request) const;

private:
    const ports::i_track_catalog& track_catalog_;
};

}  // namespace soundcloud::core::use_cases
