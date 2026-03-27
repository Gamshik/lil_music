#pragma once

#include <array>
#include <string>
#include <vector>

#include "soundcloud/core/domain/equalizer_band.h"
#include "soundcloud/core/domain/equalizer_preset.h"
#include "soundcloud/core/domain/equalizer_status.h"

namespace soundcloud::core::domain {

/**
 * Полный снимок состояния эквалайзера.
 * Содержит и пользовательские настройки, и сведения для отрисовки UI.
 */
struct equalizer_state {
    equalizer_status status = equalizer_status::loading;
    bool enabled = false;
    equalizer_preset_id active_preset_id = equalizer_preset_id::flat;
    std::array<equalizer_band, 10> bands{};
    std::array<float, 10> last_nonflat_band_gains_db{};
    std::vector<equalizer_preset> available_presets;
    float output_gain_db = 0.0F;
    float headroom_compensation_db = 0.0F;
    std::string error_message;
};

}  // namespace soundcloud::core::domain
