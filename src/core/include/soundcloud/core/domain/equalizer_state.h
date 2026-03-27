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
    // Последнее пользовательское состояние, отличное от Flat.
    // Это позволяет не терять ручную настройку при временном выборе пресета.
    std::array<float, 10> last_nonflat_band_gains_db{};
    // Встроенные пресеты передаются вместе со snapshot-ом, чтобы UI не хардкодил их локально.
    std::vector<equalizer_preset> available_presets;
    // Общий уровень уже обработанного EQ-сигнала.
    // Это отдельный контроль от playback volume и нужен для user-facing headroom.
    float output_gain_db = 0.0F;
    // Автоматическая компенсация, которую DSP рассчитывает поверх текущих полос.
    float headroom_compensation_db = 0.0F;
    std::string error_message;
};

}  // namespace soundcloud::core::domain
