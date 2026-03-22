#include "soundcloud/api/soundcloud_api_client.h"

#include <utility>

namespace soundcloud::api {

soundcloud_api_client::soundcloud_api_client(std::string client_id)
    : client_id_(std::move(client_id)) {}

std::vector<core::domain::track> soundcloud_api_client::search_tracks(
    const std::string& query) const {
    // Здесь будет HTTP-запрос к публичному API.
    // Пока skeleton возвращает пустой результат, но оставляет корректную точку расширения.
    (void)query;
    return {};
}

}  // namespace soundcloud::api
