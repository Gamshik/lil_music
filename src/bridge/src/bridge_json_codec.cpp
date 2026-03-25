#include "soundcloud/bridge/bridge_json_codec.h"

#include <cctype>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace soundcloud::bridge {
namespace {

class json_reader {
public:
    explicit json_reader(const std::string& source) : source_(source) {}

    std::optional<std::string> read_string_field_from_first_argument(
        const std::string& field_name) {
        skip_whitespace();
        if (!consume('[')) {
            return std::nullopt;
        }

        skip_whitespace();
        if (consume(']')) {
            return std::nullopt;
        }

        return read_string_field_from_object(field_name);
    }

    std::optional<int> read_integer_field_from_first_argument(const std::string& field_name) {
        skip_whitespace();
        if (!consume('[')) {
            return std::nullopt;
        }

        skip_whitespace();
        if (consume(']')) {
            return std::nullopt;
        }

        return read_integer_field_from_object(field_name);
    }

private:
    std::optional<std::string> read_string_field_from_object(const std::string& field_name) {
        if (!consume('{')) {
            return std::nullopt;
        }

        skip_whitespace();
        if (consume('}')) {
            return std::nullopt;
        }

        while (!is_end()) {
            const std::optional<std::string> key = parse_string();
            if (!key.has_value()) {
                return std::nullopt;
            }

            skip_whitespace();
            if (!consume(':')) {
                return std::nullopt;
            }

            skip_whitespace();
            if (*key == field_name) {
                return parse_string();
            }

            if (!skip_value()) {
                return std::nullopt;
            }

            skip_whitespace();
            if (consume('}')) {
                return std::nullopt;
            }

            if (!consume(',')) {
                return std::nullopt;
            }

            skip_whitespace();
        }

        return std::nullopt;
    }

    std::optional<int> read_integer_field_from_object(const std::string& field_name) {
        if (!consume('{')) {
            return std::nullopt;
        }

        skip_whitespace();
        if (consume('}')) {
            return std::nullopt;
        }

        while (!is_end()) {
            const std::optional<std::string> key = parse_string();
            if (!key.has_value()) {
                return std::nullopt;
            }

            skip_whitespace();
            if (!consume(':')) {
                return std::nullopt;
            }

            skip_whitespace();
            if (*key == field_name) {
                return parse_integer();
            }

            if (!skip_value()) {
                return std::nullopt;
            }

            skip_whitespace();
            if (consume('}')) {
                return std::nullopt;
            }

            if (!consume(',')) {
                return std::nullopt;
            }

            skip_whitespace();
        }

        return std::nullopt;
    }

    std::optional<std::string> parse_string() {
        if (!consume('"')) {
            return std::nullopt;
        }

        std::string result;

        while (!is_end()) {
            const char current_character = source_[position_++];
            if (current_character == '"') {
                return result;
            }

            if (current_character != '\\') {
                result.push_back(current_character);
                continue;
            }

            if (is_end()) {
                return std::nullopt;
            }

            const char escaped_character = source_[position_++];
            switch (escaped_character) {
                case '"':
                case '\\':
                case '/':
                    result.push_back(escaped_character);
                    break;
                case 'b':
                    result.push_back('\b');
                    break;
                case 'f':
                    result.push_back('\f');
                    break;
                case 'n':
                    result.push_back('\n');
                    break;
                case 'r':
                    result.push_back('\r');
                    break;
                case 't':
                    result.push_back('\t');
                    break;
                case 'u':
                    if (!skip_unicode_escape()) {
                        return std::nullopt;
                    }
                    break;
                default:
                    return std::nullopt;
            }
        }

        return std::nullopt;
    }

    std::optional<int> parse_integer() {
        if (peek('"')) {
            const std::optional<std::string> integer_as_string = parse_string();
            if (!integer_as_string.has_value() || integer_as_string->empty()) {
                return std::nullopt;
            }

            try {
                std::size_t parsed_length = 0;
                const long long parsed_value =
                    std::stoll(*integer_as_string, &parsed_length, 10);
                if (parsed_length != integer_as_string->size() ||
                    parsed_value < std::numeric_limits<int>::min() ||
                    parsed_value > std::numeric_limits<int>::max()) {
                    return std::nullopt;
                }

                return static_cast<int>(parsed_value);
            } catch (const std::exception&) {
                return std::nullopt;
            }
        }

        const std::size_t token_begin = position_;
        if (peek('-')) {
            ++position_;
        }

        if (!skip_digits()) {
            position_ = token_begin;
            return std::nullopt;
        }

        if (peek('.') || peek('e') || peek('E')) {
            position_ = token_begin;
            return std::nullopt;
        }

        try {
            const long long parsed_value =
                std::stoll(source_.substr(token_begin, position_ - token_begin));
            if (parsed_value < std::numeric_limits<int>::min() ||
                parsed_value > std::numeric_limits<int>::max()) {
                position_ = token_begin;
                return std::nullopt;
            }

            return static_cast<int>(parsed_value);
        } catch (const std::exception&) {
            position_ = token_begin;
            return std::nullopt;
        }
    }

