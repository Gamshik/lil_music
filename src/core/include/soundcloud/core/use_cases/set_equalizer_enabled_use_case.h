#pragma once

#include "soundcloud/core/domain/equalizer_state.h"
#include "soundcloud/core/ports/i_audio_player.h"
#include "soundcloud/core/ports/i_equalizer_settings_repository.h"

namespace soundcloud::core::use_cases {

class set_equalizer_enabled_use_case {
public:
    set_equalizer_enabled_use_case(
        ports::i_audio_player& audio_player,
        ports::i_equalizer_settings_repository& equalizer_settings_repository);

    [[nodiscard]] domain::equalizer_state execute(bool enabled) const;

private:
    ports::i_audio_player& audio_player_;
    ports::i_equalizer_settings_repository& equalizer_settings_repository_;
};

}  // namespace soundcloud::core::use_cases
