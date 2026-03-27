#include "soundcloud/core/use_cases/select_audio_output_device_use_case.h"

namespace soundcloud::core::use_cases {

select_audio_output_device_use_case::select_audio_output_device_use_case(
    ports::i_audio_player& audio_player)
    : audio_player_(audio_player) {}

void select_audio_output_device_use_case::execute(const std::string& device_id) const {
    audio_player_.select_audio_output_device(device_id);
}

}  // namespace soundcloud::core::use_cases
