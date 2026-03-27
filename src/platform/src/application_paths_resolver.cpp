#include "soundcloud/platform/application_paths.h"

#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace soundcloud::platform {
namespace {

std::filesystem::path resolve_executable_path() {
#ifdef _WIN32
    // На Windows путь к исполняемому файлу нужен для поиска соседних runtime assets.
    std::vector<wchar_t> buffer(MAX_PATH);

    while (true) {
        const DWORD copied_characters = GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size()));

        if (copied_characters == 0) {
            throw std::runtime_error("Не удалось определить путь к исполняемому файлу");
        }

        if (copied_characters < buffer.size() - 1U) {
            return std::filesystem::path(std::wstring(buffer.data(), copied_characters));
        }

        buffer.resize(buffer.size() * 2U);
    }
#else
    return std::filesystem::current_path() / "soundcloud_desktop";
#endif
}

std::filesystem::path resolve_local_app_data_directory() {
#ifdef _WIN32
    std::vector<wchar_t> buffer(32767);
    const DWORD copied_characters = GetEnvironmentVariableW(
        L"LOCALAPPDATA",
        buffer.data(),
        static_cast<DWORD>(buffer.size()));
    if (copied_characters == 0 || copied_characters >= buffer.size()) {
        throw std::runtime_error("Не удалось определить директорию LOCALAPPDATA");
    }

    return std::filesystem::path(std::wstring(buffer.data(), copied_characters)) / "LilMusic";
#else
    return std::filesystem::temp_directory_path() / "LilMusic";
#endif
}

}  // namespace

application_paths application_paths_resolver::resolve() const {
    const std::filesystem::path executable_path = resolve_executable_path();
    const std::filesystem::path executable_directory = executable_path.parent_path();
    const std::filesystem::path local_app_data_directory = resolve_local_app_data_directory();

    // UI entry point лежит рядом с exe в подпапке ui.
    // Это позволяет запускать приложение без отдельной установки assets.
    return {
        .executable_directory = executable_directory,
        .ui_entry_file = executable_directory / "ui" / "index.html",
        .local_app_data_directory = local_app_data_directory,
        .settings_database_file = local_app_data_directory / "settings.db",
    };
}

}  // namespace soundcloud::platform
