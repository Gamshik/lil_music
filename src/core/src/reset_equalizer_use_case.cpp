#include "soundcloud/core/use_cases/reset_equalizer_use_case.h"

namespace soundcloud::core::use_cases {

reset_equalizer_use_case::reset_equalizer_use_case(
    ports::i_audio_player& audio_player,
    ports::i_equalizer_settings_repository& equalizer_settings_repository)
    : audio_player_(audio_player),
      equalizer_settings_repository_(equalizer_settings_repository) {}

domain::equalizer_state reset_equalizer_use_case::execute() const {
    audio_player_.reset_equalizer();
    const domain::equalizer_state equalizer_state = audio_player_.get_equalizer_state();
    equalizer_settings_repository_.save_equalizer_state(equalizer_state);
    return equalizer_state;
}

}  // namespace soundcloud::core::use_cases
