#pragma once

#include "soundcloud/core/domain/playback_state.h"
#include "soundcloud/core/ports/i_audio_player.h"

namespace soundcloud::core::use_cases {

/**
 * Use case чтения состояния воспроизведения.
 * Позволяет UI получать единый снимок состояния через bridge.
 */
class get_playback_state_use_case {
public:
    /**
     * Создаёт use case поверх порта чтения состояния плеера.
     */
    explicit get_playback_state_use_case(const ports::i_audio_player& audio_player);

    /**
     * Возвращает текущее состояние плеера.
     */
    [[nodiscard]] domain::playback_state execute() const;

private:
    const ports::i_audio_player& audio_player_;
};

}  // namespace soundcloud::core::use_cases
