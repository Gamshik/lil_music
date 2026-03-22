#pragma once

#include "soundcloud/platform/window_configuration.h"

namespace soundcloud::platform {

/**
 * Контроллер главного desktop-окна.
 * Инкапсулирует работу с webview и не знает о прикладных сценариях приложения.
 */
class window_controller {
public:
    window_controller() = default;

    /**
     * Создаёт окно приложения и запускает UI event loop.
     */
    void run(const window_configuration& configuration) const;
};

}  // namespace soundcloud::platform
