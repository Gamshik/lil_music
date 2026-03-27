#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "soundcloud/core/domain/playback_state.h"

namespace soundcloud::player {

/**
 * Windows backend воспроизведения на базе Media Foundation.
 * Здесь живут все Windows-specific детали: COM, Media Foundation events,
 * transport state и защита от race-condition при быстрых переключениях треков.
 */
class media_foundation_audio_backend {
public:
    class implementation;

    /**
     * Внешний объект нужен только как стабильный facade.
     * Вся тяжёлая логика и синхронизация живут в pimpl implementation,
     * чтобы заголовок оставался лёгким и не тащил Windows-детали наружу.
     */
    media_foundation_audio_backend();
    ~media_foundation_audio_backend();

    media_foundation_audio_backend(const media_foundation_audio_backend&) = delete;
    media_foundation_audio_backend& operator=(const media_foundation_audio_backend&) = delete;

    /**
     * Подготавливает новый media item и сбрасывает предыдущий transport.
     * Это самый чувствительный к гонкам метод, поэтому здесь особенно важен
     * request token и очистка старого item перед новым load.
     */
    void load(const std::string& stream_url);

    /**
     * Запрашивает запуск воспроизведения у Media Foundation.
     * Если источник ещё не готов, backend сам удерживает loading-state до ready-event.
     */
    void play();

    /**
     * Ставит текущий transport на паузу, не теряя source и playback position.
     */
    void pause();

    /**
     * Выполняет seek только на уже готовом источнике.
     * До ready-state перемотка не имеет смысла и должна быть запрещена.
     */
    void seek_to(std::int64_t position_ms);

    /**
     * Меняет громкость Media Foundation transport-а в процентах 0..100.
     */
    void set_volume_percent(int volume_percent);

    /**
     * Возвращает доменный playback snapshot, собранный из native state и событий.
     */
    [[nodiscard]] core::domain::playback_state get_playback_state() const;

    std::unique_ptr<implementation> implementation_;
};

}  // namespace soundcloud::player
