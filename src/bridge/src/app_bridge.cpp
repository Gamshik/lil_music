#include "soundcloud/bridge/app_bridge.h"

#include <algorithm>
#include <exception>
#include <sstream>
#include <utility>

#include "soundcloud/bridge/bridge_json_codec.h"

namespace soundcloud::bridge {
namespace {

// Преобразует доменный enum в строковый контракт для JS-слоя.
// Bridge не должен отдавать наружу C++-типы, поэтому здесь сразу делаем нормализацию.
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

std::string to_string(const core::domain::playback_repeat_mode repeat_mode) {
    switch (repeat_mode) {
        case core::domain::playback_repeat_mode::none:
            return "none";
        case core::domain::playback_repeat_mode::playlist:
            return "playlist";
        case core::domain::playback_repeat_mode::track:
            return "track";
    }

    return "none";
}

std::string equalizer_status_to_string(const core::domain::equalizer_status status) {
    return std::string(core::domain::to_string(status));
}

std::string equalizer_preset_to_string(const core::domain::equalizer_preset_id preset_id) {
    return std::string(core::domain::to_string(preset_id));
}

std::string build_error_response(const std::string& message) {
    std::ostringstream response;
    response << R"({"ok":false,"message":)" << bridge_json_codec::escape_string(message) << '}';
    return response.str();
}

std::string build_loading_response(const std::string& track_id, const std::string& track_title) {
    // Специальный ответ для быстрых переключений track -> loading.
    // UI понимает, что операция ещё не завершилась, а не пытается интерпретировать это как ошибку.
    std::ostringstream response;
    response << R"({"ok":true,"state":"loading","trackId":)"
             << bridge_json_codec::escape_string(track_id)
             << R"(,"trackTitle":)" << bridge_json_codec::escape_string(track_title)
             << '}';
    return response.str();
}

std::string serialize_track(const core::domain::track& track) {
    // Сериализуем только те поля, которые реально нужны web-слою:
    // id для действий, title/artist/artwork для рендера и streamUrl для отладки/diagnostics.
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

std::string serialize_equalizer_state(const core::domain::equalizer_state& equalizer_state) {
    std::ostringstream response;
    response << R"({"ok":true,"status":)"
             << bridge_json_codec::escape_string(equalizer_status_to_string(equalizer_state.status))
             << R"(,"enabled":)" << (equalizer_state.enabled ? "true" : "false")
             << R"(,"activePresetId":)"
             << bridge_json_codec::escape_string(equalizer_preset_to_string(equalizer_state.active_preset_id))
             << R"(,"outputGainDb":)" << equalizer_state.output_gain_db
             << R"(,"headroomCompensationDb":)" << equalizer_state.headroom_compensation_db
             << R"(,"errorMessage":)"
             << bridge_json_codec::escape_string(equalizer_state.error_message)
             << R"(,"bands":[)";

    for (std::size_t index = 0; index < equalizer_state.bands.size(); ++index) {
        if (index != 0) {
            response << ',';
        }

        response << '{'
                 << R"("index":)" << index << ','
                 << R"("centerFrequencyHz":)" << equalizer_state.bands[index].center_frequency_hz
                 << ','
                 << R"("gainDb":)" << equalizer_state.bands[index].gain_db
                 << '}';
    }

    response << R"(],"presets":[)";
    for (std::size_t index = 0; index < equalizer_state.available_presets.size(); ++index) {
        if (index != 0) {
            response << ',';
        }

        const core::domain::equalizer_preset& preset = equalizer_state.available_presets[index];
        response << '{'
                 << R"("id":)" << bridge_json_codec::escape_string(equalizer_preset_to_string(preset.id)) << ','
                 << R"("title":)" << bridge_json_codec::escape_string(preset.title) << ','
                 << R"("isDerived":)" << (preset.id == core::domain::equalizer_preset_id::custom ? "true" : "false")
                 << R"(,"gainsDb":[)";

        for (std::size_t gain_index = 0; gain_index < preset.gains_db.size(); ++gain_index) {
            if (gain_index != 0) {
                response << ',';
            }
            response << preset.gains_db[gain_index];
        }
        response << "]}";
    }

    response << "]}";
    return response.str();
}

std::string serialize_audio_output_devices(
    const std::vector<core::domain::audio_output_device>& devices) {
    std::ostringstream response;
    response << R"({"ok":true,"devices":[)";

    for (std::size_t index = 0; index < devices.size(); ++index) {
        if (index != 0) {
            response << ',';
        }

        const core::domain::audio_output_device& device = devices[index];
        response << '{'
                 << R"("id":)" << bridge_json_codec::escape_string(device.id) << ','
                 << R"("displayName":)" << bridge_json_codec::escape_string(device.display_name)
                 << R"(,"isDefault":)" << (device.is_default ? "true" : "false")
                 << R"(,"isSelected":)" << (device.is_selected ? "true" : "false")
                 << '}';
    }

    response << "]}";
    return response.str();
}

std::string serialize_queue_state(
    const core::domain::playback_queue_state& queue_state,
    const std::optional<core::domain::track>& current_track) {
    // Возвращаем UI уже готовый снимок очереди.
    // Это защищает web-слой от знания внутренних деталей playback_session.
    std::ostringstream response;
    response << R"({"ok":true,"currentTrackId":)"
             << bridge_json_codec::escape_string(queue_state.current_track_id)
             << R"(,"currentTrackTitle":)"
             << bridge_json_codec::escape_string(current_track.has_value() ? current_track->title : "")
             << R"(,"shuffleEnabled":)" << (queue_state.shuffle_enabled ? "true" : "false")
             << R"(,"repeatMode":)" << bridge_json_codec::escape_string(to_string(queue_state.repeat_mode))
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
    core::use_cases::get_equalizer_state_use_case get_equalizer_state_use_case,
    core::use_cases::get_playback_state_use_case get_playback_state_use_case,
    core::use_cases::list_audio_output_devices_use_case list_audio_output_devices_use_case,
    core::use_cases::list_featured_tracks_use_case list_featured_tracks_use_case,
    core::use_cases::play_track_use_case play_track_use_case,
    core::use_cases::pause_playback_use_case pause_playback_use_case,
    core::use_cases::resume_playback_use_case resume_playback_use_case,
    core::use_cases::seek_playback_use_case seek_playback_use_case,
    core::use_cases::select_audio_output_device_use_case select_audio_output_device_use_case,
    core::use_cases::set_playback_volume_use_case set_playback_volume_use_case,
    core::use_cases::set_equalizer_enabled_use_case set_equalizer_enabled_use_case,
    core::use_cases::select_equalizer_preset_use_case select_equalizer_preset_use_case,
    core::use_cases::set_equalizer_band_gain_use_case set_equalizer_band_gain_use_case,
    core::use_cases::set_equalizer_output_gain_use_case set_equalizer_output_gain_use_case,
    core::use_cases::reset_equalizer_use_case reset_equalizer_use_case,
    core::use_cases::search_tracks_use_case search_tracks_use_case,
    core::use_cases::toggle_favorite_use_case toggle_favorite_use_case)
    : get_equalizer_state_use_case_(std::move(get_equalizer_state_use_case)),
      get_playback_state_use_case_(std::move(get_playback_state_use_case)),
      list_audio_output_devices_use_case_(std::move(list_audio_output_devices_use_case)),
      list_featured_tracks_use_case_(std::move(list_featured_tracks_use_case)),
      play_track_use_case_(std::move(play_track_use_case)),
      pause_playback_use_case_(std::move(pause_playback_use_case)),
      resume_playback_use_case_(std::move(resume_playback_use_case)),
      seek_playback_use_case_(std::move(seek_playback_use_case)),
      select_audio_output_device_use_case_(std::move(select_audio_output_device_use_case)),
      set_playback_volume_use_case_(std::move(set_playback_volume_use_case)),
      set_equalizer_enabled_use_case_(std::move(set_equalizer_enabled_use_case)),
      select_equalizer_preset_use_case_(std::move(select_equalizer_preset_use_case)),
      set_equalizer_band_gain_use_case_(std::move(set_equalizer_band_gain_use_case)),
      set_equalizer_output_gain_use_case_(std::move(set_equalizer_output_gain_use_case)),
      reset_equalizer_use_case_(std::move(reset_equalizer_use_case)),
      search_tracks_use_case_(std::move(search_tracks_use_case)),
      toggle_favorite_use_case_(std::move(toggle_favorite_use_case)) {}

std::vector<ui_binding> app_bridge::get_bindings() const {
    // Этот список и есть публичный JS API приложения.
    // Platform слой только публикует его в webview, но не знает ничего о core use case-ах.
    return {
        ui_binding{
            .name = "getAppInfo",
            .handler = [this](const std::string&) { return build_app_info_response(); },
        },
        ui_binding{
            .name = "getEqualizerState",
            .handler = [this](const std::string&) { return build_get_equalizer_state_response(); },
        },
        ui_binding{
            .name = "getPlaybackState",
            .handler = [this](const std::string&) { return build_get_playback_state_response(); },
        },
        ui_binding{
            .name = "getAudioOutputDevices",
            .handler = [this](const std::string&) { return build_get_audio_output_devices_response(); },
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
            .name = "playCompletionFollowUp",
            .handler = [this](const std::string&) { return build_play_completion_follow_up_response(); },
        },
        ui_binding{
            .name = "toggleShuffle",
            .handler = [this](const std::string&) { return build_toggle_shuffle_response(); },
        },
        ui_binding{
            .name = "cycleRepeatMode",
            .handler = [this](const std::string&) { return build_cycle_repeat_mode_response(); },
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
            .name = "selectAudioOutputDevice",
            .handler = [this](const std::string& request_json) {
                return build_select_audio_output_device_response(request_json);
            },
        },
        ui_binding{
            .name = "setPlaybackVolume",
            .handler = [this](const std::string& request_json) {
                return build_set_playback_volume_response(request_json);
            },
        },
        ui_binding{
            .name = "setEqualizerEnabled",
            .handler = [this](const std::string& request_json) {
                return build_set_equalizer_enabled_response(request_json);
            },
        },
        ui_binding{
            .name = "selectEqualizerPreset",
            .handler = [this](const std::string& request_json) {
                return build_select_equalizer_preset_response(request_json);
            },
        },
        ui_binding{
            .name = "setEqualizerBandGain",
            .handler = [this](const std::string& request_json) {
                return build_set_equalizer_band_gain_response(request_json);
            },
        },
        ui_binding{
            .name = "setEqualizerOutputGain",
            .handler = [this](const std::string& request_json) {
                return build_set_equalizer_output_gain_response(request_json);
            },
        },
        ui_binding{
            .name = "resetEqualizer",
            .handler = [this](const std::string&) { return build_reset_equalizer_response(); },
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
    // Лёгкий handshake для UI: подтверждаем, что bridge жив и какой playback backend активен.
    return R"({"ok":true,"applicationName":"LilMusic","bridgeStatus":"connected","playbackBackend":"WASAPI + Media Foundation Source Reader"})";
}

std::string app_bridge::build_get_equalizer_state_response() const {
    try {
        return serialize_equalizer_state(get_equalizer_state_use_case_.execute());
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_get_playback_state_response() const {
    try {
        // Собираем один snapshot из двух источников:
        // native playback state и session-state очереди/истории.
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
                 << R"(,"volumePercent":)" << playback_state.volume_percent
                 << R"(,"completionToken":)" << playback_state.completion_token
                 << R"(,"trackId":)" << bridge_json_codec::escape_string(queue_state.current_track_id)
                 << R"(,"trackTitle":)"
                 << bridge_json_codec::escape_string(current_track.has_value() ? current_track->title : "")
                 << R"(,"shuffleEnabled":)" << (queue_state.shuffle_enabled ? "true" : "false")
                 << R"(,"repeatMode":)" << bridge_json_codec::escape_string(to_string(queue_state.repeat_mode))
                 << R"(,"canPlayPrevious":)" << (queue_state.can_play_previous ? "true" : "false")
                 << R"(,"canPlayNext":)" << (queue_state.can_play_next ? "true" : "false")
                 << '}';
        return response.str();
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_get_audio_output_devices_response() const {
    try {
        return serialize_audio_output_devices(list_audio_output_devices_use_case_.execute());
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_get_queue_state_response() const {
    try {
        // Отдельный endpoint для UI, который хочет перерисовать только очередь.
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
        // Главная лента обновляет active listing в playback_session,
        // чтобы next/prev потом опирались на тот же набор треков.
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
    // Очередь работает только по trackId: UI уже видит нужный трек,
    // а business rules добавления в queue живут в core playback_session.
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
    // Удаление из очереди тоже идёт через session-state, чтобы web-слой не держал own source of truth.
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
        // Запуск трека идёт через единый orchestration-flow:
        // UI передаёт id, bridge подтягивает title и вызывает use case.
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

        if (get_playback_state_use_case_.execute().status == core::domain::playback_status::loading) {
            return build_loading_response(track_id, track_title);
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
        // Prev зависит не от текущей вкладки, а от native playback history.
        // Это позволяет переключать треки даже при уходе с экрана поиска.
        const std::optional<core::domain::track> previous_track = playback_session_.peek_previous_track();
        if (!previous_track.has_value()) {
            return build_error_response("Предыдущий трек недоступен.");
        }

        if (get_playback_state_use_case_.execute().status == core::domain::playback_status::loading) {
            return build_loading_response(previous_track->id, previous_track->title);
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
        // Next сначала берёт трек из queue, а если queue пустая — идёт по playback-list.
        const std::optional<core::domain::track> next_track = playback_session_.peek_next_track();
        if (!next_track.has_value()) {
            return build_error_response("Следующий трек недоступен.");
        }

        if (get_playback_state_use_case_.execute().status == core::domain::playback_status::loading) {
            return build_loading_response(next_track->id, next_track->title);
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

std::string app_bridge::build_play_completion_follow_up_response() const {
    try {
        const std::optional<core::domain::track> follow_up_track =
            playback_session_.peek_completion_track();
        if (!follow_up_track.has_value()) {
            return R"({"ok":true,"advanced":false})";
        }

        if (get_playback_state_use_case_.execute().status == core::domain::playback_status::loading) {
            return build_loading_response(follow_up_track->id, follow_up_track->title);
        }

        play_track_use_case_.execute(follow_up_track->id);
        playback_session_.mark_track_started(follow_up_track->id);

        std::ostringstream response;
        response << R"({"ok":true,"advanced":true,"trackId":)"
                 << bridge_json_codec::escape_string(follow_up_track->id)
                 << R"(,"trackTitle":)"
                 << bridge_json_codec::escape_string(follow_up_track->title)
                 << R"(,"state":"playing"})";
        return response.str();
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_toggle_shuffle_response() const {
    try {
        const bool shuffle_enabled = playback_session_.toggle_shuffle();
        const core::domain::playback_queue_state queue_state = playback_session_.get_queue_state();
        const std::optional<core::domain::track> current_track =
            playback_session_.find_known_track(queue_state.current_track_id);
        if (shuffle_enabled != queue_state.shuffle_enabled) {
            return build_error_response("Не удалось синхронизировать состояние shuffle.");
        }

        return serialize_queue_state(queue_state, current_track);
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_cycle_repeat_mode_response() const {
    try {
        const core::domain::playback_repeat_mode repeat_mode = playback_session_.cycle_repeat_mode();
        const core::domain::playback_queue_state queue_state = playback_session_.get_queue_state();
        const std::optional<core::domain::track> current_track =
            playback_session_.find_known_track(queue_state.current_track_id);
        if (repeat_mode != queue_state.repeat_mode) {
            return build_error_response("Не удалось синхронизировать состояние repeat.");
        }

        return serialize_queue_state(queue_state, current_track);
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_pause_playback_response() const {
    try {
        // Пауза напрямую управляет player, не меняя queue/history state.
        pause_playback_use_case_.execute();
        return R"({"ok":true,"state":"paused"})";
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_resume_playback_response() const {
    try {
        // Resume возвращает UI ещё и текущий title, чтобы экран не терял контекст трека.
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
        // Seek работает по позиции в миллисекундах, а UI сам вычисляет target из progress bar.
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

std::string app_bridge::build_select_audio_output_device_response(
    const std::string& request_json) const {
    try {
        const std::string device_id = bridge_json_codec::read_string_field_from_first_argument(
                                          request_json,
                                          "deviceId")
                                          .value_or("");
        select_audio_output_device_use_case_.execute(device_id);
        return serialize_audio_output_devices(list_audio_output_devices_use_case_.execute());
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_set_playback_volume_response(const std::string& request_json) const {
    try {
        const int volume_percent =
            bridge_json_codec::read_integer_field_from_first_argument(request_json, "volumePercent")
                .value_or(100);
        set_playback_volume_use_case_.execute(volume_percent);

        std::ostringstream response;
        response << R"({"ok":true,"volumePercent":)" << (std::clamp)(volume_percent, 0, 100)
                 << '}';
        return response.str();
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_set_equalizer_enabled_response(const std::string& request_json) const {
    try {
        const bool enabled =
            bridge_json_codec::read_boolean_field_from_first_argument(request_json, "enabled")
                .value_or(false);
        return serialize_equalizer_state(set_equalizer_enabled_use_case_.execute(enabled));
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_select_equalizer_preset_response(const std::string& request_json) const {
    try {
        const std::string preset_id = bridge_json_codec::read_string_field_from_first_argument(
                                          request_json,
                                          "presetId")
                                          .value_or("");
        const auto parsed_preset_id = core::domain::equalizer_preset_id_from_string(preset_id);
        if (!parsed_preset_id.has_value()) {
            return build_error_response("Неизвестный EQ preset.");
        }

        return serialize_equalizer_state(select_equalizer_preset_use_case_.execute(*parsed_preset_id));
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_set_equalizer_band_gain_response(const std::string& request_json) const {
    try {
        const int band_index =
            bridge_json_codec::read_integer_field_from_first_argument(request_json, "bandIndex")
                .value_or(-1);
        const float gain_db =
            bridge_json_codec::read_float_field_from_first_argument(request_json, "gainDb")
                .value_or(0.0F);
        if (band_index < 0) {
            return build_error_response("Не передан индекс EQ-полосы.");
        }

        return serialize_equalizer_state(
            set_equalizer_band_gain_use_case_.execute(
                static_cast<std::size_t>(band_index),
                gain_db));
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_reset_equalizer_response() const {
    try {
        return serialize_equalizer_state(reset_equalizer_use_case_.execute());
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_set_equalizer_output_gain_response(const std::string& request_json) const {
    try {
        const float output_gain_db =
            bridge_json_codec::read_float_field_from_first_argument(request_json, "outputGainDb")
                .value_or(0.0F);
        return serialize_equalizer_state(
            set_equalizer_output_gain_use_case_.execute(output_gain_db));
    } catch (const std::exception& exception) {
        return build_error_response(exception.what());
    }
}

std::string app_bridge::build_search_tracks_response(const std::string& request_json) const {
    try {
        // Search flow живёт в core/api, а bridge только нормализует входные параметры
        // и отдаёт UI уже готовый список треков.
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
    // Favorites - локальная функция приложения, не связанная с аккаунтом SoundCloud.
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
