#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "soundcloud/core/domain/playback_queue_state.h"

namespace soundcloud::core::services {

/**
 * Session-state текущего порядка воспроизведения.
 * Хранит активный список, очередь и историю независимо от UI-экрана.
 */
class playback_session {
public:
    playback_session() = default;

    void replace_active_listing(std::vector<domain::track> tracks);
    [[nodiscard]] bool enqueue_track(const std::string& track_id);
    [[nodiscard]] bool remove_queued_track(const std::string& track_id);

    void mark_track_started(const std::string& track_id);
    [[nodiscard]] std::optional<domain::track> peek_next_track() const;
    [[nodiscard]] std::optional<domain::track> peek_previous_track() const;
    void commit_previous_track_started();

    [[nodiscard]] std::optional<domain::track> find_known_track(const std::string& track_id) const;
    [[nodiscard]] domain::playback_queue_state get_queue_state() const;

private:
    [[nodiscard]] std::optional<domain::track> find_known_track_locked(
        const std::string& track_id) const;
    [[nodiscard]] bool has_next_track_locked() const;
    void remove_from_queue_locked(const std::string& track_id);
    void update_playback_listing_locked(const std::string& track_id);

    mutable std::mutex state_mutex_;
    std::vector<domain::track> active_listing_;
    std::vector<domain::track> playback_listing_;
    std::vector<domain::track> queued_tracks_;
    std::vector<domain::track> playback_history_;
};

}  // namespace soundcloud::core::services
