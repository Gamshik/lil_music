#include "windows_streaming_audio_backend.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <mutex>
#include <numbers>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <avrt.h>
#include <ksmedia.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>
#include <propidl.h>

#include "dsp/equalizer_chain.h"
#include "dsp/output_headroom_controller.h"

namespace soundcloud::player {
namespace {

using soundcloud::core::domain::equalizer_band;
using soundcloud::core::domain::equalizer_preset;
using soundcloud::core::domain::equalizer_preset_id;
using soundcloud::core::domain::equalizer_state;
using soundcloud::core::domain::equalizer_status;
using soundcloud::core::domain::playback_state;
using soundcloud::core::domain::playback_status;

template <typename interface_type>
class com_ptr {
public:
    com_ptr() = default;
    explicit com_ptr(interface_type* value) : value_(value) {}

    com_ptr(const com_ptr&) = delete;
    com_ptr& operator=(const com_ptr&) = delete;

    com_ptr(com_ptr&& other) noexcept : value_(other.value_) {
        other.value_ = nullptr;
    }

    com_ptr& operator=(com_ptr&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        reset();
        value_ = other.value_;
        other.value_ = nullptr;
        return *this;
    }

    ~com_ptr() {
        reset();
    }

    [[nodiscard]] interface_type* get() const {
        return value_;
    }

    [[nodiscard]] interface_type** put() {
        reset();
        return &value_;
    }

    void reset(interface_type* value = nullptr) {
        if (value_ != nullptr) {
            value_->Release();
        }

        value_ = value;
    }

private:
    interface_type* value_ = nullptr;
};

struct cotaskmem_deleter {
    void operator()(WAVEFORMATEX* format) const {
        if (format != nullptr) {
            CoTaskMemFree(format);
        }
    }
};

struct render_format_descriptor {
    std::unique_ptr<WAVEFORMATEX, cotaskmem_deleter> wave_format;
    std::uint32_t channel_count = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t bits_per_sample = 0;
    std::uint32_t block_align = 0;
    bool is_float = false;
};

struct worker_context {
    com_ptr<IMFSourceReader> source_reader;
    com_ptr<IMMDevice> render_device;
    com_ptr<IAudioClient> audio_client;
    com_ptr<IAudioRenderClient> render_client;
    render_format_descriptor render_format;
    dsp::equalizer_chain equalizer_chain;
    std::vector<float> decoded_samples;
    std::size_t decoded_sample_offset = 0;
    std::int64_t next_sample_time_100ns = 0;
    std::int64_t duration_100ns = 0;
    UINT32 buffer_frame_count = 0;
    bool source_ready = false;
    bool end_of_stream_reached = false;
    bool audio_client_started = false;
};

std::runtime_error make_hresult_error(
    const std::string_view operation_name,
    const HRESULT result_code) {
    std::ostringstream message;
    message << operation_name << " завершилась ошибкой HRESULT 0x" << std::hex
            << static_cast<unsigned long>(result_code) << std::dec << ": "
            << std::system_category().message(result_code);
    return std::runtime_error(message.str());
}

void throw_if_failed(const HRESULT result_code, const std::string_view operation_name) {
    if (FAILED(result_code)) {
        throw make_hresult_error(operation_name, result_code);
    }
}

std::wstring utf8_to_utf16(const std::string& value) {
    if (value.empty()) {
        return {};
    }

    const int wide_size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.c_str(),
        static_cast<int>(value.size()),
        nullptr,
        0);
    if (wide_size <= 0) {
        throw std::runtime_error("Не удалось преобразовать UTF-8 URL в UTF-16.");
    }

    std::wstring wide_value(static_cast<std::size_t>(wide_size), L'\0');
    const int converted_size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.c_str(),
        static_cast<int>(value.size()),
        wide_value.data(),
        wide_size);
    if (converted_size != wide_size) {
        throw std::runtime_error("Не удалось преобразовать UTF-8 URL в UTF-16.");
    }

    return wide_value;
}

std::int64_t convert_100ns_to_milliseconds(const std::int64_t value_100ns) {
    return value_100ns > 0 ? value_100ns / 10000 : 0;
}

float convert_volume_percent_to_linear(const int volume_percent) {
    return static_cast<float>((std::clamp)(volume_percent, 0, 100)) / 100.0F;
}

bool is_extensible_float_format(const WAVEFORMATEX& wave_format) {
    if (wave_format.wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
        return false;
    }

    const auto* extensible_format =
        reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(&wave_format);
    return extensible_format->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
}

