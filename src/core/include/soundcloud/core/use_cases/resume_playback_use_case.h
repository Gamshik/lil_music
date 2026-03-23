#pragma once

#include "soundcloud/core/ports/i_audio_player.h"

namespace soundcloud::core::use_cases {

/**
 * Use case возобновления воспроизведения.
 * Нужен, чтобы UI не работал с портом плеера напрямую и не знал о backend-деталях.
 */
class resume_playback_use_case {
public:
    explicit resume_playback_use_case(ports::i_audio_player& audio_player);

    /**
     * Возобновляет воспроизведение текущего загруженного трека.
     */
    void execute() const;

private:
    ports::i_audio_player& audio_player_;
};

}  // namespace soundcloud::core::use_cases
