#pragma once

#include <filesystem>
#include <string>

namespace soundcloud::platform {

/**
 * Конфигурация главного desktop-окна.
 * Хранит только параметры shell-уровня и не содержит бизнес-логики.
 */
struct window_configuration {
    std::string title;
    int width = 1280;
    int height = 820;
    std::filesystem::path entry_file_path;
};

}  // namespace soundcloud::platform
