#include "soundcloud/player/audio_player_service.h"

#include <utility>

#include "media_foundation_audio_backend.h"

namespace soundcloud::player {

audio_player_service::audio_player_service()
    // Сейчас приложение использует стабильный Media Foundation backend.
    // Фасад остаётся неизменным, даже если реализацию playback-стека позже поменяем.
    : backend_(std::make_unique<media_foundation_audio_backend>()) {}

audio_player_service::~audio_player_service() = default;

audio_player_service::audio_player_service(audio_player_service&&) noexcept = default;

audio_player_service& audio_player_service::operator=(audio_player_service&&) noexcept = default;

void audio_player_service::load(const std::string& stream_url) {
    // Здесь нет собственной логики orchestration: просто пробрасываем команду
    // в конкретный backend, чтобы не распылять правила playback по нескольким слоям.
    backend_->load(stream_url);
}

void audio_player_service::play() {
    // Re-export доменной команды в backend.
    backend_->play();
}

void audio_player_service::pause() {
    // Пауза должна остаться тонкой командой без дополнительной политики.
    backend_->pause();
}

void audio_player_service::seek_to(const std::int64_t position_ms) {
    // Seek тоже полностью делегируется backend-у, потому что именно он знает
    // текущий native transport state и ограничения Media Foundation.
    backend_->seek_to(position_ms);
}

void audio_player_service::set_volume_percent(const int volume_percent) {
    // Громкость остаётся частью backend-specific управления transport-ом.
    backend_->set_volume_percent(volume_percent);
}

core::domain::playback_state audio_player_service::get_playback_state() const {
    // Возвращаем готовый доменный снимок состояния без утечки native-API наружу.
    return backend_->get_playback_state();
}

}  // namespace soundcloud::player