    bool skip_value() {
        skip_whitespace();
        if (is_end()) {
            return false;
        }

        const char current_character = source_[position_];
        if (current_character == '"') {
            return parse_string().has_value();
        }

        if (current_character == '{') {
            return skip_object();
        }

        if (current_character == '[') {
            return skip_array();
        }

        if (std::isdigit(static_cast<unsigned char>(current_character)) != 0 ||
            current_character == '-') {
            return skip_number();
        }

        return skip_literal("true") || skip_literal("false") || skip_literal("null");
    }

    bool skip_object() {
        if (!consume('{')) {
            return false;
        }

        skip_whitespace();
        if (consume('}')) {
            return true;
        }

        while (!is_end()) {
            if (!parse_string().has_value()) {
                return false;
            }

            skip_whitespace();
            if (!consume(':')) {
                return false;
            }

            if (!skip_value()) {
                return false;
            }

            skip_whitespace();
            if (consume('}')) {
                return true;
            }

            if (!consume(',')) {
                return false;
            }

            skip_whitespace();
        }

        return false;
    }

    bool skip_array() {
        if (!consume('[')) {
            return false;
        }

        skip_whitespace();
        if (consume(']')) {
            return true;
        }

        while (!is_end()) {
            if (!skip_value()) {
                return false;
            }

            skip_whitespace();
            if (consume(']')) {
                return true;
            }

            if (!consume(',')) {
                return false;
            }

            skip_whitespace();
        }

        return false;
    }

    bool skip_number() {
        if (peek('-')) {
            ++position_;
        }

        if (!skip_digits()) {
            return false;
        }

        if (peek('.')) {
            ++position_;
            if (!skip_digits()) {
                return false;
            }
        }

        if (peek('e') || peek('E')) {
            ++position_;
            if (peek('+') || peek('-')) {
                ++position_;
            }

            if (!skip_digits()) {
                return false;
            }
        }

        return true;
    }

    bool skip_digits() {
        const std::size_t digits_begin = position_;
        while (!is_end() && std::isdigit(static_cast<unsigned char>(source_[position_])) != 0) {
            ++position_;
        }

        return position_ > digits_begin;
    }

    bool skip_literal(std::string_view literal) {
        if (source_.substr(position_, literal.size()) != literal) {
            return false;
        }

        position_ += literal.size();
        return true;
    }

    bool skip_unicode_escape() {
        for (int index = 0; index < 4; ++index) {
            if (is_end() ||
                std::isxdigit(static_cast<unsigned char>(source_[position_])) == 0) {
                return false;
            }

            ++position_;
        }

        return true;
    }

    void skip_whitespace() {
        while (!is_end() &&
               std::isspace(static_cast<unsigned char>(source_[position_])) != 0) {
            ++position_;
        }
    }

    bool consume(const char expected_character) {
        if (!peek(expected_character)) {
            return false;
        }

        ++position_;
        return true;
    }

    bool peek(const char expected_character) const {
        return !is_end() && source_[position_] == expected_character;
    }

    bool is_end() const {
        return position_ >= source_.size();
    }

    const std::string& source_;
    std::size_t position_ = 0;
};

}  // namespace

std::string bridge_json_codec::escape_string(const std::string& value) {
    // Экранируем строку вручную, чтобы bridge не зависел от внешнего JSON-пакета
    // для простых ответов и мог отдавать корректный JSON literal в любую сторону.
    std::ostringstream escaped_value;
    escaped_value << '"';

    for (const unsigned char character : value) {
        switch (character) {
            case '"':
                escaped_value << "\\\"";
                break;
            case '\\':
                escaped_value << "\\\\";
                break;
            case '\b':
                escaped_value << "\\b";
                break;
            case '\f':
                escaped_value << "\\f";
                break;
            case '\n':
                escaped_value << "\\n";
                break;
            case '\r':
                escaped_value << "\\r";
                break;
            case '\t':
                escaped_value << "\\t";
                break;
            default:
                if (character < 0x20U) {
                    escaped_value << "\\u00";
                    constexpr char hex_digits[] = "0123456789ABCDEF";
                    escaped_value << hex_digits[(character >> 4U) & 0x0FU]
                                  << hex_digits[character & 0x0FU];
                } else {
                    escaped_value << static_cast<char>(character);
                }
                break;
        }
    }

    escaped_value << '"';
    return escaped_value.str();
}

std::optional<std::string> bridge_json_codec::read_string_field_from_first_argument(
    const std::string& request_json,
    const std::string& field_name) {
    // Bridge вызывает JS-функции в формате массива аргументов.
    // Здесь читаем только первый аргумент и достаём из него нужное поле.
    json_reader reader(request_json);
    return reader.read_string_field_from_first_argument(field_name);
}

std::optional<int> bridge_json_codec::read_integer_field_from_first_argument(
    const std::string& request_json,
    const std::string& field_name) {
    // Аналогично строковому варианту, но для числовых параметров вроде limit/offset/positionMs.
    json_reader reader(request_json);
    return reader.read_integer_field_from_first_argument(field_name);
}

}  // namespace soundcloud::bridge
