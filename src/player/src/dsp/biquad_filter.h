#pragma once

#include <cstddef>
#include <vector>

namespace soundcloud::player::dsp {

/**
 * Нормализованные коэффициенты biquad-фильтра в форме Direct Form II Transposed.
 */
struct biquad_coefficients {
    float b0 = 1.0F;
    float b1 = 0.0F;
    float b2 = 0.0F;
    float a1 = 0.0F;
    float a2 = 0.0F;
};

/**
 * Одна biquad-секция с отдельным состоянием на каждый канал.
 */
class biquad_filter {
public:
    /**
     * Инициализирует фильтр под конкретное число каналов и стартовые коэффициенты.
     */
    void configure(std::size_t channel_count, biquad_coefficients coefficients);
    /**
     * Меняет только коэффициенты, не сбрасывая внутреннее состояние каналов.
     * Это критично для EQ: иначе при каждом обновлении полос слышались бы артефакты.
     */
    void set_coefficients(biquad_coefficients coefficients);
    /**
     * Сбрасывает внутреннюю память фильтра.
     */
    void reset();
    /**
     * Обрабатывает interleaved PCM buffer in-place.
     */
    void process(float* interleaved_samples, std::size_t frame_count, std::size_t channel_count);

private:
    biquad_coefficients coefficients_{};
    std::vector<float> z1_;
    std::vector<float> z2_;
};

[[nodiscard]] biquad_coefficients make_peaking_coefficients(
    float sample_rate,
    float center_frequency_hz,
    float q_value,
    float gain_db);

}  // namespace soundcloud::player::dsp
