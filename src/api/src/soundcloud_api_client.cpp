#include "soundcloud/api/soundcloud_api_client.h"

#include <utility>

namespace soundcloud::api {

soundcloud_api_client::soundcloud_api_client(soundcloud_api_configuration configuration)
    : http_client_(configuration),
      track_search_response_parser_(std::move(configuration.client_id)) {}

std::vector<core::domain::track> soundcloud_api_client::search_tracks(
    const std::string& query) const {
    const std::string payload = http_client_.fetch_search_tracks_payload(query);
    return track_search_response_parser_.parse(payload);
}

}  // namespace soundcloud::api
