#include "soundcloud/platform/window_controller.h"

#include <cctype>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

#include "soundcloud/bridge/i_ui_bridge.h"
#include "webview/webview.h"

namespace soundcloud::platform {
namespace {

std::string to_file_url(const std::filesystem::path& file_path) {
    // Приводим путь к абсолютному виду и к '/'-разделителям, чтобы URL был
    // одинаково корректным на разных платформах.
    const std::string normalized_path = std::filesystem::absolute(file_path).generic_string();

    std::ostringstream encoded_path;
    encoded_path << "file://";

    // Для Windows-путей вида C:/... нужен дополнительный '/' после scheme,
    // чтобы получить валидный URL формата file:///C:/...
    if (!normalized_path.empty() && normalized_path.front() != '/') {
        encoded_path << '/';
    }

    for (const unsigned char character : normalized_path) {
        // Оставляем безопасные для URL символы как есть, а всё остальное
        // кодируем в %HH, чтобы пробелы и спецсимволы не ломали навигацию.
        if (std::isalnum(character) != 0 ||
            character == '/' ||
            character == '-' ||
            character == '_' ||
            character == '.' ||
            character == '~' ||
            character == ':') {
            encoded_path << static_cast<char>(character);
            continue;
        }

        encoded_path << '%'
                     << std::uppercase
                     << std::hex
                     << std::setw(2)
                     << std::setfill('0')
                     << static_cast<int>(character)
                     << std::nouppercase
                     << std::dec;
    }

    return encoded_path.str();
}

std::string build_missing_asset_html(const std::filesystem::path& missing_file) {
    const std::string missing_file_path = missing_file.generic_string();

    // Если UI-asset не найден, показываем локальную fallback-страницу,
    // чтобы ошибка запуска была понятной и не выглядела как пустое окно.
    return
        "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"utf-8\">"
        "<title>UI asset not found</title>"
        "<style>"
        "body{font-family:Segoe UI,sans-serif;background:#f6f1ea;color:#1d1d1d;"
        "display:grid;place-items:center;min-height:100vh;margin:0;padding:24px;}"
        ".card{max-width:640px;padding:32px;border-radius:24px;background:#ffffff;"
        "box-shadow:0 24px 80px rgba(56,32,10,.12);}"
        "h1{margin-top:0;color:#d45500;}"
        "code{display:block;margin-top:16px;padding:12px;border-radius:12px;"
        "background:#f4ede6;overflow-wrap:anywhere;}"
        "</style></head><body><section class=\"card\">"
        "<h1>UI-слой не найден</h1>"
        "<p>Приложение запустило native-shell, но не смогло найти runtime assets.</p>"
        "<code>" +
        missing_file_path +
        "</code></section></body></html>";
}

}  // namespace

void window_controller::run(
    const window_configuration& configuration,
    const bridge::i_ui_bridge& ui_bridge) const {
#ifndef NDEBUG
    constexpr bool is_debug_enabled = true;
#else
    constexpr bool is_debug_enabled = false;
#endif

    webview::webview window(is_debug_enabled, nullptr);
    // Shell-уровень отвечает только за окно и публикацию JS bindings.
    // Все бизнес-решения остаются за bridge.
    window.set_title(configuration.title);
    window.set_size(configuration.width, configuration.height, WEBVIEW_HINT_NONE);

    const std::vector<bridge::ui_binding> bindings = ui_bridge.get_bindings();
    for (const bridge::ui_binding& binding : bindings) {
        // Регистрируем каждую JS-функцию как native callback.
        window.bind(binding.name, binding.handler);
    }

    if (std::filesystem::exists(configuration.entry_file_path)) {
        // Нормальный путь запуска: открываем локальный UI entry point.
        window.navigate(to_file_url(configuration.entry_file_path));
    } else {
        // Если assets отсутствуют, не оставляем пользователя с пустым окном.
        window.set_html(build_missing_asset_html(configuration.entry_file_path));
    }

    window.run();
}

}  // namespace soundcloud::platform
