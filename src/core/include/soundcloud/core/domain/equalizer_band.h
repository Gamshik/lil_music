#pragma once

namespace soundcloud::core::domain {

/**
 * Одна полоса 10-band эквалайзера.
 */
struct equalizer_band {
    float center_frequency_hz = 0.0F;
    float gain_db = 0.0F;
};

}  // namespace soundcloud::core::domain
