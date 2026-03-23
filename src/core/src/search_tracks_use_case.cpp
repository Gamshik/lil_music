#include "soundcloud/core/use_cases/search_tracks_use_case.h"

#include <algorithm>
#include <cctype>

namespace soundcloud::core::use_cases {
namespace {

std::string trim_copy(const std::string& value) {
    std::size_t first = 0;
    while (first < value.size() &&
           std::isspace(static_cast<unsigned char>(value[first])) != 0) {
        ++first;
    }

    std::size_t last = value.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(value[last - 1])) != 0) {
        --last;
    }

    return value.substr(first, last - first);
}

int clamp_limit(const int limit) {
    constexpr int default_limit = 24;
    constexpr int min_limit = 1;
    constexpr int max_limit = 50;

    if (limit <= 0) {
        return default_limit;
    }

    return std::clamp(limit, min_limit, max_limit);
}

int normalize_offset(const int offset) {
    return (std::max)(offset, 0);
}

}  // namespace

search_tracks_use_case::search_tracks_use_case(const ports::i_track_catalog& track_catalog)
    : track_catalog_(track_catalog) {}

std::vector<domain::track> search_tracks_use_case::execute(
    const domain::track_search_request& request) const {
    domain::track_search_request normalized_request{
        .query = trim_copy(request.query),
        .limit = clamp_limit(request.limit),
        .offset = normalize_offset(request.offset),
    };

    const std::string& trimmed_query = normalized_request.query;
    if (trimmed_query.empty()) {
        return {};
    }

    return track_catalog_.search_tracks(normalized_request);
}

}  // namespace soundcloud::core::use_cases
