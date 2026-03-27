#pragma once

#include "soundcloud/core/domain/equalizer_state.h"
#include "soundcloud/core/ports/i_audio_player.h"
#include "soundcloud/core/ports/i_equalizer_settings_repository.h"

namespace soundcloud::core::use_cases {

/**
 * Меняет общий уровень обработанного EQ-сигнала.
 * Это не playback volume, а именно gain после EQ-цепочки.
 */
class set_equalizer_output_gain_use_case {
public:
    set_equalizer_output_gain_use_case(
        ports::i_audio_player& audio_player,
        ports::i_equalizer_settings_repository& equalizer_settings_repository);

    /**
     * Нормализует входной gain, применяет его в player и сохраняет новый snapshot.
     */
    [[nodiscard]] domain::equalizer_state execute(float output_gain_db) const;

private:
    ports::i_audio_player& audio_player_;
    ports::i_equalizer_settings_repository& equalizer_settings_repository_;
};

}  // namespace soundcloud::core::use_cases