bool is_extensible_pcm_format(const WAVEFORMATEX& wave_format) {
    if (wave_format.wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
        return false;
    }

    const auto* extensible_format =
        reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(&wave_format);
    return extensible_format->SubFormat == KSDATAFORMAT_SUBTYPE_PCM;
}

bool is_supported_render_format(const WAVEFORMATEX& wave_format) {
    return wave_format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
           wave_format.wFormatTag == WAVE_FORMAT_PCM ||
           is_extensible_float_format(wave_format) ||
           is_extensible_pcm_format(wave_format);
}

render_format_descriptor describe_wave_format(WAVEFORMATEX* wave_format) {
    if (wave_format == nullptr) {
        throw std::runtime_error("Audio engine не вернул mix format.");
    }

    if (!is_supported_render_format(*wave_format)) {
        throw std::runtime_error(
            "Текущий audio path использует неподдерживаемый PCM-формат устройства.");
    }

    render_format_descriptor descriptor;
    descriptor.channel_count = wave_format->nChannels;
    descriptor.sample_rate = wave_format->nSamplesPerSec;
    descriptor.bits_per_sample = wave_format->wBitsPerSample;
    descriptor.block_align = wave_format->nBlockAlign;
    descriptor.is_float = wave_format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
                          is_extensible_float_format(*wave_format);
    descriptor.wave_format.reset(wave_format);
    return descriptor;
}

void configure_source_reader_output(
    IMFSourceReader* source_reader,
    const render_format_descriptor& render_format) {
    com_ptr<IMFMediaType> output_type;
    throw_if_failed(MFCreateMediaType(output_type.put()), "Создание Media Foundation media type");
    throw_if_failed(
        output_type.get()->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
        "Настройка major type source reader");
    throw_if_failed(
        output_type.get()->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float),
        "Настройка float PCM output type");
    throw_if_failed(
        output_type.get()->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, render_format.channel_count),
        "Настройка количества каналов source reader");
    throw_if_failed(
        output_type.get()->SetUINT32(
            MF_MT_AUDIO_SAMPLES_PER_SECOND,
            render_format.sample_rate),
        "Настройка sample rate source reader");
    throw_if_failed(
        output_type.get()->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32),
        "Настройка bits per sample source reader");
    throw_if_failed(
        output_type.get()->SetUINT32(
            MF_MT_AUDIO_BLOCK_ALIGNMENT,
            render_format.channel_count * sizeof(float)),
        "Настройка block alignment source reader");
    throw_if_failed(
        output_type.get()->SetUINT32(
            MF_MT_AUDIO_AVG_BYTES_PER_SECOND,
            render_format.sample_rate * render_format.channel_count * sizeof(float)),
        "Настройка average bytes per second source reader");
    throw_if_failed(
        output_type.get()->SetUINT32(MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, 32),
        "Настройка valid bits source reader");
    throw_if_failed(
        output_type.get()->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE),
        "Настройка independent samples source reader");
    throw_if_failed(
        source_reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE),
        "Выбор audio stream source reader");
    throw_if_failed(
        source_reader->SetCurrentMediaType(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
            nullptr,
            output_type.get()),
        "Настройка output media type source reader");
}

com_ptr<IMFSourceReader> create_source_reader(
    const std::string& stream_url,
    const render_format_descriptor& render_format) {
    com_ptr<IMFAttributes> attributes;
    throw_if_failed(MFCreateAttributes(attributes.put(), 2), "Создание атрибутов source reader");
    throw_if_failed(
        attributes.get()->SetUINT32(MF_LOW_LATENCY, TRUE),
        "Включение low latency source reader");
    throw_if_failed(
        attributes.get()->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, FALSE),
        "Отключение hardware transforms source reader");

    com_ptr<IMFSourceReader> source_reader;
    const std::wstring wide_stream_url = utf8_to_utf16(stream_url);
    throw_if_failed(
        MFCreateSourceReaderFromURL(wide_stream_url.c_str(), attributes.get(), source_reader.put()),
        "Создание source reader из stream URL");
    configure_source_reader_output(source_reader.get(), render_format);
    return source_reader;
}

std::int64_t query_source_duration_100ns(IMFSourceReader* source_reader) {
    PROPVARIANT duration_value;
    PropVariantInit(&duration_value);
    const HRESULT duration_result = source_reader->GetPresentationAttribute(
        static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE),
        MF_PD_DURATION,
        &duration_value);
    if (FAILED(duration_result)) {
        PropVariantClear(&duration_value);
        return 0;
    }

    std::int64_t duration_100ns = 0;
    if (duration_value.vt == VT_UI8) {
        duration_100ns = static_cast<std::int64_t>(duration_value.uhVal.QuadPart);
    } else if (duration_value.vt == VT_I8) {
        duration_100ns = duration_value.hVal.QuadPart;
    }

    PropVariantClear(&duration_value);
    return duration_100ns;
}

