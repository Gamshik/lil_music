#pragma once

#include <vector>

#include "soundcloud/core/domain/audio_output_device.h"
#include "soundcloud/core/ports/i_audio_player.h"

namespace soundcloud::core::use_cases {

/**
 * Возвращает список доступных устройств вывода аудио из active player backend-а.
 */
class list_audio_output_devices_use_case {
public:
    explicit list_audio_output_devices_use_case(ports::i_audio_player& audio_player);

    [[nodiscard]] std::vector<domain::audio_output_device> execute() const;

private:
    ports::i_audio_player& audio_player_;
};

}  // namespace soundcloud::core::use_cases
