#pragma once

#include <string>

#include "soundcloud/bridge/i_ui_bridge.h"
#include "soundcloud/core/services/playback_session.h"
#include "soundcloud/core/use_cases/get_playback_state_use_case.h"
#include "soundcloud/core/use_cases/list_featured_tracks_use_case.h"
#include "soundcloud/core/use_cases/pause_playback_use_case.h"
#include "soundcloud/core/use_cases/play_track_use_case.h"
#include "soundcloud/core/use_cases/resume_playback_use_case.h"
#include "soundcloud/core/use_cases/search_tracks_use_case.h"
#include "soundcloud/core/use_cases/seek_playback_use_case.h"
#include "soundcloud/core/use_cases/set_playback_volume_use_case.h"
#include "soundcloud/core/use_cases/toggle_favorite_use_case.h"

namespace soundcloud::bridge {

/**
 * Единая точка входа для UI-слоя.
 * Инкапсулирует JSON-контракт bridge API и маршрутизацию вызовов к use case-ам.
 */
class app_bridge final : public i_ui_bridge {
public:
    /**
     * Принимает готовые use case-ы и session-state.
     * Bridge не создаёт инфраструктуру сам, а только маршрутизирует вызовы UI.
     */
    app_bridge(
        core::use_cases::get_playback_state_use_case get_playback_state_use_case,
        core::use_cases::list_featured_tracks_use_case list_featured_tracks_use_case,
        core::use_cases::play_track_use_case play_track_use_case,
        core::use_cases::pause_playback_use_case pause_playback_use_case,
        core::use_cases::resume_playback_use_case resume_playback_use_case,
        core::use_cases::seek_playback_use_case seek_playback_use_case,
        core::use_cases::set_playback_volume_use_case set_playback_volume_use_case,
        core::use_cases::search_tracks_use_case search_tracks_use_case,
        core::use_cases::toggle_favorite_use_case toggle_favorite_use_case);

    /**
     * Возвращает список JS-функций, которые должны появиться в Web UI.
     */
    std::vector<ui_binding> get_bindings() const override;

private:
    std::string build_app_info_response() const;
    std::string build_get_playback_state_response() const;
    std::string build_get_queue_state_response() const;
    std::string build_get_featured_tracks_response(const std::string& request_json) const;
    std::string build_enqueue_track_response(const std::string& request_json) const;
    std::string build_remove_queued_track_response(const std::string& request_json) const;
    std::string build_play_track_response(const std::string& request_json) const;
    std::string build_play_previous_track_response() const;
    std::string build_play_next_track_response() const;
    std::string build_toggle_shuffle_response() const;
    std::string build_pause_playback_response() const;
    std::string build_resume_playback_response() const;
    std::string build_seek_playback_response(const std::string& request_json) const;
    std::string build_set_playback_volume_response(const std::string& request_json) const;
    std::string build_search_tracks_response(const std::string& request_json) const;
    std::string build_toggle_favorite_response(const std::string& request_json) const;

    core::use_cases::get_playback_state_use_case get_playback_state_use_case_;
    core::use_cases::list_featured_tracks_use_case list_featured_tracks_use_case_;
    core::use_cases::play_track_use_case play_track_use_case_;
    core::use_cases::pause_playback_use_case pause_playback_use_case_;
    core::use_cases::resume_playback_use_case resume_playback_use_case_;
    core::use_cases::seek_playback_use_case seek_playback_use_case_;
    core::use_cases::set_playback_volume_use_case set_playback_volume_use_case_;
    core::use_cases::search_tracks_use_case search_tracks_use_case_;
    core::use_cases::toggle_favorite_use_case toggle_favorite_use_case_;
    mutable core::services::playback_session playback_session_;
};

}  // namespace soundcloud::bridge
