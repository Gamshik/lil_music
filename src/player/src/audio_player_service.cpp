#include "soundcloud/player/audio_player_service.h"

namespace soundcloud::player {

void audio_player_service::load(const std::string& stream_url) {
    current_stream_url_ = stream_url;
    is_playing_ = false;
}

void audio_player_service::play() {
    if (!current_stream_url_.empty()) {
        is_playing_ = true;
    }
}

void audio_player_service::pause() {
    is_playing_ = false;
}

}  // namespace soundcloud::player
