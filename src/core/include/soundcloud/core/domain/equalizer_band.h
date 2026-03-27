#pragma once

namespace soundcloud::core::domain {

/**
 * Одна полоса 10-band эквалайзера.
 * center_frequency_hz определяет фиксированную центральную частоту полосы,
 * а gain_db хранит текущее пользовательское усиление/ослабление этой полосы.
 */
struct equalizer_band {
    float center_frequency_hz = 0.0F;
    float gain_db = 0.0F;
};

}  // namespace soundcloud::core::domain