void reset_audio_client(worker_context& context) {
    if (context.audio_client_started && context.audio_client.get() != nullptr) {
        context.audio_client.get()->Stop();
        context.audio_client_started = false;
    }

    if (context.audio_client.get() != nullptr) {
        context.audio_client.get()->Reset();
    }
}

void convert_float_to_render_format(
    const float* source_samples,
    const std::size_t frame_count,
    const render_format_descriptor& render_format,
    BYTE* destination_bytes) {
    const std::size_t sample_count = frame_count * render_format.channel_count;
    if (render_format.is_float) {
        std::memcpy(destination_bytes, source_samples, sample_count * sizeof(float));
        return;
    }

    if (render_format.bits_per_sample == 16) {
        auto* destination = reinterpret_cast<std::int16_t*>(destination_bytes);
        for (std::size_t sample_index = 0; sample_index < sample_count; ++sample_index) {
            const float clamped_sample = (std::clamp)(source_samples[sample_index], -1.0F, 1.0F);
            destination[sample_index] = static_cast<std::int16_t>(clamped_sample * 32767.0F);
        }
        return;
    }

    if (render_format.bits_per_sample == 32) {
        auto* destination = reinterpret_cast<std::int32_t*>(destination_bytes);
        for (std::size_t sample_index = 0; sample_index < sample_count; ++sample_index) {
            const float clamped_sample = (std::clamp)(source_samples[sample_index], -1.0F, 1.0F);
            destination[sample_index] = static_cast<std::int32_t>(clamped_sample * 2147483647.0F);
        }
        return;
    }

    std::memset(destination_bytes, 0, frame_count * render_format.block_align);
}

bool band_gains_match(
    const std::array<equalizer_band, 10>& bands,
    const std::array<float, 10>& gains_db) {
    for (std::size_t index = 0; index < bands.size(); ++index) {
        if (std::fabs(bands[index].gain_db - gains_db[index]) > 0.01F) {
            return false;
        }
    }

    return true;
}

}  // namespace

class windows_streaming_audio_backend::implementation {
public:
    implementation()
        : available_presets_(dsp::get_builtin_equalizer_presets()) {
        populate_default_equalizer_state_locked();

        throw_if_failed(MFStartup(MF_VERSION), "Запуск Media Foundation для streaming backend");
        media_foundation_started_ = true;

        worker_thread_ = std::thread([this]() { worker_main(); });
        {
            std::scoped_lock lock(state_mutex_);
            device_reinitialization_requested_ = true;
        }
        command_condition_variable_.notify_all();
    }

    ~implementation() {
        {
            std::scoped_lock lock(state_mutex_);
            shutting_down_ = true;
        }
        command_condition_variable_.notify_all();

        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }

        if (media_foundation_started_) {
            MFShutdown();
        }
    }

    void load(const std::string& stream_url) {
        if (stream_url.empty()) {
            throw std::invalid_argument("Не передан stream_url для воспроизведения.");
        }

        std::scoped_lock lock(state_mutex_);
        requested_stream_url_ = stream_url;
        ++requested_source_generation_;
        playback_intent_playing_ = true;
        pending_seek_ms_.reset();
        playback_state_.status = playback_status::loading;
        playback_state_.stream_url = stream_url;
        playback_state_.error_message.clear();
        playback_state_.position_ms = 0;
        playback_state_.duration_ms = 0;
        current_stream_url_.clear();
        source_ready_ = false;
        command_condition_variable_.notify_all();
    }

    void play() {
        std::scoped_lock lock(state_mutex_);
        playback_intent_playing_ = true;
        if (requested_stream_url_.empty() && current_stream_url_.empty()) {
            playback_state_.status = playback_status::error;
            playback_state_.error_message = "Плеер ещё не загрузил аудиопоток.";
            throw std::runtime_error(playback_state_.error_message);
        }

        if (!source_ready_) {
            playback_state_.status = playback_status::loading;
        } else {
            playback_state_.status = playback_status::playing;
            playback_state_.error_message.clear();
        }
        command_condition_variable_.notify_all();
    }

    void pause() {
        std::scoped_lock lock(state_mutex_);
        playback_intent_playing_ = false;
        if (!current_stream_url_.empty() || source_ready_) {
            playback_state_.status = playback_status::paused;
            playback_state_.error_message.clear();
        }
        command_condition_variable_.notify_all();
    }

    void seek_to(const std::int64_t position_ms) {
        std::scoped_lock lock(state_mutex_);
        if (!source_ready_) {
            throw std::runtime_error("Нельзя перемотать трек, пока плеер не загрузил аудиопоток.");
        }

        pending_seek_ms_ = (std::max)(position_ms, static_cast<std::int64_t>(0));
        playback_state_.position_ms = *pending_seek_ms_;
        if (playback_intent_playing_) {
            playback_state_.status = playback_status::loading;
        }
        command_condition_variable_.notify_all();
    }

    void set_volume_percent(const int volume_percent) {
        std::scoped_lock lock(state_mutex_);
        playback_state_.volume_percent = (std::clamp)(volume_percent, 0, 100);
        command_condition_variable_.notify_all();
    }

    void set_equalizer_enabled(const bool enabled) {
        std::scoped_lock lock(state_mutex_);
        equalizer_state_.enabled = enabled;
        recompute_equalizer_metadata_locked(current_sample_rate_or_default_locked());
        command_condition_variable_.notify_all();
    }

    void select_equalizer_preset(const equalizer_preset_id preset_id) {
        std::scoped_lock lock(state_mutex_);
        if (preset_id == equalizer_preset_id::custom) {
            return;
        }

        const auto preset_iterator = std::ranges::find(
            available_presets_,
            preset_id,
            &equalizer_preset::id);
        if (preset_iterator == available_presets_.end()) {
            throw std::runtime_error("Неизвестный EQ preset.");
        }

        for (std::size_t index = 0; index < equalizer_state_.bands.size(); ++index) {
            equalizer_state_.bands[index].gain_db = preset_iterator->gains_db[index];
        }
        equalizer_state_.active_preset_id = preset_id;
        update_last_nonflat_state_locked();
        recompute_equalizer_metadata_locked(current_sample_rate_or_default_locked());
        command_condition_variable_.notify_all();
    }

    void set_equalizer_band_gain(const std::size_t band_index, const float gain_db) {
        std::scoped_lock lock(state_mutex_);
        if (band_index >= equalizer_state_.bands.size()) {
            throw std::out_of_range("EQ band index outside of 10-band range.");
        }

        equalizer_state_.bands[band_index].gain_db = (std::clamp)(gain_db, -12.0F, 12.0F);
        recalculate_active_preset_locked();
        update_last_nonflat_state_locked();
        recompute_equalizer_metadata_locked(current_sample_rate_or_default_locked());
        command_condition_variable_.notify_all();
    }

    void reset_equalizer() {
        select_equalizer_preset(equalizer_preset_id::flat);

        std::scoped_lock lock(state_mutex_);
        equalizer_state_.output_gain_db = 0.0F;
        recompute_equalizer_metadata_locked(current_sample_rate_or_default_locked());
        command_condition_variable_.notify_all();
    }

    void set_equalizer_output_gain(const float output_gain_db) {
        std::scoped_lock lock(state_mutex_);
        equalizer_state_.output_gain_db = (std::clamp)(output_gain_db, -12.0F, 12.0F);
        command_condition_variable_.notify_all();
    }

    void apply_equalizer_state(const equalizer_state& equalizer_state) {
        std::scoped_lock lock(state_mutex_);
        equalizer_state_.enabled = equalizer_state.enabled;
        for (std::size_t index = 0; index < equalizer_state_.bands.size(); ++index) {
            equalizer_state_.bands[index].gain_db =
                (std::clamp)(equalizer_state.bands[index].gain_db, -12.0F, 12.0F);
            equalizer_state_.last_nonflat_band_gains_db[index] =
                equalizer_state.last_nonflat_band_gains_db[index];
        }
        equalizer_state_.active_preset_id = equalizer_state.active_preset_id;
        equalizer_state_.output_gain_db = (std::clamp)(equalizer_state.output_gain_db, -12.0F, 12.0F);
        recalculate_active_preset_locked();
        recompute_equalizer_metadata_locked(current_sample_rate_or_default_locked());
        command_condition_variable_.notify_all();
    }

    [[nodiscard]] playback_state get_playback_state() const {
        std::scoped_lock lock(state_mutex_);
        return playback_state_;
    }

    [[nodiscard]] equalizer_state get_equalizer_state() const {
        std::scoped_lock lock(state_mutex_);
        return equalizer_state_;
    }

