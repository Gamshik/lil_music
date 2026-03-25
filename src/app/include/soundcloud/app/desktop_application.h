#pragma once

#include "soundcloud/api/soundcloud_api_client.h"
#include "soundcloud/bridge/app_bridge.h"
#include "soundcloud/database/sqlite_library_repository.h"
#include "soundcloud/platform/application_paths.h"
#include "soundcloud/platform/window_controller.h"
#include "soundcloud/player/audio_player_service.h"

namespace soundcloud::app {

/**
 * Composition root приложения.
 * Собирает вместе адаптеры инфраструктуры и прикладные сценарии.
 */
class desktop_application {
public:
    /**
     * Собирает весь graph зависимостей приложения в одном месте.
     * Именно здесь создаётся связка API, player, bridge и shell-уровня.
     */
    desktop_application();

    /**
     * Запускает базовый жизненный цикл приложения.
     */
    int run();

private:
    api::soundcloud_api_client api_client_;
    database::sqlite_library_repository library_repository_;
    player::audio_player_service audio_player_;
    platform::application_paths_resolver application_paths_resolver_;
    platform::window_controller window_controller_;
    bridge::app_bridge bridge_;
};

}  // namespace soundcloud::app
