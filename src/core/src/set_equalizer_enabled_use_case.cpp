#include "soundcloud/core/use_cases/set_equalizer_enabled_use_case.h"

namespace soundcloud::core::use_cases {

set_equalizer_enabled_use_case::set_equalizer_enabled_use_case(
    ports::i_audio_player& audio_player,
    ports::i_equalizer_settings_repository& equalizer_settings_repository)
    : audio_player_(audio_player),
      equalizer_settings_repository_(equalizer_settings_repository) {}

domain::equalizer_state set_equalizer_enabled_use_case::execute(const bool enabled) const {
    // Выключение EQ не стирает preset/bands/output gain, а только переводит DSP в bypass.
    audio_player_.set_equalizer_enabled(enabled);
    const domain::equalizer_state equalizer_state = audio_player_.get_equalizer_state();
    equalizer_settings_repository_.save_equalizer_state(equalizer_state);
    return equalizer_state;
}

}  // namespace soundcloud::core::use_cases
