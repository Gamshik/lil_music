#include "soundcloud/core/use_cases/set_playback_volume_use_case.h"

#include <algorithm>

namespace soundcloud::core::use_cases {

set_playback_volume_use_case::set_playback_volume_use_case(ports::i_audio_player& audio_player)
    : audio_player_(audio_player) {}

void set_playback_volume_use_case::execute(const int volume_percent) const {
    // Нормализацию диапазона держим в application-layer, чтобы UI мог
    // свободно отправлять значения слайдера, а backend получал уже валидные проценты.
    audio_player_.set_volume_percent((std::clamp)(volume_percent, 0, 100));
}

}  // namespace soundcloud::core::use_cases
