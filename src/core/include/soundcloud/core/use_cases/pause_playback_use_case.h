#pragma once

#include "soundcloud/core/ports/i_audio_player.h"

namespace soundcloud::core::use_cases {

/**
 * Use case паузы воспроизведения.
 * Нужен, чтобы bridge не работал с портом плеера напрямую.
 */
class pause_playback_use_case {
public:
    /**
     * Создаёт use case поверх порта плеера, который умеет ставить playback на паузу.
     */
    explicit pause_playback_use_case(ports::i_audio_player& audio_player);

    /**
     * Ставит текущее воспроизведение на паузу.
     */
    void execute() const;

private:
    ports::i_audio_player& audio_player_;
};

}  // namespace soundcloud::core::use_cases
