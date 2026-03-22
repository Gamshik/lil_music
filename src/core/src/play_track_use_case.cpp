#include "soundcloud/core/use_cases/play_track_use_case.h"

#include <stdexcept>

namespace soundcloud::core::use_cases {

play_track_use_case::play_track_use_case(ports::i_audio_player& audio_player)
    : audio_player_(audio_player) {}

void play_track_use_case::execute(const std::string& stream_url) const {
    if (stream_url.empty()) {
        throw std::invalid_argument("Не передан stream_url для воспроизведения.");
    }

    audio_player_.load(stream_url);
    audio_player_.play();
}

}  // namespace soundcloud::core::use_cases
