#include "soundcloud/bridge/app_bridge.h"

#include <exception>
#include <sstream>
#include <utility>

#include "soundcloud/bridge/bridge_json_codec.h"

namespace soundcloud::bridge {
namespace {

std::string to_string(const core::domain::playback_status status) {
    switch (status) {
        case core::domain::playback_status::idle:
            return "idle";
        case core::domain::playback_status::loading:
            return "loading";
        case core::domain::playback_status::playing:
            return "playing";
        case core::domain::playback_status::paused:
            return "paused";
        case core::domain::playback_status::error:
            return "error";
    }

    return "idle";
}

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
                     << R"("artworkUrl":)" << bridge_json_codec::escape_string(track.artwork_url)
                     << ','
                     << R"("streamUrl":)" << bridge_json_codec::escape_string(track.stream_url)
                     << '}';
    return serialized_track.str();
}

std::string serialize_queue_state(
    const core::domain::playback_queue_state& queue_state,
    const std::optional<core::domain::track>& current_track) {
    std::ostringstream response;
    response << R"({"ok":true,"currentTrackId":)"
             << bridge_json_codec::escape_string(queue_state.current_track_id)
             << R"(,"currentTrackTitle":)"
             << bridge_json_codec::escape_string(current_track.has_value() ? current_track->title : "")
             << R"(,"canPlayPrevious":)" << (queue_state.can_play_previous ? "true" : "false")
             << R"(,"canPlayNext":)" << (queue_state.can_play_next ? "true" : "false")
             << R"(,"queuedTracks":[)";

    for (std::size_t index = 0; index < queue_state.queued_tracks.size(); ++index) {
        if (index != 0) {
            response << ',';
        }

        response << serialize_track(queue_state.queued_tracks[index]);
    }

    response << "]}";
    return response.str();
}

}  // namespace

app_bridge::app_bridge(
    core::use_cases::get_playback_state_use_case get_playback_state_use_case,
    core::use_cases::list_featured_tracks_use_case list_featured_tracks_use_case,
    core::use_cases::play_track_use_case play_track_use_case,
    core::use_cases::pause_playback_use_case pause_playback_use_case,
    core::use_cases::resume_playback_use_case resume_playback_use_case,
    core::use_cases::seek_playback_use_case seek_playback_use_case,
    core::use_cases::search_tracks_use_case search_tracks_use_case,
    core::use_cases::toggle_favorite_use_case toggle_favorite_use_case)
    : get_playback_state_use_case_(std::move(get_playback_state_use_case)),
      list_featured_tracks_use_case_(std::move(list_featured_tracks_use_case)),
      play_track_use_case_(std::move(play_track_use_case)),
      pause_playback_use_case_(std::move(pause_playback_use_case)),
      resume_playback_use_case_(std::move(resume_playback_use_case)),
      seek_playback_use_case_(std::move(seek_playback_use_case)),
      search_tracks_use_case_(std::move(search_tracks_use_case)),
      toggle_favorite_use_case_(std::move(toggle_favorite_use_case)) {}

