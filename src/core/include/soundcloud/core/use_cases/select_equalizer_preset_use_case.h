#pragma once

#include "soundcloud/core/domain/equalizer_preset.h"
#include "soundcloud/core/domain/equalizer_state.h"
#include "soundcloud/core/ports/i_audio_player.h"
#include "soundcloud/core/ports/i_equalizer_settings_repository.h"

namespace soundcloud::core::use_cases {

class select_equalizer_preset_use_case {
public:
    select_equalizer_preset_use_case(
        ports::i_audio_player& audio_player,
        ports::i_equalizer_settings_repository& equalizer_settings_repository);

    [[nodiscard]] domain::equalizer_state execute(domain::equalizer_preset_id preset_id) const;

private:
    ports::i_audio_player& audio_player_;
    ports::i_equalizer_settings_repository& equalizer_settings_repository_;
};

}  // namespace soundcloud::core::use_cases
