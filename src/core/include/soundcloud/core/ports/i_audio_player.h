#pragma once

#include <string>

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
};

}  // namespace soundcloud::core::ports
