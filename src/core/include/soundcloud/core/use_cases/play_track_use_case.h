#pragma once

#include <string>

#include "soundcloud/core/ports/i_audio_player.h"

namespace soundcloud::core::use_cases {

/**
 * Use case запуска воспроизведения выбранного трека.
 * Отделяет решение о старте playback от UI-слоя и конкретного backend плеера.
 */
class play_track_use_case {
public:
    explicit play_track_use_case(ports::i_audio_player& audio_player);

    /**
     * Загружает поток и запускает воспроизведение.
     */
    void execute(const std::string& stream_url) const;

private:
    ports::i_audio_player& audio_player_;
};

}  // namespace soundcloud::core::use_cases
