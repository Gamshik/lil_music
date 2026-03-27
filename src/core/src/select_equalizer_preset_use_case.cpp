#include "soundcloud/core/use_cases/select_equalizer_preset_use_case.h"

namespace soundcloud::core::use_cases {

select_equalizer_preset_use_case::select_equalizer_preset_use_case(
    ports::i_audio_player& audio_player,
    ports::i_equalizer_settings_repository& equalizer_settings_repository)
    : audio_player_(audio_player),
      equalizer_settings_repository_(equalizer_settings_repository) {}

domain::equalizer_state select_equalizer_preset_use_case::execute(
    const domain::equalizer_preset_id preset_id) const {
    // Preset-логика целиком живёт в player/backend-е:
    // use case только оркестрирует команду и сохранение результата.
    audio_player_.select_equalizer_preset(preset_id);
    const domain::equalizer_state equalizer_state = audio_player_.get_equalizer_state();
    equalizer_settings_repository_.save_equalizer_state(equalizer_state);
    return equalizer_state;
}

}  // namespace soundcloud::core::use_cases
