#include "windows_streaming_audio_backend.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <sonotide/device_info.h>
#include <sonotide/equalizer.h>
#include <sonotide/playback_session.h>
#include <sonotide/playback_state.h>
#include <sonotide/result.h>
#include <sonotide/runtime.h>

namespace soundcloud::player {
namespace {

using soundcloud::core::domain::audio_output_device;
using soundcloud::core::domain::equalizer_band;
using soundcloud::core::domain::equalizer_preset;
using soundcloud::core::domain::equalizer_preset_id;
using soundcloud::core::domain::equalizer_state;
using soundcloud::core::domain::equalizer_status;
using soundcloud::core::domain::playback_state;
using soundcloud::core::domain::playback_status;

std::runtime_error make_sonotide_error(
    const std::string_view operation_name,
    const sonotide::error& error) {
    std::ostringstream message;
    message << operation_name << " завершилась ошибкой Sonotide ["
            << sonotide::to_string(error.category) << '/'
            << sonotide::to_string(error.code) << "]: " << error.message;
    if (!error.operation.empty()) {
        message << " (operation=" << error.operation << ')';
    }
    if (error.native_code.has_value()) {
        message << " [native=" << *error.native_code << ']';
    }
    return std::runtime_error(message.str());
}

void throw_if_failed(const sonotide::result<void>& result, const std::string_view operation_name) {
    if (!result) {
        throw make_sonotide_error(operation_name, result.error());
    }
}

template <typename value_type>
value_type unwrap_or_throw(
    sonotide::result<value_type> result,
    const std::string_view operation_name) {
    if (!result) {
        throw make_sonotide_error(operation_name, result.error());
    }

    return std::move(result.value());
}

playback_status to_core_playback_status(const sonotide::playback_status status) {
    switch (status) {
        case sonotide::playback_status::idle:
            return playback_status::idle;
        case sonotide::playback_status::loading:
            return playback_status::loading;
        case sonotide::playback_status::playing:
            return playback_status::playing;
        case sonotide::playback_status::paused:
            return playback_status::paused;
        case sonotide::playback_status::error:
            return playback_status::error;
    }

    return playback_status::error;
}

equalizer_status to_core_equalizer_status(const sonotide::equalizer_status status) {
    switch (status) {
        case sonotide::equalizer_status::loading:
            return equalizer_status::loading;
        case sonotide::equalizer_status::ready:
            return equalizer_status::ready;
        case sonotide::equalizer_status::unsupported_audio_path:
            return equalizer_status::unsupported_audio_path;
        case sonotide::equalizer_status::audio_engine_unavailable:
            return equalizer_status::audio_engine_unavailable;
        case sonotide::equalizer_status::error:
            return equalizer_status::error;
    }

    return equalizer_status::error;
}

equalizer_preset_id to_core_equalizer_preset_id(const sonotide::equalizer_preset_id preset_id) {
    switch (preset_id) {
        case sonotide::equalizer_preset_id::flat:
            return equalizer_preset_id::flat;
        case sonotide::equalizer_preset_id::bass_boost:
            return equalizer_preset_id::bass_boost;
        case sonotide::equalizer_preset_id::treble_boost:
            return equalizer_preset_id::treble_boost;
        case sonotide::equalizer_preset_id::vocal:
            return equalizer_preset_id::vocal;
        case sonotide::equalizer_preset_id::pop:
            return equalizer_preset_id::pop;
        case sonotide::equalizer_preset_id::rock:
            return equalizer_preset_id::rock;
        case sonotide::equalizer_preset_id::electronic:
            return equalizer_preset_id::electronic;
        case sonotide::equalizer_preset_id::hip_hop:
            return equalizer_preset_id::hip_hop;
        case sonotide::equalizer_preset_id::jazz:
            return equalizer_preset_id::jazz;
        case sonotide::equalizer_preset_id::classical:
            return equalizer_preset_id::classical;
        case sonotide::equalizer_preset_id::acoustic:
            return equalizer_preset_id::acoustic;
        case sonotide::equalizer_preset_id::dance:
            return equalizer_preset_id::dance;
        case sonotide::equalizer_preset_id::piano:
            return equalizer_preset_id::piano;
        case sonotide::equalizer_preset_id::spoken_podcast:
            return equalizer_preset_id::spoken_podcast;
        case sonotide::equalizer_preset_id::loudness:
            return equalizer_preset_id::loudness;
        case sonotide::equalizer_preset_id::custom:
            return equalizer_preset_id::custom;
    }

    return equalizer_preset_id::custom;
}

sonotide::equalizer_preset_id to_sonotide_equalizer_preset_id(
    const equalizer_preset_id preset_id) {
    switch (preset_id) {
        case equalizer_preset_id::flat:
            return sonotide::equalizer_preset_id::flat;
        case equalizer_preset_id::bass_boost:
            return sonotide::equalizer_preset_id::bass_boost;
        case equalizer_preset_id::treble_boost:
            return sonotide::equalizer_preset_id::treble_boost;
        case equalizer_preset_id::vocal:
            return sonotide::equalizer_preset_id::vocal;
        case equalizer_preset_id::pop:
            return sonotide::equalizer_preset_id::pop;
        case equalizer_preset_id::rock:
            return sonotide::equalizer_preset_id::rock;
        case equalizer_preset_id::electronic:
            return sonotide::equalizer_preset_id::electronic;
        case equalizer_preset_id::hip_hop:
            return sonotide::equalizer_preset_id::hip_hop;
        case equalizer_preset_id::jazz:
            return sonotide::equalizer_preset_id::jazz;
        case equalizer_preset_id::classical:
            return sonotide::equalizer_preset_id::classical;
        case equalizer_preset_id::acoustic:
            return sonotide::equalizer_preset_id::acoustic;
        case equalizer_preset_id::dance:
            return sonotide::equalizer_preset_id::dance;
        case equalizer_preset_id::piano:
            return sonotide::equalizer_preset_id::piano;
        case equalizer_preset_id::spoken_podcast:
            return sonotide::equalizer_preset_id::spoken_podcast;
        case equalizer_preset_id::loudness:
            return sonotide::equalizer_preset_id::loudness;
        case equalizer_preset_id::custom:
            return sonotide::equalizer_preset_id::custom;
    }

    return sonotide::equalizer_preset_id::custom;
}

equalizer_preset to_core_equalizer_preset(const sonotide::equalizer_preset& preset) {
    return equalizer_preset{
        .id = to_core_equalizer_preset_id(preset.id),
        .title = preset.title,
        .gains_db = preset.gains_db,
    };
}

equalizer_state to_core_equalizer_state(const sonotide::equalizer_state& state) {
    equalizer_state mapped_state;
    mapped_state.status = to_core_equalizer_status(state.status);
    mapped_state.enabled = state.enabled;
    mapped_state.active_preset_id = to_core_equalizer_preset_id(state.active_preset_id);
    mapped_state.last_nonflat_band_gains_db = state.last_nonflat_band_gains_db;
    mapped_state.output_gain_db = state.output_gain_db;
    mapped_state.headroom_compensation_db = state.headroom_compensation_db;
    mapped_state.error_message = state.error_message;

    for (std::size_t index = 0; index < mapped_state.bands.size(); ++index) {
        mapped_state.bands[index] = equalizer_band{
            .center_frequency_hz = state.bands[index].center_frequency_hz,
            .gain_db = state.bands[index].gain_db,
        };
    }

    mapped_state.available_presets.reserve(state.available_presets.size());
    for (const sonotide::equalizer_preset& preset : state.available_presets) {
        mapped_state.available_presets.push_back(to_core_equalizer_preset(preset));
    }

    return mapped_state;
}

sonotide::equalizer_state to_sonotide_equalizer_state(const equalizer_state& state) {
    sonotide::equalizer_state mapped_state;
    mapped_state.enabled = state.enabled;
    mapped_state.active_preset_id = to_sonotide_equalizer_preset_id(state.active_preset_id);
    mapped_state.last_nonflat_band_gains_db = state.last_nonflat_band_gains_db;
    mapped_state.output_gain_db = state.output_gain_db;

    for (std::size_t index = 0; index < mapped_state.bands.size(); ++index) {
        mapped_state.bands[index] = sonotide::equalizer_band{
            .center_frequency_hz = state.bands[index].center_frequency_hz,
            .gain_db = state.bands[index].gain_db,
        };
    }

    return mapped_state;
}

playback_state to_core_playback_state(const sonotide::playback_state& state) {
    return playback_state{
        .status = to_core_playback_status(state.status),
        .stream_url = state.source_uri,
        .error_message = state.error_message,
        .position_ms = state.position_ms,
        .duration_ms = state.duration_ms,
        .volume_percent = state.volume_percent,
        .completion_token = state.completion_token,
    };
}

std::string resolve_selected_output_device_id(
    const sonotide::playback_state& state,
    const std::vector<sonotide::device_info>& devices) {
    if (!state.preferred_output_device_id.empty()) {
        return state.preferred_output_device_id;
    }

    if (!state.active_output_device_id.empty()) {
        return state.active_output_device_id;
    }

    const auto default_device_iterator = std::find_if(
        devices.begin(),
        devices.end(),
        [](const sonotide::device_info& device) {
            return device.is_default_multimedia ||
                   device.is_default_console ||
                   device.is_default_communications;
        });
    return default_device_iterator != devices.end() ? default_device_iterator->id : std::string{};
}

audio_output_device to_core_audio_output_device(
    const sonotide::device_info& device,
    const std::string& selected_device_id) {
    return audio_output_device{
        .id = device.id,
        .display_name = device.friendly_name,
        .is_default = device.is_default_multimedia ||
                      device.is_default_console ||
                      device.is_default_communications,
        .is_selected = device.id == selected_device_id,
    };
}

}  // namespace

class windows_streaming_audio_backend::implementation {
public:
    implementation() {
        auto runtime_result = sonotide::runtime::create();
        if (!runtime_result) {
            throw make_sonotide_error("Создание Sonotide runtime", runtime_result.error());
        }

        runtime_ = std::make_unique<sonotide::runtime>(std::move(runtime_result.value()));

        sonotide::playback_session_config config;
        config.auto_play_on_load = false;
        config.initial_volume_percent = 100;

        auto session_result = runtime_->open_playback_session(config);
        if (!session_result) {
            throw make_sonotide_error(
                "Создание Sonotide playback session",
                session_result.error());
        }

        playback_session_ =
            std::make_unique<sonotide::playback_session>(std::move(session_result.value()));
    }

