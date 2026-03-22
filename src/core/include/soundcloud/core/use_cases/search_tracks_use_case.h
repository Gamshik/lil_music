#pragma once

#include <string>
#include <vector>

#include "soundcloud/core/domain/track.h"
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
     * Выполняет поиск по очищенному пользовательскому запросу.
     */
    std::vector<domain::track> execute(const std::string& query) const;

private:
    const ports::i_track_catalog& track_catalog_;
};

}  // namespace soundcloud::core::use_cases
