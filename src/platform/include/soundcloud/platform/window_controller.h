#pragma once

#include "soundcloud/platform/window_configuration.h"

namespace soundcloud::bridge {
class i_ui_bridge;
}

namespace soundcloud::platform {

/**
 * Контроллер главного desktop-окна.
 * Инкапсулирует работу с webview и не содержит бизнес-правил.
 */
class window_controller {
public:
    window_controller() = default;

    /**
     * Создаёт окно приложения и запускает UI event loop.
     */
    void run(const window_configuration& configuration, const bridge::i_ui_bridge& ui_bridge) const;
};

}  // namespace soundcloud::platform
