#include "soundcloud/core/use_cases/seek_playback_use_case.h"

#include <algorithm>

namespace soundcloud::core::use_cases {

seek_playback_use_case::seek_playback_use_case(ports::i_audio_player& audio_player)
    : audio_player_(audio_player) {}

void seek_playback_use_case::execute(const std::int64_t position_ms) const {
    audio_player_.seek_to((std::max)(position_ms, static_cast<std::int64_t>(0)));
}

}  // namespace soundcloud::core::use_cases
