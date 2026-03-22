#pragma once

#include <string>

#include "soundcloud/core/ports/i_audio_player.h"

namespace soundcloud::player {

/**
 * Базовый адаптер плеера.
 * Его задача — изолировать будущую audio-библиотеку от прикладных сценариев.
 */
class audio_player_service final : public core::ports::i_audio_player {
public:
    audio_player_service() = default;

    /**
     * Сохраняет адрес аудиопотока для последующей загрузки реальным backend.
     */
    void load(const std::string& stream_url) override;

    /**
     * Переводит плеер в состояние воспроизведения.
     */
    void play() override;

    /**
     * Переводит плеер в состояние паузы.
     */
    void pause() override;

private:
    std::string current_stream_url_;
    bool is_playing_ = false;
};

}  // namespace soundcloud::player
