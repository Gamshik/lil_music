#include "soundcloud/api/soundcloud_transcoding_response_parser.h"

#include <stdexcept>

#include "nlohmann/json.hpp"

namespace soundcloud::api {

std::string soundcloud_transcoding_response_parser::parse(const std::string& payload) const {
    try {
        // Этот endpoint должен вернуть только финальный URL.
        // Если структура ответа меняется, лучше упасть здесь с понятной ошибкой,
        // чем отдать player backend-у некорректные данные.
        const nlohmann::json root_json = nlohmann::json::parse(payload);
        const auto url_iterator = root_json.find("url");
        if (url_iterator == root_json.end() || !url_iterator->is_string()) {
            // Transcoding endpoint должен вернуть уже финальный media URL,
            // который можно отдать player backend-у без дополнительных преобразований.
            throw std::runtime_error(
                "Transcoding endpoint не вернул финальный playback URL.");
        }

        return url_iterator->get<std::string>();
    } catch (const nlohmann::json::exception& exception) {
        throw std::runtime_error(
            std::string("Не удалось разобрать JSON-ответ transcoding endpoint: ") +
            exception.what());
    }
}

}  // namespace soundcloud::api
