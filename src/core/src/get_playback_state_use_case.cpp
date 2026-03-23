#include "soundcloud/core/use_cases/get_playback_state_use_case.h"

namespace soundcloud::core::use_cases {

get_playback_state_use_case::get_playback_state_use_case(const ports::i_audio_player& audio_player)
    : audio_player_(audio_player) {}

domain::playback_state get_playback_state_use_case::execute() const {
    return audio_player_.get_playback_state();
}

}  // namespace soundcloud::core::use_cases
