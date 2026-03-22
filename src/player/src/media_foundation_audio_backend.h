#pragma once

#include <memory>
#include <string>

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

    std::unique_ptr<implementation> implementation_;
};

}  // namespace soundcloud::player
