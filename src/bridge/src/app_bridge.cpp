#include "soundcloud/bridge/app_bridge.h"

#include <utility>

namespace soundcloud::bridge {

app_bridge::app_bridge(
    core::use_cases::search_tracks_use_case search_tracks_use_case,
    core::use_cases::toggle_favorite_use_case toggle_favorite_use_case)
    : search_tracks_use_case_(std::move(search_tracks_use_case)),
      toggle_favorite_use_case_(std::move(toggle_favorite_use_case)) {}

std::vector<core::domain::track> app_bridge::search_tracks(const std::string& query) const {
    return search_tracks_use_case_.execute(query);
}

bool app_bridge::toggle_favorite(const std::string& track_id) const {
    return toggle_favorite_use_case_.execute(track_id);
}

}  // namespace soundcloud::bridge
