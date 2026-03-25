#include "soundcloud/core/use_cases/play_track_use_case.h"

#include <stdexcept>

namespace soundcloud::core::use_cases {

play_track_use_case::play_track_use_case(
    const ports::i_track_stream_resolver& track_stream_resolver,
    ports::i_audio_player& audio_player)
    : track_stream_resolver_(track_stream_resolver),
      audio_player_(audio_player) {}

void play_track_use_case::execute(const std::string& track_id) const {
    if (track_id.empty()) {
        // Пустой идентификатор всегда считается прикладной ошибкой, а не сетевой.
        throw std::invalid_argument("Не передан идентификатор трека для воспроизведения.");
    }

    // Сначала резолвим актуальный stream URL, затем передаём его в player.
    // Так core не зависит от деталей SoundCloud API и конкретного backend.
    const std::string stream_url = track_stream_resolver_.resolve_stream_url(track_id);
    audio_player_.load(stream_url);
    audio_player_.play();
}

}  // namespace soundcloud::core::use_cases
