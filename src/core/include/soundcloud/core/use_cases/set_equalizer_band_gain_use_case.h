#pragma once

#include <cstddef>

#include "soundcloud/core/domain/equalizer_state.h"
#include "soundcloud/core/ports/i_audio_player.h"
#include "soundcloud/core/ports/i_equalizer_settings_repository.h"

namespace soundcloud::core::use_cases {

class set_equalizer_band_gain_use_case {
public:
    set_equalizer_band_gain_use_case(
        ports::i_audio_player& audio_player,
        ports::i_equalizer_settings_repository& equalizer_settings_repository);

    [[nodiscard]] domain::equalizer_state execute(std::size_t band_index, float gain_db) const;

private:
    ports::i_audio_player& audio_player_;
    ports::i_equalizer_settings_repository& equalizer_settings_repository_;
};

}  // namespace soundcloud::core::use_cases
