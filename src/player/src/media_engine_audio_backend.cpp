#include "media_engine_audio_backend.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfmediaengine.h>
#include <oleauto.h>

namespace soundcloud::player {
namespace {

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

class bstr_ptr {
public:
    explicit bstr_ptr(const std::wstring& value) {
        value_ = SysAllocStringLen(value.c_str(), static_cast<UINT>(value.size()));
        if (value_ == nullptr && !value.empty()) {
            throw std::bad_alloc();
        }
    }

    ~bstr_ptr() {
        if (value_ != nullptr) {
            SysFreeString(value_);
        }
    }

    [[nodiscard]] BSTR get() const {
        return value_;
    }

private:
    BSTR value_ = nullptr;
};

std::int64_t seconds_to_milliseconds(const double seconds) {
    if (!std::isfinite(seconds) || seconds < 0.0) {
        return 0;
    }

    return static_cast<std::int64_t>(seconds * 1000.0);
}

std::string describe_media_engine_error(const USHORT error_code) {
    switch (error_code) {
        case MF_MEDIA_ENGINE_ERR_ABORTED:
            return "Воспроизведение было прервано Media Engine.";
        case MF_MEDIA_ENGINE_ERR_NETWORK:
            return "Media Engine не смог загрузить аудиопоток из сети.";
        case MF_MEDIA_ENGINE_ERR_DECODE:
            return "Media Engine не смог декодировать аудиопоток.";
        case MF_MEDIA_ENGINE_ERR_SRC_NOT_SUPPORTED:
            return "Media Engine не поддерживает источник аудио для этого трека.";
        case MF_MEDIA_ENGINE_ERR_ENCRYPTED:
            return "Media Engine запросил защищённый контент, который приложение не поддерживает.";
        default:
            return "Media Engine вернул неизвестную ошибку воспроизведения.";
    }
}

class media_engine_notify final : public IMFMediaEngineNotify {
public:
    explicit media_engine_notify(class media_engine_audio_backend::implementation& owner)
        : owner_(owner) {}

    ULONG STDMETHODCALLTYPE AddRef() override {
        return ++reference_count_;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG reference_count = --reference_count_;
        if (reference_count == 0) {
            delete this;
        }

        return reference_count;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iid, void** object) override {
        if (object == nullptr) {
            return E_POINTER;
        }

        if (iid == __uuidof(IUnknown) || iid == __uuidof(IMFMediaEngineNotify)) {
            *object = static_cast<IMFMediaEngineNotify*>(this);
            AddRef();
            return S_OK;
        }

        *object = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE EventNotify(
        const DWORD event,
        const DWORD_PTR param1,
        const DWORD param2) override;

private:
    std::atomic<ULONG> reference_count_{1};
    class media_engine_audio_backend::implementation& owner_;
};

}  // namespace

class media_engine_audio_backend::implementation {
public:
    implementation() {
        const HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(com_result)) {
            should_uninitialize_com_ = true;
        } else if (com_result != RPC_E_CHANGED_MODE) {
            throw make_hresult_error("Инициализация COM для Media Engine", com_result);
        }

        throw_if_failed(MFStartup(MF_VERSION), "Запуск Media Foundation");
        media_foundation_started_ = true;

        throw_if_failed(
            CoCreateInstance(
                CLSID_MFMediaEngineClassFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(class_factory_.put())),
            "Создание Media Engine class factory");

        notify_ = new media_engine_notify(*this);
        recreate_media_engine();
    }

    ~implementation() {
        shutdown_media_engine();

        if (notify_ != nullptr) {
            notify_->Release();
            notify_ = nullptr;
        }

        class_factory_.reset();

        if (media_foundation_started_) {
            MFShutdown();
        }

        if (should_uninitialize_com_) {
            CoUninitialize();
        }
    }

    void load(const std::string& stream_url) {
        if (stream_url.empty()) {
            throw std::invalid_argument("Не передан stream_url для воспроизведения.");
        }

        const std::wstring wide_stream_url = utf8_to_utf16(stream_url);
        const bstr_ptr source_url(wide_stream_url);
        recreate_media_engine();

        {
            std::scoped_lock lock(state_mutex_);
            pending_stream_url_ = stream_url;
            current_stream_url_.clear();
            media_ready_ = false;
            should_start_playback_when_ready_ = false;
            playback_state_.status = core::domain::playback_status::loading;
            playback_state_.stream_url = stream_url;
            playback_state_.error_message.clear();
            playback_state_.position_ms = 0;
            playback_state_.duration_ms = 0;
        }

        try {
            throw_if_failed(media_engine_.get()->SetSource(source_url.get()), "Назначение media source");
            throw_if_failed(media_engine_.get()->Load(), "Запуск загрузки media source");
        } catch (const std::exception& exception) {
            std::scoped_lock lock(state_mutex_);
            fail_request_locked(exception.what());
            throw;
        }
    }

