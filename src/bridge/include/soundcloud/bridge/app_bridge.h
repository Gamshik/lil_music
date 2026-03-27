#pragma once

#include <string>

#include "soundcloud/bridge/i_ui_bridge.h"
#include "soundcloud/core/services/playback_session.h"
#include "soundcloud/core/use_cases/get_equalizer_state_use_case.h"
#include "soundcloud/core/use_cases/get_playback_state_use_case.h"
#include "soundcloud/core/use_cases/list_featured_tracks_use_case.h"
#include "soundcloud/core/use_cases/pause_playback_use_case.h"
#include "soundcloud/core/use_cases/play_track_use_case.h"
#include "soundcloud/core/use_cases/reset_equalizer_use_case.h"
#include "soundcloud/core/use_cases/resume_playback_use_case.h"
#include "soundcloud/core/use_cases/search_tracks_use_case.h"
#include "soundcloud/core/use_cases/select_equalizer_preset_use_case.h"
#include "soundcloud/core/use_cases/seek_playback_use_case.h"
#include "soundcloud/core/use_cases/set_equalizer_band_gain_use_case.h"
#include "soundcloud/core/use_cases/set_equalizer_enabled_use_case.h"
#include "soundcloud/core/use_cases/set_equalizer_output_gain_use_case.h"
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
        core::use_cases::get_equalizer_state_use_case get_equalizer_state_use_case,
        core::use_cases::get_playback_state_use_case get_playback_state_use_case,
        core::use_cases::list_featured_tracks_use_case list_featured_tracks_use_case,
        core::use_cases::play_track_use_case play_track_use_case,
        core::use_cases::pause_playback_use_case pause_playback_use_case,
        core::use_cases::resume_playback_use_case resume_playback_use_case,
        core::use_cases::seek_playback_use_case seek_playback_use_case,
        core::use_cases::set_playback_volume_use_case set_playback_volume_use_case,
        core::use_cases::set_equalizer_enabled_use_case set_equalizer_enabled_use_case,
        core::use_cases::select_equalizer_preset_use_case select_equalizer_preset_use_case,
        core::use_cases::set_equalizer_band_gain_use_case set_equalizer_band_gain_use_case,
        core::use_cases::set_equalizer_output_gain_use_case set_equalizer_output_gain_use_case,
        core::use_cases::reset_equalizer_use_case reset_equalizer_use_case,
        core::use_cases::search_tracks_use_case search_tracks_use_case,
        core::use_cases::toggle_favorite_use_case toggle_favorite_use_case);

    /**
     * Возвращает список JS-функций, которые должны появиться в Web UI.
     */
    std::vector<ui_binding> get_bindings() const override;

private:
    // Ниже идут builder-ы конкретных JSON-ответов для публичного JS API.
    std::string build_app_info_response() const;
    std::string build_get_equalizer_state_response() const;
    std::string build_get_playback_state_response() const;
    std::string build_get_queue_state_response() const;
    std::string build_get_featured_tracks_response(const std::string& request_json) const;
    std::string build_enqueue_track_response(const std::string& request_json) const;
    std::string build_remove_queued_track_response(const std::string& request_json) const;
    std::string build_play_track_response(const std::string& request_json) const;
    std::string build_play_previous_track_response() const;
    std::string build_play_next_track_response() const;
    std::string build_play_completion_follow_up_response() const;
    std::string build_toggle_shuffle_response() const;
    std::string build_cycle_repeat_mode_response() const;
    std::string build_pause_playback_response() const;
    std::string build_resume_playback_response() const;
    std::string build_seek_playback_response(const std::string& request_json) const;
    std::string build_set_playback_volume_response(const std::string& request_json) const;
    std::string build_set_equalizer_enabled_response(const std::string& request_json) const;
    std::string build_select_equalizer_preset_response(const std::string& request_json) const;
    std::string build_set_equalizer_band_gain_response(const std::string& request_json) const;
    std::string build_set_equalizer_output_gain_response(const std::string& request_json) const;
    std::string build_reset_equalizer_response() const;
    std::string build_search_tracks_response(const std::string& request_json) const;
    std::string build_toggle_favorite_response(const std::string& request_json) const;

    // Read-only EQ snapshot из player.
    core::use_cases::get_equalizer_state_use_case get_equalizer_state_use_case_;
    // Read-only playback snapshot из player.
    core::use_cases::get_playback_state_use_case get_playback_state_use_case_;
    // Стартовая витрина популярных треков.
    core::use_cases::list_featured_tracks_use_case list_featured_tracks_use_case_;
    // Запуск нового трека.
    core::use_cases::play_track_use_case play_track_use_case_;
    // Transport pause.
    core::use_cases::pause_playback_use_case pause_playback_use_case_;
    // Transport resume.
    core::use_cases::resume_playback_use_case resume_playback_use_case_;
    // Transport seek.
    core::use_cases::seek_playback_use_case seek_playback_use_case_;
    // Playback volume.
    core::use_cases::set_playback_volume_use_case set_playback_volume_use_case_;
    // EQ bypass toggle.
    core::use_cases::set_equalizer_enabled_use_case set_equalizer_enabled_use_case_;
    // Built-in EQ preset selection.
    core::use_cases::select_equalizer_preset_use_case select_equalizer_preset_use_case_;
    // Manual EQ band control.
    core::use_cases::set_equalizer_band_gain_use_case set_equalizer_band_gain_use_case_;
    // EQ output level control.
    core::use_cases::set_equalizer_output_gain_use_case set_equalizer_output_gain_use_case_;
    // EQ reset to Flat.
    core::use_cases::reset_equalizer_use_case reset_equalizer_use_case_;
    // Track search flow.
    core::use_cases::search_tracks_use_case search_tracks_use_case_;
    // Local library/favorites toggle.
    core::use_cases::toggle_favorite_use_case toggle_favorite_use_case_;
    // Native session-state очереди, history, shuffle и repeat.
    mutable core::services::playback_session playback_session_;
};

}  // namespace soundcloud::bridge
