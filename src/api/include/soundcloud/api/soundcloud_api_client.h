#pragma once

#include <string>
#include <vector>

#include "soundcloud/api/soundcloud_api_configuration.h"
#include "soundcloud/api/soundcloud_http_client.h"
#include "soundcloud/api/soundcloud_track_search_response_parser.h"
#include "soundcloud/core/domain/track.h"
#include "soundcloud/core/ports/i_track_catalog.h"

namespace soundcloud::api {

/**
 * Адаптер публичного SoundCloud API.
 * В skeleton-версии закрепляет контракт и место интеграции, не подменяя собой core.
 */
class soundcloud_api_client final : public core::ports::i_track_catalog {
public:
    explicit soundcloud_api_client(soundcloud_api_configuration configuration);

    /**
     * Ищет публичные треки через внешний каталог.
     */
    std::vector<core::domain::track> search_tracks(const std::string& query) const override;

private:
    soundcloud_http_client http_client_;
    soundcloud_track_search_response_parser track_search_response_parser_;
};

}  // namespace soundcloud::api
