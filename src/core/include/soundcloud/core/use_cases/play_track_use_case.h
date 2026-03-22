#pragma once

#include <string>

#include "soundcloud/core/ports/i_audio_player.h"
#include "soundcloud/core/ports/i_track_stream_resolver.h"

namespace soundcloud::core::use_cases {

/**
 * Use case запуска воспроизведения выбранного трека.
 * Отделяет решение о старте playback от UI-слоя и конкретного backend плеера.
 */
class play_track_use_case {
public:
    play_track_use_case(
        const ports::i_track_stream_resolver& track_stream_resolver,
        ports::i_audio_player& audio_player);

    /**
     * Разрешает playback URL по track id и запускает воспроизведение.
     */
    void execute(const std::string& track_id) const;

private:
    const ports::i_track_stream_resolver& track_stream_resolver_;
    ports::i_audio_player& audio_player_;
};

}  // namespace soundcloud::core::use_cases
