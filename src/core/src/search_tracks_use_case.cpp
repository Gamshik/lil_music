#include "soundcloud/core/use_cases/search_tracks_use_case.h"

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

}  // namespace

search_tracks_use_case::search_tracks_use_case(const ports::i_track_catalog& track_catalog)
    : track_catalog_(track_catalog) {}

std::vector<domain::track> search_tracks_use_case::execute(const std::string& query) const {
    const std::string trimmed_query = trim_copy(query);
    if (trimmed_query.empty()) {
        return {};
    }

    return track_catalog_.search_tracks(trimmed_query);
}

}  // namespace soundcloud::core::use_cases
