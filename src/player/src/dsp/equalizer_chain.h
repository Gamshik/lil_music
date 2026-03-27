#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "soundcloud/core/domain/equalizer_band.h"
#include "soundcloud/core/domain/equalizer_preset.h"

#include "dsp/biquad_filter.h"
#include "dsp/output_headroom_controller.h"
#include "dsp/parameter_smoother.h"

namespace soundcloud::player::dsp {

/**
 * Реальная DSP-цепочка эквалайзера над PCM float buffer.
 */
class equalizer_chain {
public:
    static constexpr std::size_t band_count = 10;

    /**
     * Инициализирует DSP-цепочку под текущий format render path-а.
     */
    void configure(float sample_rate, std::size_t channel_count);
    /**
     * Сбрасывает внутреннее состояние biquad-секций.
     */
    void reset();
    /**
     * Включает/выключает wet-ветку EQ через плавный bypass.
     */
    void set_enabled(bool enabled);
    /**
     * Назначает gain для всех 10 полос сразу.
     */
    void set_band_gains(const std::array<float, band_count>& band_gains_db);
    /**
     * Меняет общий уровень уже обработанного EQ-сигнала.
     */
    void set_output_gain_db(float output_gain_db);
    /**
     * Меняет общий playback volume, приходящий из transport слоя.
     */
    void set_volume_linear(float volume_linear);
    /**
     * Обрабатывает interleaved PCM buffer in-place.
     */
    void process(float* interleaved_samples, std::size_t frame_count);

    [[nodiscard]] std::array<float, band_count> target_band_gains_db() const;
    [[nodiscard]] float headroom_compensation_db() const;
    [[nodiscard]] bool enabled() const;
    [[nodiscard]] float sample_rate() const;
    [[nodiscard]] std::size_t channel_count() const;
    [[nodiscard]] float output_gain_db() const;
    [[nodiscard]] static const std::array<float, band_count>& band_frequencies_hz();

private:
    /**
     * Пересчитывает коэффициенты полос с учётом текущего состояния smoother-ов.
     * Вызывается поблочно, чтобы обновление EQ оставалось плавным и недорогим.
     */
    void update_filter_coefficients(std::size_t control_block_frames);

    float sample_rate_ = 0.0F;
    std::size_t channel_count_ = 0;
    std::array<biquad_filter, band_count> filters_;
    std::array<parameter_smoother, band_count> band_smoothers_;
    // Wet mix даёт clickless bypass при включении/выключении EQ.
    parameter_smoother wet_mix_smoother_;
    // Auto-preamp компенсирует перегруз от суммарного boost нескольких полос.
    parameter_smoother preamp_smoother_;
    // Отдельный пользовательский "level" для EQ-ветки.
    parameter_smoother output_gain_smoother_;
    // Playback volume остаётся отдельным от EQ output gain.
    parameter_smoother volume_smoother_;
    std::array<float, band_count> current_band_gains_db_{};
    std::array<float, band_count> target_band_gains_db_{};
    output_headroom_controller headroom_controller_;
    float output_gain_db_ = 0.0F;
    float headroom_compensation_db_ = 0.0F;
    bool enabled_ = false;
};

[[nodiscard]] std::vector<soundcloud::core::domain::equalizer_preset> get_builtin_equalizer_presets();

}  // namespace soundcloud::player::dsp
