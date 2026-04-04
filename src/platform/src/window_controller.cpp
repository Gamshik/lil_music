#include "soundcloud/platform/window_controller.h"

#include <cctype>
#include <filesystem>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>

#include "soundcloud/bridge/i_ui_bridge.h"
#include "webview/webview.h"

namespace soundcloud::platform {
namespace {

std::string read_text_file(const std::filesystem::path& file_path) {
    std::ifstream input_stream(file_path, std::ios::binary);
    if (!input_stream.is_open()) {
        throw std::runtime_error("Не удалось открыть UI asset: " + file_path.generic_string());
    }

    std::ostringstream buffer;
    buffer << input_stream.rdbuf();
    return buffer.str();
}

std::string escape_script_for_inline_html(std::string script_contents) {
    constexpr std::string_view closing_script_tag = "</script>";
    std::size_t position = 0;

    while ((position = script_contents.find(closing_script_tag, position)) != std::string::npos) {
        script_contents.replace(position, closing_script_tag.size(), "<\\/script>");
        position += 10;
    }

    return script_contents;
}

void replace_all(std::string& source, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return;
    }

    std::size_t search_position = 0;
    while ((search_position = source.find(from, search_position)) != std::string::npos) {
        source.replace(search_position, from.length(), to);
        search_position += to.length();
    }
}

std::string build_runtime_html(const std::filesystem::path& entry_file_path) {
    const std::filesystem::path ui_directory = entry_file_path.parent_path();
    std::string html = read_text_file(entry_file_path);

    // WebView на file:// плохо дружит с внешними module assets из Vite bundle.
    // Поэтому на runtime инлайним CSS и JS прямо в HTML, чтобы не зависеть от CORS/origin ограничений.
    const std::size_t script_tag_position = html.find("<script type=\"module\"");
    if (script_tag_position != std::string::npos) {
        const std::size_t script_src_begin = html.find("src=\"", script_tag_position);
        const std::size_t script_src_end = html.find('"', script_src_begin + 5U);
        const std::size_t script_tag_end = html.find("</script>", script_tag_position);
        const std::string script_relative_path =
            html.substr(script_src_begin + 5U, script_src_end - (script_src_begin + 5U));
        const std::string script_contents =
            escape_script_for_inline_html(read_text_file(ui_directory / script_relative_path));
        html.replace(script_tag_position, script_tag_end + 9U - script_tag_position, "");

        const std::size_t body_end_position = html.find("</body>");
        if (body_end_position != std::string::npos) {
            html.insert(body_end_position, "<script>" + script_contents + "</script>");
        } else {
            html += "<script>" + script_contents + "</script>";
        }
    }

    const std::size_t stylesheet_tag_position = html.find("<link rel=\"stylesheet\"");
    if (stylesheet_tag_position != std::string::npos) {
        const std::size_t stylesheet_href_begin = html.find("href=\"", stylesheet_tag_position);
        const std::size_t stylesheet_href_end = html.find('"', stylesheet_href_begin + 6U);
        const std::size_t stylesheet_tag_end = html.find('>', stylesheet_tag_position);
        const std::string stylesheet_relative_path =
            html.substr(
                stylesheet_href_begin + 6U,
                stylesheet_href_end - (stylesheet_href_begin + 6U));
        const std::string stylesheet_contents =
            read_text_file(ui_directory / stylesheet_relative_path);
        html.replace(
            stylesheet_tag_position,
            stylesheet_tag_end + 1U - stylesheet_tag_position,
            "<style>" + stylesheet_contents + "</style>");
    }

    return html;
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
        // В production-runtime показываем инлайненный HTML bundle,
        // чтобы webview не упирался в file:// ограничения для module assets.
        window.set_html(build_runtime_html(configuration.entry_file_path));
    } else {
        // Если assets отсутствуют, не оставляем пользователя с пустым окном.
        window.set_html(build_missing_asset_html(configuration.entry_file_path));
    }

    window.run();
}

}  // namespace soundcloud::platform
