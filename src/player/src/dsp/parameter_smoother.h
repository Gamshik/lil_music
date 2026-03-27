#pragma once

#include <cstddef>

namespace soundcloud::player::dsp {

/**
 * Линейный smoother параметров для clickless обновления gain/wet/preamp.
 */
class parameter_smoother {
public:
    parameter_smoother() = default;

    /**
     * Мгновенно устанавливает текущее и целевое значение.
     * Используется при холодной инициализации без плавного перехода.
     */
    void reset(float value);

    /**
     * Назначает новую цель для параметра и длину перехода в samples.
     */
    void set_target(float target_value, std::size_t ramp_samples);

    /**
     * Продвигает smoother на consumed_samples и возвращает новое текущее значение.
     */
    [[nodiscard]] float advance(std::size_t consumed_samples);
    [[nodiscard]] float current_value() const;
    [[nodiscard]] float target_value() const;

private:
    float current_value_ = 0.0F;
    float target_value_ = 0.0F;
    std::size_t remaining_samples_ = 0;
};

}  // namespace soundcloud::player::dsp