std::vector<ui_binding> app_bridge::get_bindings() const {
    return {
        ui_binding{
            .name = "getAppInfo",
            .handler = [this](const std::string&) { return build_app_info_response(); },
        },
        ui_binding{
            .name = "getPlaybackState",
            .handler = [this](const std::string&) { return build_get_playback_state_response(); },
        },
        ui_binding{
            .name = "getQueueState",
            .handler = [this](const std::string&) { return build_get_queue_state_response(); },
        },
        ui_binding{
            .name = "getFeaturedTracks",
            .handler = [this](const std::string& request_json) {
                return build_get_featured_tracks_response(request_json);
            },
        },
        ui_binding{
            .name = "enqueueTrack",
            .handler = [this](const std::string& request_json) {
                return build_enqueue_track_response(request_json);
            },
        },
        ui_binding{
            .name = "removeQueuedTrack",
            .handler = [this](const std::string& request_json) {
                return build_remove_queued_track_response(request_json);
            },
        },
        ui_binding{
            .name = "playTrack",
            .handler = [this](const std::string& request_json) {
                return build_play_track_response(request_json);
            },
        },
        ui_binding{
            .name = "playPreviousTrack",
            .handler = [this](const std::string&) { return build_play_previous_track_response(); },
        },
        ui_binding{
            .name = "playNextTrack",
            .handler = [this](const std::string&) { return build_play_next_track_response(); },
        },
        ui_binding{
            .name = "pausePlayback",
            .handler = [this](const std::string&) { return build_pause_playback_response(); },
        },
        ui_binding{
            .name = "resumePlayback",
            .handler = [this](const std::string&) { return build_resume_playback_response(); },
        },
        ui_binding{
            .name = "seekPlayback",
            .handler = [this](const std::string& request_json) {
                return build_seek_playback_response(request_json);
            },
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
    return R"({"ok":true,"applicationName":"Lil Music","bridgeStatus":"connected","playbackBackend":"Media Foundation Media Engine"})";
}

std::string app_bridge::build_get_playback_state_response() const {
    try {
        const core::domain::playback_state playback_state = get_playback_state_use_case_.execute();
        const core::domain::playback_queue_state queue_state = playback_session_.get_queue_state();
        const std::optional<core::domain::track> current_track =
            playback_session_.find_known_track(queue_state.current_track_id);

        std::ostringstream response;
        response << R"({"ok":true,"state":)"
                 << bridge_json_codec::escape_string(to_string(playback_state.status))
                 << R"(,"streamUrl":)"
                 << bridge_json_codec::escape_string(playback_state.stream_url)
                 << R"(,"errorMessage":)"
                 << bridge_json_codec::escape_string(playback_state.error_message)
                 << R"(,"positionMs":)" << playback_state.position_ms
                 << R"(,"durationMs":)" << playback_state.duration_ms
                 << R"(,"completionToken":)" << playback_state.completion_token
                 << R"(,"trackId":)" << bridge_json_codec::escape_string(queue_state.current_track_id)
                 << R"(,"trackTitle":)"
                 << bridge_json_codec::escape_string(current_track.has_value() ? current_track->title : "")
                 << R"(,"canPlayPrevious":)" << (queue_state.can_play_previous ? "true" : "false")
                 << R"(,"canPlayNext":)" << (queue_state.can_play_next ? "true" : "false")
                 << '}';
        return response.str();
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_get_queue_state_response() const {
    try {
        const core::domain::playback_queue_state queue_state = playback_session_.get_queue_state();
        const std::optional<core::domain::track> current_track =
            playback_session_.find_known_track(queue_state.current_track_id);
        return serialize_queue_state(queue_state, current_track);
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_get_featured_tracks_response(const std::string& request_json) const {
    try {
        const int limit =
            bridge_json_codec::read_integer_field_from_first_argument(request_json, "limit")
                .value_or(12);
        const std::vector<core::domain::track> tracks = list_featured_tracks_use_case_.execute(limit);
        playback_session_.replace_active_listing(tracks);

        std::ostringstream response;
        response << R"({"ok":true,"title":"Популярное сейчас","limit":)" << limit
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

std::string app_bridge::build_enqueue_track_response(const std::string& request_json) const {
    const std::string track_id = bridge_json_codec::read_string_field_from_first_argument(
                                     request_json,
                                     "trackId")
                                     .value_or("");
    if (track_id.empty()) {
        return build_error_response("Не передан идентификатор трека для очереди.");
    }

    const bool queued = playback_session_.enqueue_track(track_id);
    const core::domain::playback_queue_state queue_state = playback_session_.get_queue_state();
    const std::optional<core::domain::track> current_track =
        playback_session_.find_known_track(queue_state.current_track_id);

    std::ostringstream response;
    response << serialize_queue_state(queue_state, current_track);
    std::string response_json = response.str();
    response_json.pop_back();
    response_json += R"(,"queued":)";
    response_json += queued ? "true" : "false";
    response_json += '}';
    return response_json;
}

std::string app_bridge::build_remove_queued_track_response(const std::string& request_json) const {
    const std::string track_id = bridge_json_codec::read_string_field_from_first_argument(
                                     request_json,
                                     "trackId")
                                     .value_or("");
    if (track_id.empty()) {
        return build_error_response("Не передан идентификатор трека для удаления из очереди.");
    }

    const bool removed = playback_session_.remove_queued_track(track_id);
    const core::domain::playback_queue_state queue_state = playback_session_.get_queue_state();
    const std::optional<core::domain::track> current_track =
        playback_session_.find_known_track(queue_state.current_track_id);

    std::ostringstream response;
    response << serialize_queue_state(queue_state, current_track);
    std::string response_json = response.str();
    response_json.pop_back();
    response_json += R"(,"removed":)";
    response_json += removed ? "true" : "false";
    response_json += '}';
    return response_json;
}

std::string app_bridge::build_play_track_response(const std::string& request_json) const {
    try {
        const std::string track_id = bridge_json_codec::read_string_field_from_first_argument(
                                         request_json,
                                         "trackId")
                                         .value_or("");
        std::string track_title = bridge_json_codec::read_string_field_from_first_argument(
                                            request_json,
                                            "title")
                                            .value_or("Без названия");
        if (const std::optional<core::domain::track> known_track =
                playback_session_.find_known_track(track_id);
            known_track.has_value()) {
            track_title = known_track->title;
        }

        play_track_use_case_.execute(track_id);
        playback_session_.mark_track_started(track_id);

        std::ostringstream response;
        response << R"({"ok":true,"trackTitle":)" << bridge_json_codec::escape_string(track_title)
                 << R"(,"trackId":)" << bridge_json_codec::escape_string(track_id)
                 << R"(,"state":"playing"})";
        return response.str();
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_play_previous_track_response() const {
    try {
        const std::optional<core::domain::track> previous_track = playback_session_.peek_previous_track();
        if (!previous_track.has_value()) {
            return build_error_response("Предыдущий трек недоступен.");
        }

        play_track_use_case_.execute(previous_track->id);
        playback_session_.commit_previous_track_started();

        std::ostringstream response;
        response << R"({"ok":true,"trackId":)"
                 << bridge_json_codec::escape_string(previous_track->id)
                 << R"(,"trackTitle":)"
                 << bridge_json_codec::escape_string(previous_track->title)
                 << R"(,"state":"playing"})";
        return response.str();
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_play_next_track_response() const {
    try {
        const std::optional<core::domain::track> next_track = playback_session_.peek_next_track();
        if (!next_track.has_value()) {
            return build_error_response("Следующий трек недоступен.");
        }

        play_track_use_case_.execute(next_track->id);
        playback_session_.mark_track_started(next_track->id);

        std::ostringstream response;
        response << R"({"ok":true,"trackId":)" << bridge_json_codec::escape_string(next_track->id)
                 << R"(,"trackTitle":)"
                 << bridge_json_codec::escape_string(next_track->title)
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

std::string app_bridge::build_resume_playback_response() const {
    try {
        resume_playback_use_case_.execute();
        const core::domain::playback_queue_state queue_state = playback_session_.get_queue_state();
        const std::optional<core::domain::track> current_track =
            playback_session_.find_known_track(queue_state.current_track_id);
        std::ostringstream response;
        response << R"({"ok":true,"state":"playing","trackTitle":)"
                 << bridge_json_codec::escape_string(current_track.has_value() ? current_track->title : "")
                 << '}';
        return response.str();
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_seek_playback_response(const std::string& request_json) const {
    try {
        const int position_ms =
            bridge_json_codec::read_integer_field_from_first_argument(request_json, "positionMs")
                .value_or(0);
        seek_playback_use_case_.execute(position_ms);

        std::ostringstream response;
        response << R"({"ok":true,"positionMs":)" << position_ms
                 << R"(,"trackId":)"
                 << bridge_json_codec::escape_string(
                        playback_session_.get_queue_state().current_track_id)
                 << '}';
        return response.str();
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_search_tracks_response(const std::string& request_json) const {
    try {
        const core::domain::track_search_request request{
            .query = bridge_json_codec::read_string_field_from_first_argument(request_json, "query")
                         .value_or(""),
            .limit = bridge_json_codec::read_integer_field_from_first_argument(request_json, "limit")
                         .value_or(24),
            .offset =
                bridge_json_codec::read_integer_field_from_first_argument(request_json, "offset")
                    .value_or(0),
        };

        const std::vector<core::domain::track> tracks = search_tracks_use_case_.execute(request);
        playback_session_.replace_active_listing(tracks);

        std::ostringstream response;
        response << R"({"ok":true,"query":)"
                 << bridge_json_codec::escape_string(request.query)
                 << R"(,"limit":)" << request.limit
                 << R"(,"offset":)" << request.offset
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
