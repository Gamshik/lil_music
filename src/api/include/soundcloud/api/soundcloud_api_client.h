#pragma once

#include <string>
#include <vector>

#include "soundcloud/api/soundcloud_api_configuration.h"
#include "soundcloud/api/soundcloud_http_client.h"
#include "soundcloud/api/soundcloud_track_playback_reference.h"
#include "soundcloud/api/soundcloud_track_search_response_parser.h"
#include "soundcloud/api/soundcloud_transcoding_response_parser.h"
#include "soundcloud/core/domain/track.h"
#include "soundcloud/core/ports/i_track_catalog.h"
#include "soundcloud/core/ports/i_track_stream_resolver.h"

#include <mutex>
#include <unordered_map>

namespace soundcloud::api {

/**
 * Адаптер публичного SoundCloud API.
 * В skeleton-версии закрепляет контракт и место интеграции, не подменяя собой core.
 */
class soundcloud_api_client final : public core::ports::i_track_catalog,
                                    public core::ports::i_track_stream_resolver {
public:
    explicit soundcloud_api_client(soundcloud_api_configuration configuration);

    /**
     * Ищет публичные треки через внешний каталог.
     */
    std::vector<core::domain::track> search_tracks(
        const core::domain::track_search_request& request) const override;

    /**
     * Разрешает актуальный playback URL для выбранного трека.
     */
    std::string resolve_stream_url(const std::string& track_id) const override;

private:
    track_playback_reference require_track_playback_reference(const std::string& track_id) const;

    soundcloud_http_client http_client_;
    soundcloud_track_search_response_parser track_search_response_parser_;
    soundcloud_transcoding_response_parser transcoding_response_parser_;
    mutable std::mutex playback_reference_mutex_;
    mutable std::unordered_map<std::string, track_playback_reference> playback_reference_by_track_id_;
};

}  // namespace soundcloud::api
