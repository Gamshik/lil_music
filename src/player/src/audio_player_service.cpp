#include "soundcloud/player/audio_player_service.h"

#include <utility>

#include "windows_streaming_audio_backend.h"

namespace soundcloud::player {

audio_player_service::audio_player_service()
    // Фасад остаётся тем же, но реальный playback теперь живёт на PCM-level backend,
    // где можно встроить полноценный DSP-эквалайзер.
    : backend_(std::make_unique<windows_streaming_audio_backend>()) {}

audio_player_service::~audio_player_service() = default;

// Перемещение безопасно, потому что unique_ptr полностью переносит владение backend-ом.
audio_player_service::audio_player_service(audio_player_service&&) noexcept = default;

// Аналогично конструктору перемещения: просто передаём владение backend-ом новому объекту.
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

void audio_player_service::set_equalizer_enabled(const bool enabled) {
    // EQ bypass полностью делегируем backend-у, чтобы core не знал о DSP реализации.
    backend_->set_equalizer_enabled(enabled);
}

void audio_player_service::select_equalizer_preset(const core::domain::equalizer_preset_id preset_id) {
    // Preset-логика тоже сосредоточена в backend-е рядом с runtime EQ state.
    backend_->select_equalizer_preset(preset_id);
}

void audio_player_service::set_equalizer_band_gain(const std::size_t band_index, const float gain_db) {
    // Фасад лишь передаёт индекс полосы и gain дальше в backend.
    backend_->set_equalizer_band_gain(band_index, gain_db);
}

void audio_player_service::reset_equalizer() {
    // Reset живёт на стороне backend-а, потому что там же находятся preset/state/headroom правила.
    backend_->reset_equalizer();
}

void audio_player_service::set_equalizer_output_gain(const float output_gain_db) {
    // Output gain относится именно к wet/EQ path, а не к общему playback volume.
    backend_->set_equalizer_output_gain(output_gain_db);
}

void audio_player_service::apply_equalizer_state(const core::domain::equalizer_state& equalizer_state) {
    // При старте приложения backend получает уже готовый persistence snapshot и восстанавливает его.
    backend_->apply_equalizer_state(equalizer_state);
}

core::domain::playback_state audio_player_service::get_playback_state() const {
    // Возвращаем готовый доменный снимок состояния без утечки native-API наружу.
    return backend_->get_playback_state();
}

core::domain::equalizer_state audio_player_service::get_equalizer_state() const {
    // Возвращаем единый EQ snapshot для bridge/UI/persistence use case-ов.
    return backend_->get_equalizer_state();
}

}  // namespace soundcloud::player