    void load(const std::string& stream_url) {
        std::scoped_lock lock(state_mutex_);
        throw_if_failed(playback_session_->load(stream_url), "Загрузка audio source через Sonotide");
    }

    void play() {
        std::scoped_lock lock(state_mutex_);
        throw_if_failed(playback_session_->play(), "Запуск воспроизведения через Sonotide");
    }

    void pause() {
        std::scoped_lock lock(state_mutex_);
        throw_if_failed(playback_session_->pause(), "Пауза воспроизведения через Sonotide");
    }

    void seek_to(const std::int64_t position_ms) {
        std::scoped_lock lock(state_mutex_);
        throw_if_failed(
            playback_session_->seek_to(position_ms),
            "Перемотка воспроизведения через Sonotide");
    }

    void set_volume_percent(const int volume_percent) {
        std::scoped_lock lock(state_mutex_);
        throw_if_failed(
            playback_session_->set_volume_percent(volume_percent),
            "Изменение громкости через Sonotide");
    }

    [[nodiscard]] std::vector<audio_output_device> list_audio_output_devices() const {
        std::scoped_lock lock(state_mutex_);
        auto devices_result = playback_session_->list_output_devices();
        if (!devices_result) {
            throw make_sonotide_error(
                "Перечисление output devices через Sonotide",
                devices_result.error());
        }

        const sonotide::playback_state current_state = playback_session_->state();
        const std::string selected_device_id =
            resolve_selected_output_device_id(current_state, devices_result.value());

        std::vector<audio_output_device> devices;
        devices.reserve(devices_result.value().size());
        for (const sonotide::device_info& device : devices_result.value()) {
            devices.push_back(to_core_audio_output_device(device, selected_device_id));
        }

        return devices;
    }

