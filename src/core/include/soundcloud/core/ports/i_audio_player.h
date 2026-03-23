#pragma once

#include <cstdint>
#include <string>

#include "soundcloud/core/domain/playback_state.h"

namespace soundcloud::core::ports {

/**
 * Порт аудиоплеера.
 * Core использует только абстракцию управления воспроизведением.
 */
class i_audio_player {
public:
    virtual ~i_audio_player() = default;

    /**
     * Загружает указанный аудиоресурс для последующего воспроизведения.
     */
    virtual void load(const std::string& stream_url) = 0;

    /**
     * Запускает или возобновляет воспроизведение.
     */
    virtual void play() = 0;

    /**
     * Ставит воспроизведение на паузу.
     */
    virtual void pause() = 0;

    /**
     * Перематывает текущее воспроизведение к заданной позиции.
     */
    virtual void seek_to(std::int64_t position_ms) = 0;

    /**
     * Возвращает текущее нормализованное состояние плеера.
     */
    [[nodiscard]] virtual domain::playback_state get_playback_state() const = 0;
};

}  // namespace soundcloud::core::ports
