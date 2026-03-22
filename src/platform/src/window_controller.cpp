#include "soundcloud/platform/window_controller.h"

#include <cctype>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

#include "webview/webview.h"

namespace soundcloud::platform {
namespace {

std::string to_file_url(const std::filesystem::path& file_path) {
    const std::string normalized_path = std::filesystem::absolute(file_path).generic_string();

    std::ostringstream encoded_path;
    encoded_path << "file://";

    if (!normalized_path.empty() && normalized_path.front() != '/') {
        encoded_path << '/';
    }

    for (const unsigned char character : normalized_path) {
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

void window_controller::run(const window_configuration& configuration) const {
#ifndef NDEBUG
    constexpr bool is_debug_enabled = true;
#else
    constexpr bool is_debug_enabled = false;
#endif

    webview::webview window(is_debug_enabled, nullptr);
    window.set_title(configuration.title);
    window.set_size(configuration.width, configuration.height, WEBVIEW_HINT_NONE);

    if (std::filesystem::exists(configuration.entry_file_path)) {
        window.navigate(to_file_url(configuration.entry_file_path));
    } else {
        window.set_html(build_missing_asset_html(configuration.entry_file_path));
    }

    window.run();
}

}  // namespace soundcloud::platform
