#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "soundcloud/core/domain/playback_state.h"

namespace soundcloud::player {

/**
 * Audio-only backend на базе IMFMediaEngine.
 * Этот вариант оставлен как альтернативный playback-stack: он показывает, как
 * можно заменить transport backend без изменений в core/bridge.
 */
class media_engine_audio_backend {
public:
    class implementation;

    /**
     * PIMPL нужен, чтобы не протаскивать тяжёлые Windows media headers в public API.
     * Для пользователя player-слоя это просто обычный backend с load/play/pause/seek.
     */
    media_engine_audio_backend();
    ~media_engine_audio_backend();

    media_engine_audio_backend(const media_engine_audio_backend&) = delete;
    media_engine_audio_backend& operator=(const media_engine_audio_backend&) = delete;

    /**
     * Назначает новый источник и переводит engine в loading-state.
     */
    void load(const std::string& stream_url);

    /**
     * Просит Media Engine начать или продолжить воспроизведение.
     */
    void play();

    /**
     * Ставит engine на паузу, не уничтожая текущий source.
     */
    void pause();

    /**
     * Перематывает текущий source в миллисекундах.
     */
    void seek_to(std::int64_t position_ms);

    /**
     * Отдаёт доменный снимок состояния, собранный из native engine state.
     */
    [[nodiscard]] core::domain::playback_state get_playback_state() const;

private:
    std::unique_ptr<implementation> implementation_;
};

}  // namespace soundcloud::player
