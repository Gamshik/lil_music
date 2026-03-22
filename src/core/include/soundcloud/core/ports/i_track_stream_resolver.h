#pragma once

#include <string>

namespace soundcloud::core::ports {

/**
 * Порт разрешения playback URL по идентификатору трека.
 * Нужен, чтобы core не зависел от деталей SoundCloud streams endpoint.
 */
class i_track_stream_resolver {
public:
    virtual ~i_track_stream_resolver() = default;

    /**
     * Возвращает актуальный playback URL для указанного трека.
     */
    virtual std::string resolve_stream_url(const std::string& track_id) const = 0;
};

}  // namespace soundcloud::core::ports
