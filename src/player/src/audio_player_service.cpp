#include "soundcloud/player/audio_player_service.h"

#include <utility>

#include "media_foundation_audio_backend.h"

namespace soundcloud::player {

audio_player_service::audio_player_service()
    : backend_(std::make_unique<media_foundation_audio_backend>()) {}

audio_player_service::~audio_player_service() = default;

audio_player_service::audio_player_service(audio_player_service&&) noexcept = default;

audio_player_service& audio_player_service::operator=(audio_player_service&&) noexcept = default;

void audio_player_service::load(const std::string& stream_url) {
    backend_->load(stream_url);
}

void audio_player_service::play() {
    backend_->play();
}

void audio_player_service::pause() {
    backend_->pause();
}

}  // namespace soundcloud::player
