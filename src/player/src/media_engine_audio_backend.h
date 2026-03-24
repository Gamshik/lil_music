#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "soundcloud/core/domain/playback_state.h"

namespace soundcloud::player {

/**
 * Audio-only backend на базе IMFMediaEngine.
 * Вынесен в отдельный класс, чтобы изолировать замену playback-стека от core и bridge.
 */
class media_engine_audio_backend {
public:
    class implementation;

    media_engine_audio_backend();
    ~media_engine_audio_backend();

    media_engine_audio_backend(const media_engine_audio_backend&) = delete;
    media_engine_audio_backend& operator=(const media_engine_audio_backend&) = delete;

    void load(const std::string& stream_url);
    void play();
    void pause();
    void seek_to(std::int64_t position_ms);
    [[nodiscard]] core::domain::playback_state get_playback_state() const;

private:
    std::unique_ptr<implementation> implementation_;
};

}  // namespace soundcloud::player
