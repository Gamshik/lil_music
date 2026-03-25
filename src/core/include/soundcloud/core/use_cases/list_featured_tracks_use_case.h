#pragma once

#include <vector>

#include "soundcloud/core/domain/track.h"
#include "soundcloud/core/ports/i_track_catalog.h"

namespace soundcloud::core::use_cases {

/**
 * Use case загрузки стартовой витрины треков.
 * Нужен для начального экрана, где UI показывает популярные треки без поискового запроса.
 */
class list_featured_tracks_use_case {
public:
    /**
     * Создаёт use case поверх порта каталога, из которого берётся стартовая витрина.
     */
    explicit list_featured_tracks_use_case(const ports::i_track_catalog& track_catalog);

    /**
     * Возвращает подборку популярных публичных треков.
     */
    [[nodiscard]] std::vector<domain::track> execute(int limit) const;

private:
    const ports::i_track_catalog& track_catalog_;
};

}  // namespace soundcloud::core::use_cases
