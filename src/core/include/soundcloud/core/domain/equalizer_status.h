#pragma once

#include <string_view>

namespace soundcloud::core::domain {

/**
 * Нормализованное состояние доступности эквалайзера.
 * UI и bridge используют его вместо прямой зависимости от native audio engine.
 */
enum class equalizer_status {
    loading,
    ready,
    unsupported_audio_path,
    audio_engine_unavailable,
    error,
};

[[nodiscard]] inline std::string_view to_string(const equalizer_status status) {
    switch (status) {
        case equalizer_status::loading:
            return "loading";
        case equalizer_status::ready:
            return "ready";
        case equalizer_status::unsupported_audio_path:
            return "unsupported_audio_path";
        case equalizer_status::audio_engine_unavailable:
            return "audio_engine_unavailable";
        case equalizer_status::error:
            return "error";
    }

    return "error";
}

}  // namespace soundcloud::core::domain