private:
    void worker_main() {
        const HRESULT com_result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool worker_should_uninitialize_com = SUCCEEDED(com_result);
        DWORD task_index = 0;
        HANDLE mmcss_handle = AvSetMmThreadCharacteristicsW(L"Audio", &task_index);

        worker_context context;

        try {
            while (true) {
                std::optional<std::uint64_t> load_generation;
                std::optional<std::int64_t> seek_ms;
                bool should_play = false;
                bool should_pause = false;
                bool should_reinitialize_audio_path = false;

                {
                    std::unique_lock lock(state_mutex_);
                    command_condition_variable_.wait_for(
                        lock,
                        std::chrono::milliseconds(8),
                        [this]() {
                            return shutting_down_ || device_reinitialization_requested_ ||
                                   requested_source_generation_ != active_source_generation_ ||
                                   pending_seek_ms_.has_value() || transport_state_changed_locked();
                        });

                    if (shutting_down_) {
                        break;
                    }

                    if (device_reinitialization_requested_) {
                        should_reinitialize_audio_path = true;
                        device_reinitialization_requested_ = false;
                    }

                    if (requested_source_generation_ != active_source_generation_) {
                        load_generation = requested_source_generation_;
                    }

                    if (pending_seek_ms_.has_value()) {
                        seek_ms = pending_seek_ms_;
                        pending_seek_ms_.reset();
                    }

                    should_play = playback_intent_playing_;
                    should_pause = !playback_intent_playing_;
                }

                if (should_reinitialize_audio_path) {
                    initialize_audio_path(context);
                }

                if (load_generation.has_value()) {
                    load_source(context, *load_generation);
                }

                if (seek_ms.has_value()) {
                    perform_seek(context, *seek_ms);
                }

                if (should_pause) {
                    pause_rendering(context);
                }

                if (should_play) {
                    start_rendering(context);
                }

                if (context.source_ready && should_play) {
                    render_audio(context);
                }
            }
        } catch (const std::exception& exception) {
            std::scoped_lock lock(state_mutex_);
            playback_state_.status = playback_status::error;
            playback_state_.error_message = exception.what();
        }

        teardown_context(context);

        if (mmcss_handle != nullptr) {
            AvRevertMmThreadCharacteristics(mmcss_handle);
        }

        if (worker_should_uninitialize_com) {
            CoUninitialize();
        }
    }

    [[nodiscard]] bool transport_state_changed_locked() const {
        if (playback_intent_playing_) {
            return playback_state_.status == playback_status::loading ||
                   playback_state_.status == playback_status::paused;
        }

        return playback_state_.status == playback_status::playing ||
               playback_state_.status == playback_status::loading;
    }

    void initialize_audio_path(worker_context& context) {
        context.render_client.reset();
        context.audio_client.reset();
        context.render_device.reset();

        com_ptr<IMMDeviceEnumerator> device_enumerator;
        throw_if_failed(
            CoCreateInstance(
                __uuidof(MMDeviceEnumerator),
                nullptr,
                CLSCTX_ALL,
                __uuidof(IMMDeviceEnumerator),
                reinterpret_cast<void**>(device_enumerator.put())),
            "Создание MMDeviceEnumerator");
        throw_if_failed(
            device_enumerator.get()->GetDefaultAudioEndpoint(eRender, eMultimedia, context.render_device.put()),
            "Получение default render device");
        throw_if_failed(
            context.render_device.get()->Activate(
                __uuidof(IAudioClient),
                CLSCTX_ALL,
                nullptr,
                reinterpret_cast<void**>(context.audio_client.put())),
            "Активация IAudioClient");

        WAVEFORMATEX* mix_format = nullptr;
        throw_if_failed(
            context.audio_client.get()->GetMixFormat(&mix_format),
            "Получение mix format аудио-устройства");
        context.render_format = describe_wave_format(mix_format);

        throw_if_failed(
            context.audio_client.get()->Initialize(
                AUDCLNT_SHAREMODE_SHARED,
                AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                1000000,
                0,
                context.render_format.wave_format.get(),
                nullptr),
            "Инициализация WASAPI shared render client");
        throw_if_failed(
            context.audio_client.get()->GetService(
                __uuidof(IAudioRenderClient),
                reinterpret_cast<void**>(context.render_client.put())),
            "Получение IAudioRenderClient");
        throw_if_failed(
            context.audio_client.get()->GetBufferSize(&context.buffer_frame_count),
            "Получение размера render buffer");

        context.equalizer_chain.configure(
            static_cast<float>(context.render_format.sample_rate),
            context.render_format.channel_count);

        {
            std::scoped_lock lock(state_mutex_);
            apply_equalizer_targets_to_chain_locked(context.equalizer_chain);
            equalizer_state_.status = equalizer_status::ready;
            equalizer_state_.error_message.clear();
        }
    }

    void load_source(worker_context& context, const std::uint64_t generation) {
        if (context.audio_client_started) {
            context.audio_client.get()->Stop();
            context.audio_client_started = false;
        }
        context.source_reader.reset();
        context.decoded_samples.clear();
        context.decoded_sample_offset = 0;
        context.next_sample_time_100ns = 0;
        context.duration_100ns = 0;
        context.source_ready = false;
        context.end_of_stream_reached = false;
        context.equalizer_chain.reset();

        std::string stream_url;
        {
            std::scoped_lock lock(state_mutex_);
            stream_url = requested_stream_url_;
        }

        if (context.audio_client.get() == nullptr) {
            initialize_audio_path(context);
        }

        try {
            context.source_reader = create_source_reader(stream_url, context.render_format);
            context.duration_100ns = query_source_duration_100ns(context.source_reader.get());
            context.source_ready = true;

            {
                std::scoped_lock lock(state_mutex_);
                active_source_generation_ = generation;
                current_stream_url_ = stream_url;
                source_ready_ = true;
                playback_state_.stream_url = current_stream_url_;
                playback_state_.duration_ms = convert_100ns_to_milliseconds(context.duration_100ns);
                playback_state_.position_ms = 0;
                playback_state_.error_message.clear();
                playback_state_.status =
                    playback_intent_playing_ ? playback_status::loading : playback_status::paused;
            }
        } catch (const std::exception& exception) {
            std::scoped_lock lock(state_mutex_);
            active_source_generation_ = generation;
            source_ready_ = false;
            playback_state_.status = playback_status::error;
            playback_state_.error_message = exception.what();
            current_stream_url_.clear();
        }
    }

    void perform_seek(worker_context& context, const std::int64_t position_ms) {
        if (!context.source_ready || context.source_reader.get() == nullptr) {
            return;
        }

        reset_audio_client(context);

        PROPVARIANT position_value;
        PropVariantInit(&position_value);
        position_value.vt = VT_I8;
        position_value.hVal.QuadPart = position_ms * 10000;
        throw_if_failed(
            context.source_reader.get()->SetCurrentPosition(GUID_NULL, position_value),
            "Перемотка source reader");
        PropVariantClear(&position_value);

        context.decoded_samples.clear();
        context.decoded_sample_offset = 0;
        context.next_sample_time_100ns = position_ms * 10000;
        context.end_of_stream_reached = false;
        context.equalizer_chain.reset();

        std::scoped_lock lock(state_mutex_);
        playback_state_.position_ms = position_ms;
        playback_state_.status = playback_intent_playing_ ? playback_status::loading : playback_status::paused;
        playback_state_.error_message.clear();
    }

    void start_rendering(worker_context& context) {
        if (!context.source_ready || context.audio_client.get() == nullptr) {
            return;
        }

        apply_runtime_targets_to_chain_locked(context.equalizer_chain);
        if (!context.audio_client_started) {
            throw_if_failed(context.audio_client.get()->Start(), "Запуск WASAPI render client");
            context.audio_client_started = true;
        }

        std::scoped_lock lock(state_mutex_);
        playback_state_.status = playback_status::playing;
        playback_state_.error_message.clear();
    }

    void pause_rendering(worker_context& context) {
        if (!context.audio_client_started || context.audio_client.get() == nullptr) {
            return;
        }

        context.audio_client.get()->Stop();
        context.audio_client_started = false;

        std::scoped_lock lock(state_mutex_);
        playback_state_.status = playback_status::paused;
        playback_state_.error_message.clear();
    }

    void render_audio(worker_context& context) {
        if (context.audio_client.get() == nullptr || context.render_client.get() == nullptr) {
            return;
        }

        UINT32 padding = 0;
        const HRESULT padding_result = context.audio_client.get()->GetCurrentPadding(&padding);
        if (padding_result == AUDCLNT_E_DEVICE_INVALIDATED) {
            handle_device_invalidation();
            return;
        }
        throw_if_failed(padding_result, "Получение current padding WASAPI");

        if (padding >= context.buffer_frame_count) {
            return;
        }

        const UINT32 available_frames = context.buffer_frame_count - padding;
        ensure_decoded_frames(context, available_frames);

        if (available_frames == 0) {
            return;
        }

        BYTE* destination_buffer = nullptr;
        const HRESULT get_buffer_result =
            context.render_client.get()->GetBuffer(available_frames, &destination_buffer);
        if (get_buffer_result == AUDCLNT_E_DEVICE_INVALIDATED) {
            handle_device_invalidation();
            return;
        }
        throw_if_failed(get_buffer_result, "Получение render buffer WASAPI");

        std::vector<float> render_samples(
            static_cast<std::size_t>(available_frames) * context.render_format.channel_count,
            0.0F);
        const std::size_t available_sample_count =
            context.decoded_samples.size() - context.decoded_sample_offset;
        const std::size_t requested_sample_count =
            static_cast<std::size_t>(available_frames) * context.render_format.channel_count;
        const std::size_t copied_sample_count = (std::min)(requested_sample_count, available_sample_count);

        if (copied_sample_count > 0) {
            std::copy_n(
                context.decoded_samples.data() + context.decoded_sample_offset,
                copied_sample_count,
                render_samples.data());
            context.decoded_sample_offset += copied_sample_count;
        }

        if (context.decoded_sample_offset >= context.decoded_samples.size()) {
            context.decoded_samples.clear();
            context.decoded_sample_offset = 0;
        }

        apply_runtime_targets_to_chain_locked(context.equalizer_chain);
        context.equalizer_chain.process(render_samples.data(), available_frames);
        convert_float_to_render_format(
            render_samples.data(),
            available_frames,
            context.render_format,
            destination_buffer);

        const HRESULT release_result = context.render_client.get()->ReleaseBuffer(available_frames, 0);
        if (release_result == AUDCLNT_E_DEVICE_INVALIDATED) {
            handle_device_invalidation();
            return;
        }
        throw_if_failed(release_result, "Освобождение render buffer WASAPI");

        context.next_sample_time_100ns +=
            static_cast<std::int64_t>(available_frames) * 10000000LL /
            static_cast<std::int64_t>(context.render_format.sample_rate);

        {
            std::scoped_lock lock(state_mutex_);
            playback_state_.position_ms = convert_100ns_to_milliseconds(context.next_sample_time_100ns);
            playback_state_.duration_ms = convert_100ns_to_milliseconds(context.duration_100ns);
            playback_state_.status = playback_intent_playing_ ? playback_status::playing : playback_status::paused;
            playback_state_.error_message.clear();
        }

        if (context.end_of_stream_reached && context.decoded_samples.empty()) {
            reset_audio_client(context);
            std::scoped_lock lock(state_mutex_);
            playback_intent_playing_ = false;
            playback_state_.status = playback_status::idle;
            playback_state_.position_ms = playback_state_.duration_ms;
            ++playback_state_.completion_token;
        }
    }

    void ensure_decoded_frames(worker_context& context, const std::size_t required_frames) {
        const std::size_t required_sample_count =
            required_frames * context.render_format.channel_count;
        const std::size_t available_sample_count =
            context.decoded_samples.size() - context.decoded_sample_offset;
        if (available_sample_count >= required_sample_count || context.end_of_stream_reached) {
            return;
        }

        while ((context.decoded_samples.size() - context.decoded_sample_offset) < required_sample_count &&
               !context.end_of_stream_reached) {
            DWORD stream_index = 0;
            DWORD stream_flags = 0;
            LONGLONG sample_time = 0;
            com_ptr<IMFSample> sample;
            throw_if_failed(
                context.source_reader.get()->ReadSample(
                    static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
                    0,
                    &stream_index,
                    &stream_flags,
                    &sample_time,
                    sample.put()),
                "Чтение PCM sample из source reader");

            if ((stream_flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0U) {
                context.end_of_stream_reached = true;
                break;
            }

            if (sample.get() == nullptr) {
                continue;
            }

            if (context.decoded_samples.empty()) {
                context.next_sample_time_100ns = sample_time;
            }

            com_ptr<IMFMediaBuffer> media_buffer;
            throw_if_failed(
                sample.get()->ConvertToContiguousBuffer(media_buffer.put()),
                "Преобразование sample в contiguous buffer");

            BYTE* sample_bytes = nullptr;
            DWORD max_length = 0;
            DWORD current_length = 0;
            throw_if_failed(
                media_buffer.get()->Lock(&sample_bytes, &max_length, &current_length),
                "Lock PCM media buffer");

            const std::size_t float_count = current_length / sizeof(float);
            const float* sample_floats = reinterpret_cast<const float*>(sample_bytes);
            context.decoded_samples.insert(
                context.decoded_samples.end(),
                sample_floats,
                sample_floats + float_count);

            media_buffer.get()->Unlock();
        }
    }

    void teardown_context(worker_context& context) {
        if (context.audio_client_started && context.audio_client.get() != nullptr) {
            context.audio_client.get()->Stop();
            context.audio_client_started = false;
        }

        context.render_client.reset();
        context.audio_client.reset();
        context.render_device.reset();
        context.source_reader.reset();
    }

    void handle_device_invalidation() {
        std::scoped_lock lock(state_mutex_);
        source_ready_ = false;
        device_reinitialization_requested_ = true;
        playback_state_.status = playback_status::loading;
        equalizer_state_.status = equalizer_status::audio_engine_unavailable;
        equalizer_state_.error_message = "Render device был потерян, пытаемся восстановить audio path.";
    }

    void populate_default_equalizer_state_locked() {
        equalizer_state_.available_presets = available_presets_;
        equalizer_state_.status = equalizer_status::loading;
        for (std::size_t index = 0; index < equalizer_state_.bands.size(); ++index) {
            equalizer_state_.bands[index] = equalizer_band{
                .center_frequency_hz = dsp::equalizer_chain::band_frequencies_hz()[index],
                .gain_db = 0.0F,
            };
            equalizer_state_.last_nonflat_band_gains_db[index] = 0.0F;
        }
        equalizer_state_.output_gain_db = 0.0F;
    }

    void recalculate_active_preset_locked() {
        for (const equalizer_preset& preset : available_presets_) {
            if (preset.id == equalizer_preset_id::custom) {
                continue;
            }

            if (band_gains_match(equalizer_state_.bands, preset.gains_db)) {
                equalizer_state_.active_preset_id = preset.id;
                return;
            }
        }

        equalizer_state_.active_preset_id = equalizer_preset_id::custom;
    }

    void update_last_nonflat_state_locked() {
        bool is_flat = true;
        for (const equalizer_band& band : equalizer_state_.bands) {
            if (std::fabs(band.gain_db) > 0.01F) {
                is_flat = false;
                break;
            }
        }

        if (is_flat) {
            return;
        }

        for (std::size_t index = 0; index < equalizer_state_.bands.size(); ++index) {
            equalizer_state_.last_nonflat_band_gains_db[index] = equalizer_state_.bands[index].gain_db;
        }
    }

    void recompute_equalizer_metadata_locked(const float sample_rate) {
        std::array<float, 10> band_gains_db{};
        for (std::size_t index = 0; index < equalizer_state_.bands.size(); ++index) {
            band_gains_db[index] = equalizer_state_.bands[index].gain_db;
        }

        equalizer_state_.headroom_compensation_db =
            headroom_controller_.compute_target_preamp_db(band_gains_db, sample_rate);
        if (equalizer_state_.status != equalizer_status::audio_engine_unavailable &&
            equalizer_state_.status != equalizer_status::unsupported_audio_path) {
            equalizer_state_.status = equalizer_status::ready;
            equalizer_state_.error_message.clear();
        }
    }

    [[nodiscard]] float current_sample_rate_or_default_locked() const {
        return last_known_sample_rate_ > 0.0F ? last_known_sample_rate_ : 48000.0F;
    }

    void apply_equalizer_targets_to_chain_locked(dsp::equalizer_chain& equalizer_chain) {
        std::array<float, 10> band_gains_db{};
        for (std::size_t index = 0; index < equalizer_state_.bands.size(); ++index) {
            band_gains_db[index] = equalizer_state_.bands[index].gain_db;
        }
        equalizer_chain.set_band_gains(band_gains_db);
        equalizer_chain.set_enabled(equalizer_state_.enabled);
        equalizer_chain.set_output_gain_db(equalizer_state_.output_gain_db);
        equalizer_chain.set_volume_linear(convert_volume_percent_to_linear(playback_state_.volume_percent));
    }

    void apply_runtime_targets_to_chain_locked(dsp::equalizer_chain& equalizer_chain) {
        std::scoped_lock lock(state_mutex_);
        last_known_sample_rate_ = equalizer_chain.sample_rate();
        apply_equalizer_targets_to_chain_locked(equalizer_chain);
    }

    mutable std::mutex state_mutex_;
    std::condition_variable command_condition_variable_;
    bool shutting_down_ = false;
    bool media_foundation_started_ = false;
    bool playback_intent_playing_ = false;
    bool source_ready_ = false;
    bool device_reinitialization_requested_ = false;
    std::uint64_t requested_source_generation_ = 0;
    std::uint64_t active_source_generation_ = 0;
    std::optional<std::int64_t> pending_seek_ms_;
    std::string requested_stream_url_;
    std::string current_stream_url_;
    playback_state playback_state_{};
    equalizer_state equalizer_state_{};
    std::vector<equalizer_preset> available_presets_;
    dsp::output_headroom_controller headroom_controller_;
    float last_known_sample_rate_ = 0.0F;
    std::thread worker_thread_;
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
