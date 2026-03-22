#pragma once

#include <string>
#include <vector>

#include "soundcloud/core/domain/track.h"
#include "soundcloud/core/ports/i_track_catalog.h"

namespace soundcloud::api {

/**
 * Адаптер публичного SoundCloud API.
 * В skeleton-версии закрепляет контракт и место интеграции, не подменяя собой core.
 */
class soundcloud_api_client final : public core::ports::i_track_catalog {
public:
    explicit soundcloud_api_client(std::string client_id);

    /**
     * Ищет публичные треки через внешний каталог.
     */
    std::vector<core::domain::track> search_tracks(const std::string& query) const override;

private:
    std::string client_id_;
};

}  // namespace soundcloud::api
