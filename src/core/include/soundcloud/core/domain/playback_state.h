#pragma once

#include <cstdint>
#include <string>

namespace soundcloud::core::domain {

/**
 * Нормализованное состояние воспроизведения.
 * Используется bridge/UI без знания о деталях Media Foundation.
 */
enum class playback_status {
    idle,
    loading,
    playing,
    paused,
    error,
};

/**
 * Снимок состояния плеера на момент запроса.
 */
struct playback_state {
    playback_status status = playback_status::idle;
    std::string stream_url;
    std::string error_message;
    std::int64_t position_ms = 0;
    std::int64_t duration_ms = 0;
    std::uint64_t completion_token = 0;
};

}  // namespace soundcloud::core::domain
