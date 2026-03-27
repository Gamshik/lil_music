#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "soundcloud/core/domain/audio_output_device.h"
#include "soundcloud/core/domain/equalizer_preset.h"
#include "soundcloud/core/domain/equalizer_state.h"
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
     * Меняет громкость текущего playback в нормализованном диапазоне 0..100.
     */
    virtual void set_volume_percent(int volume_percent) = 0;

    /**
     * Возвращает список доступных output devices и текущий активный выбор.
     */
    [[nodiscard]] virtual std::vector<domain::audio_output_device> list_audio_output_devices()
        const = 0;

    /**
     * Переключает playback backend на конкретное устройство вывода.
     * Пустой id означает возврат к system default endpoint.
     */
    virtual void select_audio_output_device(const std::string& device_id) = 0;

    /**
     * Включает или выключает EQ без потери сохранённых пользовательских настроек.
     */
    virtual void set_equalizer_enabled(bool enabled) = 0;

    /**
     * Применяет один из встроенных EQ-пресетов.
     */
    virtual void select_equalizer_preset(domain::equalizer_preset_id preset_id) = 0;

    /**
     * Меняет gain одной полосы эквалайзера в dB.
     */
    virtual void set_equalizer_band_gain(std::size_t band_index, float gain_db) = 0;

    /**
     * Сбрасывает EQ к Flat-настройке.
     */
    virtual void reset_equalizer() = 0;

    /**
     * Меняет общий уровень обработанного EQ-сигнала в dB.
     */
    virtual void set_equalizer_output_gain(float output_gain_db) = 0;

    /**
     * Восстанавливает ранее сохранённое EQ-состояние при старте приложения.
     */
    virtual void apply_equalizer_state(const domain::equalizer_state& equalizer_state) = 0;

    /**
     * Возвращает текущее нормализованное состояние плеера.
     */
    [[nodiscard]] virtual domain::playback_state get_playback_state() const = 0;

    /**
     * Возвращает снимок текущего состояния EQ и его доступности.
     */
    [[nodiscard]] virtual domain::equalizer_state get_equalizer_state() const = 0;
};

}  // namespace soundcloud::core::ports
