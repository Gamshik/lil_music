#include "soundcloud/core/use_cases/list_featured_tracks_use_case.h"

#include <algorithm>

namespace soundcloud::core::use_cases {
namespace {

int normalize_limit(const int limit) {
    constexpr int default_limit = 12;
    constexpr int min_limit = 1;
    constexpr int max_limit = 30;

    if (limit <= 0) {
        return default_limit;
    }

    return std::clamp(limit, min_limit, max_limit);
}

}  // namespace

list_featured_tracks_use_case::list_featured_tracks_use_case(
    const ports::i_track_catalog& track_catalog)
    : track_catalog_(track_catalog) {}

std::vector<domain::track> list_featured_tracks_use_case::execute(const int limit) const {
    // Верхний слой может передать любой limit, но use case держит границы
    // локально, чтобы API-адаптер всегда получал безопасный диапазон.
    return track_catalog_.get_featured_tracks(normalize_limit(limit));
}

}  // namespace soundcloud::core::use_cases
