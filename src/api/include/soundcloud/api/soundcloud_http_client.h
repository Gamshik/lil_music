#pragma once

#include <string>

#include "soundcloud/api/soundcloud_api_configuration.h"
#include "soundcloud/core/domain/track_search_request.h"

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
    std::string fetch_search_tracks_payload(const core::domain::track_search_request& request) const;

    /**
     * Загружает стартовую подборку популярных треков.
     */
    std::string fetch_featured_tracks_payload(int limit) const;

    /**
     * Выполняет запрос к transcoding endpoint и получает финальный playback payload.
     */
    std::string fetch_transcoding_payload(
        const std::string& transcoding_url,
        const std::string& track_authorization) const;

private:
    std::string perform_get_request(const std::string& path_with_query) const;

    soundcloud_api_configuration configuration_;
};

}  // namespace soundcloud::api
