#pragma once

#include "soundcloud/core/ports/i_audio_player.h"

namespace soundcloud::core::use_cases {

/**
 * Use case изменения громкости playback.
 * Нужен, чтобы UI не управлял плеером напрямую даже для простого volume slider.
 */
class set_playback_volume_use_case {
public:
    /**
     * Создаёт use case поверх порта плеера для команды смены громкости.
     */
    explicit set_playback_volume_use_case(ports::i_audio_player& audio_player);

    /**
     * Устанавливает новую громкость в диапазоне 0..100 процентов.
     */
    void execute(int volume_percent) const;

private:
    ports::i_audio_player& audio_player_;
};

}  // namespace soundcloud::core::use_cases
