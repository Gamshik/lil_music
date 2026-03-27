#include "soundcloud/core/use_cases/set_equalizer_output_gain_use_case.h"

#include <algorithm>

namespace soundcloud::core::use_cases {

set_equalizer_output_gain_use_case::set_equalizer_output_gain_use_case(
    ports::i_audio_player& audio_player,
    ports::i_equalizer_settings_repository& equalizer_settings_repository)
    : audio_player_(audio_player),
      equalizer_settings_repository_(equalizer_settings_repository) {}

domain::equalizer_state set_equalizer_output_gain_use_case::execute(
    const float output_gain_db) const {
    audio_player_.set_equalizer_output_gain((std::clamp)(output_gain_db, -12.0F, 12.0F));
    const domain::equalizer_state equalizer_state = audio_player_.get_equalizer_state();
    equalizer_settings_repository_.save_equalizer_state(equalizer_state);
    return equalizer_state;
}

}  // namespace soundcloud::core::use_cases
