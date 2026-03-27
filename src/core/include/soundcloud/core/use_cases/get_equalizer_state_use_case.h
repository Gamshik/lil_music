#pragma once

#include "soundcloud/core/domain/equalizer_state.h"
#include "soundcloud/core/ports/i_audio_player.h"

namespace soundcloud::core::use_cases {

class get_equalizer_state_use_case {
public:
    explicit get_equalizer_state_use_case(const ports::i_audio_player& audio_player);

    [[nodiscard]] domain::equalizer_state execute() const;

private:
    const ports::i_audio_player& audio_player_;
};

}  // namespace soundcloud::core::use_cases
