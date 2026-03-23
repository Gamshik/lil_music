#pragma once

#include <string>

namespace soundcloud::core::domain {

/**
 * Параметры поиска треков.
 * Позволяют UI и bridge явно задавать query и pagination,
 * не протаскивая transport-детали SoundCloud API в core.
 */
struct track_search_request {
    std::string query;
    int limit = 24;
    int offset = 0;
};

}  // namespace soundcloud::core::domain
