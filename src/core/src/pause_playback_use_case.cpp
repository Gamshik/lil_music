#include "soundcloud/core/use_cases/pause_playback_use_case.h"

namespace soundcloud::core::use_cases {

pause_playback_use_case::pause_playback_use_case(ports::i_audio_player& audio_player)
    : audio_player_(audio_player) {}

void pause_playback_use_case::execute() const {
    audio_player_.pause();
}

}  // namespace soundcloud::core::use_cases
