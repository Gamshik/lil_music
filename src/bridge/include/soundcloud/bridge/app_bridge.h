#pragma once

#include <string>
#include <vector>

#include "soundcloud/core/domain/track.h"
#include "soundcloud/core/use_cases/search_tracks_use_case.h"
#include "soundcloud/core/use_cases/toggle_favorite_use_case.h"

namespace soundcloud::bridge {

/**
 * Единая точка входа для UI-слоя.
 * Позже сюда будут привязаны webview callbacks и сериализация в JSON.
 */
class app_bridge {
public:
    app_bridge(
        core::use_cases::search_tracks_use_case search_tracks_use_case,
        core::use_cases::toggle_favorite_use_case toggle_favorite_use_case);

    /**
     * Обрабатывает поисковый запрос из UI.
     */
    std::vector<core::domain::track> search_tracks(const std::string& query) const;

    /**
     * Переключает локальное избранное для указанного трека.
     */
    bool toggle_favorite(const std::string& track_id) const;

private:
    core::use_cases::search_tracks_use_case search_tracks_use_case_;
    core::use_cases::toggle_favorite_use_case toggle_favorite_use_case_;
};

}  // namespace soundcloud::bridge
