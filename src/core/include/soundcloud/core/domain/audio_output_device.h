#pragma once

#include <string>

namespace soundcloud::core::domain {

/**
 * Нормализованное описание доступного устройства вывода аудио.
 * UI использует этот snapshot для списка output devices без знания о WASAPI/MMDevice.
 */
struct audio_output_device {
    std::string id;
    std::string display_name;
    bool is_default = false;
    bool is_selected = false;
};

}  // namespace soundcloud::core::domain
