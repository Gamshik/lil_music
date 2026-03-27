#pragma once

#include <optional>
#include <string>

namespace soundcloud::bridge {

/**
 * Минимальный JSON codec для bridge-контракта.
 * Нужен, чтобы не зависеть от private API webview и держать сериализацию на стороне bridge-слоя.
 */
class bridge_json_codec {
public:
    /**
     * Экранирует строку как JSON string literal вместе с кавычками.
     */
    static std::string escape_string(const std::string& value);

    /**
     * Читает строковое поле из первого аргумента bridge-вызова.
     * Ожидаемый формат: `[{"fieldName":"value"}]`.
     */
    static std::optional<std::string> read_string_field_from_first_argument(
        const std::string& request_json,
        const std::string& field_name);

    /**
     * Читает целочисленное поле из первого аргумента bridge-вызова.
     * Ожидаемый формат: `[{"fieldName":123}]`.
     */
    static std::optional<int> read_integer_field_from_first_argument(
        const std::string& request_json,
        const std::string& field_name);

    static std::optional<float> read_float_field_from_first_argument(
        const std::string& request_json,
        const std::string& field_name);

    static std::optional<bool> read_boolean_field_from_first_argument(
        const std::string& request_json,
        const std::string& field_name);
};

}  // namespace soundcloud::bridge
