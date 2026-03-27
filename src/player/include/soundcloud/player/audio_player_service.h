#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "soundcloud/core/ports/i_audio_player.h"

namespace soundcloud::player {

class media_foundation_audio_backend;

/**
 * Тонкий прикладной фасад плеера.
 * Этот класс не содержит аудио-логики сам по себе: он только передаёт
 * команды в реальный backend и удерживает domain/core от прямой зависимости
 * на конкретную Windows-реализацию.
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
     * Загружает новый аудиопоток в backend.
     * На уровне core это выглядит как обычная команда, но реальная подготовка
     * источника и обработка асинхронных событий происходят внутри backend-слоя.
     */
    void load(const std::string& stream_url) override;

    /**
     * Запрашивает запуск воспроизведения.
     * Если backend ещё догружает источник, команда может быть отложена до ready-state.
     */
    void play() override;

    /**
     * Ставит текущее воспроизведение на паузу.
     * Важно, что это не сбрасывает источник, а только меняет transport state.
     */
    void pause() override;

    /**
     * Перематывает текущий трек к указанной позиции.
     * Backend сам решает, можно ли выполнить seek на текущем этапе загрузки.
     */
    void seek_to(std::int64_t position_ms) override;

    /**
     * Меняет громкость backend-а в нормализованном формате 0..100.
     */
    void set_volume_percent(int volume_percent) override;

    /**
     * Возвращает доменный снимок состояния backend-а.
     * Это нужно bridge/UI, чтобы не вытаскивать из player native-детали напрямую.
     */
    [[nodiscard]] core::domain::playback_state get_playback_state() const override;

private:
    std::unique_ptr<media_foundation_audio_backend> backend_;
};

}  // namespace soundcloud::player
