#pragma once

#include <string>

#include "soundcloud/api/soundcloud_api_configuration.h"

namespace soundcloud::api {

/**
 * HTTP-клиент публичного SoundCloud API.
 * Инкапсулирует детали сетевого транспорта и возвращает сырой JSON payload.
 */
class soundcloud_http_client {
public:
    explicit soundcloud_http_client(soundcloud_api_configuration configuration);

    /**
     * Выполняет запрос поиска треков через api-v2.soundcloud.com.
     */
    std::string fetch_search_tracks_payload(const std::string& query) const;

private:
    std::string perform_get_request(const std::string& path_with_query) const;

    soundcloud_api_configuration configuration_;
};

}  // namespace soundcloud::api
