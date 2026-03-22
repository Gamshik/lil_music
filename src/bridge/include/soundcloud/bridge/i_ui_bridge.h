#pragma once

#include <functional>
#include <string>
#include <vector>

namespace soundcloud::bridge {

/**
 * Описание одной JS-функции, доступной во встроенном Web UI.
 * Bridge возвращает только строковый JSON-контракт, а платформа лишь публикует его в webview.
 */
struct ui_binding {
    std::string name;
    std::function<std::string(const std::string& request_json)> handler;
};

/**
 * Абстракция bridge-слоя между web UI и прикладными сценариями.
 * Позволяет платформенному shell публиковать JS API, не зная ничего о use case-логике.
 */
class i_ui_bridge {
public:
    virtual ~i_ui_bridge() = default;

    /**
     * Возвращает список JS-функций, которые нужно зарегистрировать в webview.
     */
    virtual std::vector<ui_binding> get_bindings() const = 0;
};

}  // namespace soundcloud::bridge
