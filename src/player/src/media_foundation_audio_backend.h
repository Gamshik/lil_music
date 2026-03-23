#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "soundcloud/core/domain/playback_state.h"

namespace soundcloud::player {

/**
 * Windows backend воспроизведения на базе Media Foundation.
 * Изолирован внутри player-слоя, чтобы его можно было заменить без изменения bridge/core.
 */
class media_foundation_audio_backend {
public:
    class implementation;

    media_foundation_audio_backend();
    ~media_foundation_audio_backend();

    media_foundation_audio_backend(const media_foundation_audio_backend&) = delete;
    media_foundation_audio_backend& operator=(const media_foundation_audio_backend&) = delete;

    void load(const std::string& stream_url);
    void play();
    void pause();
    void seek_to(std::int64_t position_ms);
    [[nodiscard]] core::domain::playback_state get_playback_state() const;

    std::unique_ptr<implementation> implementation_;
};

}  // namespace soundcloud::player
