#pragma once

#include <string>

#include "soundcloud/core/ports/i_audio_player.h"

namespace soundcloud::core::use_cases {

/**
 * Переключает active output device в playback backend-е.
 */
class select_audio_output_device_use_case {
public:
    explicit select_audio_output_device_use_case(ports::i_audio_player& audio_player);

    void execute(const std::string& device_id) const;

private:
    ports::i_audio_player& audio_player_;
};

}  // namespace soundcloud::core::use_cases
