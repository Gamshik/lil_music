#include "soundcloud/bridge/app_bridge.h"

#include <exception>
#include <sstream>
#include <utility>

#include "soundcloud/bridge/bridge_json_codec.h"

namespace soundcloud::bridge {
namespace {

std::string build_error_response(const std::string& message) {
    std::ostringstream response;
    response << R"({"ok":false,"message":)" << bridge_json_codec::escape_string(message) << '}';
    return response.str();
}

std::string serialize_track(const core::domain::track& track) {
    std::ostringstream serialized_track;
    serialized_track << '{'
                     << R"("id":)" << bridge_json_codec::escape_string(track.id) << ','
                     << R"("title":)" << bridge_json_codec::escape_string(track.title) << ','
                     << R"("artistName":)" << bridge_json_codec::escape_string(track.artist_name)
                     << ','
                     << R"("streamUrl":)" << bridge_json_codec::escape_string(track.stream_url)
                     << '}';
    return serialized_track.str();
}

}  // namespace

app_bridge::app_bridge(
    core::use_cases::play_track_use_case play_track_use_case,
    core::use_cases::pause_playback_use_case pause_playback_use_case,
    core::use_cases::search_tracks_use_case search_tracks_use_case,
    core::use_cases::toggle_favorite_use_case toggle_favorite_use_case)
    : play_track_use_case_(std::move(play_track_use_case)),
      pause_playback_use_case_(std::move(pause_playback_use_case)),
      search_tracks_use_case_(std::move(search_tracks_use_case)),
      toggle_favorite_use_case_(std::move(toggle_favorite_use_case)) {}

std::vector<ui_binding> app_bridge::get_bindings() const {
    return {
        ui_binding{
            .name = "getAppInfo",
            .handler = [this](const std::string&) { return build_app_info_response(); },
        },
        ui_binding{
            .name = "playTrack",
            .handler = [this](const std::string& request_json) {
                return build_play_track_response(request_json);
            },
        },
        ui_binding{
            .name = "pausePlayback",
            .handler = [this](const std::string&) { return build_pause_playback_response(); },
        },
        ui_binding{
            .name = "searchTracks",
            .handler = [this](const std::string& request_json) {
                return build_search_tracks_response(request_json);
            },
        },
        ui_binding{
            .name = "toggleFavorite",
            .handler = [this](const std::string& request_json) {
                return build_toggle_favorite_response(request_json);
            },
        },
    };
}

std::string app_bridge::build_app_info_response() const {
    return R"({"ok":true,"applicationName":"Lil Music","bridgeStatus":"connected","playbackBackend":"Media Foundation"})";
}

std::string app_bridge::build_play_track_response(const std::string& request_json) const {
    try {
        const std::string stream_url = bridge_json_codec::read_string_field_from_first_argument(
                                           request_json,
                                           "streamUrl")
                                           .value_or("");
        const std::string track_title = bridge_json_codec::read_string_field_from_first_argument(
                                            request_json,
                                            "title")
                                            .value_or("Без названия");

        play_track_use_case_.execute(stream_url);

        std::ostringstream response;
        response << R"({"ok":true,"trackTitle":)" << bridge_json_codec::escape_string(track_title)
                 << R"(,"state":"playing"})";
        return response.str();
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_pause_playback_response() const {
    try {
        pause_playback_use_case_.execute();
        return R"({"ok":true,"state":"paused"})";
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_search_tracks_response(const std::string& request_json) const {
    try {
        const std::string query = bridge_json_codec::read_string_field_from_first_argument(
                                      request_json,
                                      "query")
                                      .value_or("");

        const std::vector<core::domain::track> tracks = search_tracks_use_case_.execute(query);

        std::ostringstream response;
        response << R"({"ok":true,"query":)" << bridge_json_codec::escape_string(query)
                 << R"(,"tracks":[)";

        for (std::size_t index = 0; index < tracks.size(); ++index) {
            if (index != 0) {
                response << ',';
            }

            response << serialize_track(tracks[index]);
        }

        response << "]}";
        return response.str();
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_toggle_favorite_response(const std::string& request_json) const {
    const std::string track_id = bridge_json_codec::read_string_field_from_first_argument(
                                     request_json,
                                     "trackId")
                                     .value_or("");

    if (track_id.empty()) {
        return build_error_response("Не передан идентификатор трека для избранного.");
    }

    const bool is_favorite = toggle_favorite_use_case_.execute(track_id);

    std::ostringstream response;
    response << R"({"ok":true,"trackId":)" << bridge_json_codec::escape_string(track_id)
             << R"(,"isFavorite":)" << (is_favorite ? "true" : "false")
             << '}';
    return response.str();
}

}  // namespace soundcloud::bridge
