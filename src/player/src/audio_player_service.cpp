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

void audio_player_service::seek_to(const std::int64_t position_ms) {
    backend_->seek_to(position_ms);
}

core::domain::playback_state audio_player_service::get_playback_state() const {
    return backend_->get_playback_state();
}

}  // namespace soundcloud::player
