#pragma once

#include <cstdint>

#include "soundcloud/core/ports/i_audio_player.h"

namespace soundcloud::core::use_cases {

/**
 * Use case перемотки текущего воспроизведения.
 * Изолирует seek-операцию от UI и конкретной реализации плеера.
 */
class seek_playback_use_case {
public:
    /**
     * Создаёт use case поверх порта плеера для операции seek.
     */
    explicit seek_playback_use_case(ports::i_audio_player& audio_player);

    /**
     * Перематывает текущий трек к указанной позиции в миллисекундах.
     */
    void execute(std::int64_t position_ms) const;

private:
    ports::i_audio_player& audio_player_;
};

}  // namespace soundcloud::core::use_cases