    void play() {
        {
            std::scoped_lock lock(state_mutex_);
            if (!pending_stream_url_.empty() && !media_ready_) {
                should_start_playback_when_ready_ = true;
                playback_state_.status = core::domain::playback_status::loading;
                return;
            }

            if (current_stream_url_.empty()) {
                playback_state_.status = core::domain::playback_status::idle;
                playback_state_.error_message.clear();
                return;
            }
        }

        try {
            throw_if_failed(media_engine_.get()->Play(), "Запуск воспроизведения");
            std::scoped_lock lock(state_mutex_);
            playback_state_.status = core::domain::playback_status::loading;
            playback_state_.error_message.clear();
        } catch (const std::exception& exception) {
            std::scoped_lock lock(state_mutex_);
            fail_request_locked(exception.what());
            throw;
        }
    }

    void pause() {
        bool should_pause_engine = false;
        {
            std::scoped_lock lock(state_mutex_);
            should_start_playback_when_ready_ = false;

            if (!pending_stream_url_.empty() && !media_ready_) {
                playback_state_.status = core::domain::playback_status::paused;
                playback_state_.error_message.clear();
                return;
            }

            if (current_stream_url_.empty() || !media_ready_) {
                return;
            }

            should_pause_engine = true;
        }

        if (!should_pause_engine) {
            return;
        }

        throw_if_failed(media_engine_.get()->Pause(), "Пауза воспроизведения");
        std::scoped_lock lock(state_mutex_);
        playback_state_.status = core::domain::playback_status::paused;
        playback_state_.error_message.clear();
    }

    void seek_to(const std::int64_t position_ms) {
        {
            std::scoped_lock lock(state_mutex_);
            if (current_stream_url_.empty() || !media_ready_) {
                throw std::runtime_error("Нельзя перемотать трек, пока плеер не загрузил аудиопоток.");
            }
        }

        throw_if_failed(
            media_engine_.get()->SetCurrentTime(static_cast<double>((std::max)(position_ms, static_cast<std::int64_t>(0))) / 1000.0),
            "Перемотка воспроизведения");

        std::scoped_lock lock(state_mutex_);
        playback_state_.position_ms = position_ms;
        playback_state_.error_message.clear();
    }

    [[nodiscard]] core::domain::playback_state get_playback_state() const {
        std::scoped_lock lock(state_mutex_);
        core::domain::playback_state playback_state = playback_state_;

        if (media_engine_.get() != nullptr) {
            playback_state.position_ms =
                seconds_to_milliseconds(media_engine_.get()->GetCurrentTime());
            playback_state.duration_ms =
                seconds_to_milliseconds(media_engine_.get()->GetDuration());
        }

        return playback_state;
    }

    void on_media_engine_event(const DWORD event, const DWORD_PTR param1, const DWORD param2) {
        (void)param1;
        (void)param2;

        try {
            switch (event) {
                case MF_MEDIA_ENGINE_EVENT_LOADSTART:
                case MF_MEDIA_ENGINE_EVENT_PROGRESS:
                case MF_MEDIA_ENGINE_EVENT_WAITING:
                case MF_MEDIA_ENGINE_EVENT_BUFFERINGSTARTED:
                case MF_MEDIA_ENGINE_EVENT_STALLED:
                    update_loading_status();
                    break;
                case MF_MEDIA_ENGINE_EVENT_CANPLAY:
                case MF_MEDIA_ENGINE_EVENT_CANPLAYTHROUGH:
                case MF_MEDIA_ENGINE_EVENT_LOADEDDATA:
                case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:
                    handle_media_ready();
                    break;
                case MF_MEDIA_ENGINE_EVENT_PLAYING:
                    handle_playing();
                    break;
                case MF_MEDIA_ENGINE_EVENT_PAUSE:
                    handle_paused();
                    break;
                case MF_MEDIA_ENGINE_EVENT_ENDED:
                    handle_ended();
                    break;
                case MF_MEDIA_ENGINE_EVENT_ERROR:
                case MF_MEDIA_ENGINE_EVENT_ABORT:
                case MF_MEDIA_ENGINE_EVENT_EMPTIED:
                    handle_media_error();
                    break;
                default:
                    break;
            }
        } catch (const std::exception& exception) {
            std::scoped_lock lock(state_mutex_);
            fail_request_locked(exception.what());
        }
    }

private:
    void recreate_media_engine() {
        shutdown_media_engine();

        com_ptr<IMFAttributes> attributes;
        throw_if_failed(MFCreateAttributes(attributes.put(), 1), "Создание attributes для Media Engine");
        throw_if_failed(
            attributes.get()->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, notify_),
            "Регистрация callback для Media Engine");

        constexpr DWORD create_flags =
            MF_MEDIA_ENGINE_AUDIOONLY | MF_MEDIA_ENGINE_WAITFORSTABLE_STATE;
        throw_if_failed(
            class_factory_.get()->CreateInstance(create_flags, attributes.get(), media_engine_.put()),
            "Создание Media Engine instance");
    }

    void shutdown_media_engine() {
        if (media_engine_.get() != nullptr) {
            media_engine_.get()->Shutdown();
            media_engine_.reset();
        }
    }

