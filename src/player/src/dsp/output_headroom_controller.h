#pragma once

#include <array>

namespace soundcloud::player::dsp {

/**
 * Вычисляет auto-preamp/headroom compensation на основе суммарной АЧХ EQ.
 */
class output_headroom_controller {
public:
    /**
     * Оценивает суммарный boost всей EQ-цепочки и возвращает recommended preamp compensation.
     */
    [[nodiscard]] float compute_target_preamp_db(
        const std::array<float, 10>& band_gains_db,
        float sample_rate) const;
};

}  // namespace soundcloud::player::dsp
