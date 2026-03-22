#pragma once

#include <string>

#include "soundcloud/bridge/i_ui_bridge.h"
#include "soundcloud/core/use_cases/search_tracks_use_case.h"
#include "soundcloud/core/use_cases/toggle_favorite_use_case.h"

namespace soundcloud::bridge {

/**
 * Единая точка входа для UI-слоя.
 * Инкапсулирует JSON-контракт bridge API и маршрутизацию вызовов к use case-ам.
 */
class app_bridge final : public i_ui_bridge {
public:
    app_bridge(
        core::use_cases::search_tracks_use_case search_tracks_use_case,
        core::use_cases::toggle_favorite_use_case toggle_favorite_use_case);

    /**
     * Возвращает список JS-функций, которые должны появиться в Web UI.
     */
    std::vector<ui_binding> get_bindings() const override;

private:
    std::string build_app_info_response() const;
    std::string build_search_tracks_response(const std::string& request_json) const;
    std::string build_toggle_favorite_response(const std::string& request_json) const;

    core::use_cases::search_tracks_use_case search_tracks_use_case_;
    core::use_cases::toggle_favorite_use_case toggle_favorite_use_case_;
};

}  // namespace soundcloud::bridge
