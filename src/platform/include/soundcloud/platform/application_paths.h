#pragma once

#include <filesystem>

namespace soundcloud::platform {

/**
 * Набор runtime-путей приложения.
 * Используется как единый источник путей, зависящих от расположения исполняемого файла.
 */
struct application_paths {
    std::filesystem::path executable_directory;
    std::filesystem::path ui_entry_file;
    std::filesystem::path local_app_data_directory;
    std::filesystem::path settings_database_file;
};

/**
 * Разрешает runtime-пути приложения.
 * Отделён от window_controller, чтобы управление окном не смешивалось с поиском файлов.
 */
class application_paths_resolver {
public:
    /**
     * Возвращает директорию исполняемого файла и основной UI-asset.
     */
    application_paths resolve() const;
};

}  // namespace soundcloud::platform
