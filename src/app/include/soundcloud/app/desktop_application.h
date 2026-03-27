#pragma once

#include "soundcloud/api/soundcloud_api_client.h"
#include "soundcloud/bridge/app_bridge.h"
#include "soundcloud/database/sqlite_equalizer_settings_repository.h"
#include "soundcloud/database/sqlite_library_repository.h"
#include "soundcloud/platform/application_paths.h"
#include "soundcloud/player/audio_player_service.h"
#include "soundcloud/platform/window_controller.h"

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
    platform::application_paths application_paths_;
    api::soundcloud_api_client api_client_;
    database::sqlite_equalizer_settings_repository equalizer_settings_repository_;
    database::sqlite_library_repository library_repository_;
    player::audio_player_service audio_player_;
    platform::window_controller window_controller_;
    bridge::app_bridge bridge_;
};

}  // namespace soundcloud::app