    void select_audio_output_device(const std::string& device_id) {
        std::scoped_lock lock(state_mutex_);
        throw_if_failed(
            playback_session_->select_output_device(device_id),
            "Переключение output device через Sonotide");
    }

    void set_equalizer_enabled(const bool enabled) {
        std::scoped_lock lock(state_mutex_);
        throw_if_failed(
            playback_session_->set_equalizer_enabled(enabled),
            "Изменение состояния эквалайзера через Sonotide");
    }

    void select_equalizer_preset(const equalizer_preset_id preset_id) {
        std::scoped_lock lock(state_mutex_);
        throw_if_failed(
            playback_session_->select_equalizer_preset(to_sonotide_equalizer_preset_id(preset_id)),
            "Выбор EQ preset через Sonotide");
    }

    void set_equalizer_band_gain(const std::size_t band_index, const float gain_db) {
        std::scoped_lock lock(state_mutex_);
        throw_if_failed(
            playback_session_->set_equalizer_band_gain(band_index, gain_db),
            "Изменение EQ band gain через Sonotide");
    }

    void reset_equalizer() {
        std::scoped_lock lock(state_mutex_);
        throw_if_failed(playback_session_->reset_equalizer(), "Сброс эквалайзера через Sonotide");
    }

    void set_equalizer_output_gain(const float output_gain_db) {
        std::scoped_lock lock(state_mutex_);
        throw_if_failed(
            playback_session_->set_equalizer_output_gain(output_gain_db),
            "Изменение EQ output gain через Sonotide");
    }

