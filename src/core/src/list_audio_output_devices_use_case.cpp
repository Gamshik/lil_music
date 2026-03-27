#include "soundcloud/core/use_cases/list_audio_output_devices_use_case.h"

namespace soundcloud::core::use_cases {

list_audio_output_devices_use_case::list_audio_output_devices_use_case(
    ports::i_audio_player& audio_player)
    : audio_player_(audio_player) {}

std::vector<domain::audio_output_device> list_audio_output_devices_use_case::execute() const {
    return audio_player_.list_audio_output_devices();
}

}  // namespace soundcloud::core::use_cases