    void update_loading_status() {
        std::scoped_lock lock(state_mutex_);
        if (playback_state_.status == core::domain::playback_status::error) {
            return;
        }

        if (!pending_stream_url_.empty() || should_start_playback_when_ready_) {
            playback_state_.status = core::domain::playback_status::loading;
        }
    }

    [[nodiscard]] bool has_active_source_locked() const {
        return !pending_stream_url_.empty() || !current_stream_url_.empty();
    }

    void handle_media_ready() {
        bool should_play = false;
        {
            std::scoped_lock lock(state_mutex_);
            if (pending_stream_url_.empty()) {
                return;
            }

            current_stream_url_ = pending_stream_url_;
            pending_stream_url_.clear();
            media_ready_ = true;
            playback_state_.stream_url = current_stream_url_;
            playback_state_.duration_ms =
                seconds_to_milliseconds(media_engine_.get()->GetDuration());
            should_play = should_start_playback_when_ready_;

            if (!should_play) {
                playback_state_.status = core::domain::playback_status::paused;
                playback_state_.error_message.clear();
            }
        }

        if (!should_play) {
            return;
        }

        throw_if_failed(media_engine_.get()->Play(), "Автоматический старт воспроизведения");
        std::scoped_lock lock(state_mutex_);
        playback_state_.status = core::domain::playback_status::loading;
        playback_state_.error_message.clear();
    }

    void handle_playing() {
        std::scoped_lock lock(state_mutex_);
        playback_state_.status = core::domain::playback_status::playing;
        playback_state_.error_message.clear();
    }

    void handle_paused() {
        std::scoped_lock lock(state_mutex_);
        if (playback_state_.status == core::domain::playback_status::error) {
            return;
        }

        if (!has_active_source_locked()) {
            playback_state_.status = core::domain::playback_status::idle;
            playback_state_.error_message.clear();
            return;
        }

        playback_state_.status = core::domain::playback_status::paused;
        playback_state_.error_message.clear();
    }

    void handle_ended() {
        std::scoped_lock lock(state_mutex_);
        playback_state_.status = core::domain::playback_status::idle;
        playback_state_.position_ms = playback_state_.duration_ms;
        should_start_playback_when_ready_ = false;
        ++playback_state_.completion_token;
    }

    void handle_media_error() {
        {
            std::scoped_lock lock(state_mutex_);
            if (!has_active_source_locked()) {
                playback_state_.status = core::domain::playback_status::idle;
                playback_state_.error_message.clear();
                return;
            }
        }

        com_ptr<IMFMediaError> media_error;
        std::string error_message = "Media Engine завершил воспроизведение с неизвестной ошибкой.";

        if (media_engine_.get() != nullptr &&
            SUCCEEDED(media_engine_.get()->GetError(media_error.put())) &&
            media_error.get() != nullptr) {
            const USHORT error_code = media_error.get()->GetErrorCode();
            const HRESULT extended_error_code = media_error.get()->GetExtendedErrorCode();

            std::ostringstream message;
            message << describe_media_engine_error(error_code);
            if (extended_error_code != S_OK) {
                message << " HRESULT 0x" << std::hex
                        << static_cast<unsigned long>(extended_error_code) << std::dec;
            }

            error_message = message.str();
        }

        std::scoped_lock lock(state_mutex_);
        fail_request_locked(error_message);
    }

    void fail_request_locked(const std::string& error_message) {
        pending_stream_url_.clear();
        current_stream_url_.clear();
        media_ready_ = false;
        should_start_playback_when_ready_ = false;
        playback_state_.status = core::domain::playback_status::error;
        playback_state_.stream_url.clear();
        playback_state_.position_ms = 0;
        playback_state_.duration_ms = 0;
        playback_state_.error_message = error_message;
    }

    mutable std::mutex state_mutex_;
    com_ptr<IMFMediaEngineClassFactory> class_factory_;
    com_ptr<IMFMediaEngine> media_engine_;
    media_engine_notify* notify_ = nullptr;
    std::string pending_stream_url_;
    std::string current_stream_url_;
    core::domain::playback_state playback_state_{};
    bool should_start_playback_when_ready_ = false;
    bool media_ready_ = false;
    bool should_uninitialize_com_ = false;
    bool media_foundation_started_ = false;
};

HRESULT media_engine_notify::EventNotify(
    const DWORD event,
    const DWORD_PTR param1,
    const DWORD param2) {
    owner_.on_media_engine_event(event, param1, param2);
    return S_OK;
}

media_engine_audio_backend::media_engine_audio_backend()
    : implementation_(std::make_unique<implementation>()) {}

media_engine_audio_backend::~media_engine_audio_backend() = default;

void media_engine_audio_backend::load(const std::string& stream_url) {
    implementation_->load(stream_url);
}

void media_engine_audio_backend::play() {
    implementation_->play();
}

void media_engine_audio_backend::pause() {
    implementation_->pause();
}

void media_engine_audio_backend::seek_to(const std::int64_t position_ms) {
    implementation_->seek_to(position_ms);
}

core::domain::playback_state media_engine_audio_backend::get_playback_state() const {
    return implementation_->get_playback_state();
}

}  // namespace soundcloud::player