    void apply_equalizer_state(const equalizer_state& equalizer_state) {
        std::scoped_lock lock(state_mutex_);
        throw_if_failed(
            playback_session_->apply_equalizer_state(to_sonotide_equalizer_state(equalizer_state)),
            "Применение EQ state через Sonotide");
    }

    [[nodiscard]] playback_state get_playback_state() const {
        std::scoped_lock lock(state_mutex_);
        return to_core_playback_state(playback_session_->state());
    }

    [[nodiscard]] equalizer_state get_equalizer_state() const {
        std::scoped_lock lock(state_mutex_);
        return to_core_equalizer_state(playback_session_->equalizer_state());
    }

private:
    mutable std::mutex state_mutex_;
    std::unique_ptr<sonotide::runtime> runtime_;
    std::unique_ptr<sonotide::playback_session> playback_session_;
};

windows_streaming_audio_backend::windows_streaming_audio_backend()
    : implementation_(std::make_unique<implementation>()) {}

windows_streaming_audio_backend::~windows_streaming_audio_backend() = default;

void windows_streaming_audio_backend::load(const std::string& stream_url) {
    implementation_->load(stream_url);
}

void windows_streaming_audio_backend::play() {
    implementation_->play();
}

void windows_streaming_audio_backend::pause() {
    implementation_->pause();
}

void windows_streaming_audio_backend::seek_to(const std::int64_t position_ms) {
    implementation_->seek_to(position_ms);
}

void windows_streaming_audio_backend::set_volume_percent(const int volume_percent) {
    implementation_->set_volume_percent(volume_percent);
}

std::vector<core::domain::audio_output_device>
windows_streaming_audio_backend::list_audio_output_devices() const {
    return implementation_->list_audio_output_devices();
}

void windows_streaming_audio_backend::select_audio_output_device(const std::string& device_id) {
    implementation_->select_audio_output_device(device_id);
}

void windows_streaming_audio_backend::set_equalizer_enabled(const bool enabled) {
    implementation_->set_equalizer_enabled(enabled);
}

void windows_streaming_audio_backend::select_equalizer_preset(
    const core::domain::equalizer_preset_id preset_id) {
    implementation_->select_equalizer_preset(preset_id);
}

void windows_streaming_audio_backend::set_equalizer_band_gain(
    const std::size_t band_index,
    const float gain_db) {
    implementation_->set_equalizer_band_gain(band_index, gain_db);
}

void windows_streaming_audio_backend::reset_equalizer() {
    implementation_->reset_equalizer();
}

void windows_streaming_audio_backend::set_equalizer_output_gain(const float output_gain_db) {
    implementation_->set_equalizer_output_gain(output_gain_db);
}

void windows_streaming_audio_backend::apply_equalizer_state(
    const core::domain::equalizer_state& equalizer_state) {
    implementation_->apply_equalizer_state(equalizer_state);
}

core::domain::playback_state windows_streaming_audio_backend::get_playback_state() const {
    return implementation_->get_playback_state();
}

core::domain::equalizer_state windows_streaming_audio_backend::get_equalizer_state() const {
    return implementation_->get_equalizer_state();
}

}  // namespace soundcloud::player
