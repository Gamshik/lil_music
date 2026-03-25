#include "soundcloud/core/use_cases/resume_playback_use_case.h"

namespace soundcloud::core::use_cases {

resume_playback_use_case::resume_playback_use_case(ports::i_audio_player& audio_player)
    : audio_player_(audio_player) {}

void resume_playback_use_case::execute() const {
    // Возобновление повторно использует тот же play()-порт, потому что backend
    // сам знает, как отличить resume от first-start.
    audio_player_.play();
}

}  // namespace soundcloud::core::use_cases
