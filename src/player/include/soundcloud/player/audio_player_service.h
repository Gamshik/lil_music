#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "soundcloud/core/ports/i_audio_player.h"

namespace soundcloud::player {

class media_foundation_audio_backend;

/**
 * Базовый адаптер плеера.
 * Его задача — изолировать будущую audio-библиотеку от прикладных сценариев.
 */
class audio_player_service final : public core::ports::i_audio_player {
public:
    audio_player_service();
    ~audio_player_service() override;

    audio_player_service(const audio_player_service&) = delete;
    audio_player_service& operator=(const audio_player_service&) = delete;
    audio_player_service(audio_player_service&&) noexcept;
    audio_player_service& operator=(audio_player_service&&) noexcept;

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

    /**
     * Перематывает текущий трек к указанной позиции.
     */
    void seek_to(std::int64_t position_ms) override;

    /**
     * Возвращает текущее состояние Media Foundation backend в доменном формате.
     */
    [[nodiscard]] core::domain::playback_state get_playback_state() const override;

private:
    std::unique_ptr<media_foundation_audio_backend> backend_;
};

}  // namespace soundcloud::player
