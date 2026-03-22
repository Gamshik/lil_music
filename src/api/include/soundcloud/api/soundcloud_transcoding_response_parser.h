#pragma once

#include <string>

namespace soundcloud::api {

/**
 * Разбирает ответ transcoding endpoint и извлекает финальный playback URL.
 */
class soundcloud_transcoding_response_parser {
public:
    /**
     * Возвращает URL медиаресурса или плейлиста, который реально нужно воспроизводить.
     */
    std::string parse(const std::string& payload) const;
};

}  // namespace soundcloud::api
