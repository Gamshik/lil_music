#pragma once

#include <string>
#include <vector>

#include "soundcloud/core/domain/track.h"

namespace soundcloud::core::domain {

/**
 * Снимок очереди воспроизведения в текущей desktop-сессии.
 */
struct playback_queue_state {
    std::string current_track_id;
    std::vector<track> queued_tracks;
    bool can_play_previous = false;
    bool can_play_next = false;
};

}  // namespace soundcloud::core::domain
