#include "media_foundation_audio_backend.h"

#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfplay.h>

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

class media_player_callback final : public IMFPMediaPlayerCallback {
public:
    explicit media_player_callback(class media_foundation_audio_backend::implementation& owner)
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

        if (iid == __uuidof(IUnknown) || iid == __uuidof(IMFPMediaPlayerCallback)) {
            *object = static_cast<IMFPMediaPlayerCallback*>(this);
            AddRef();
            return S_OK;
        }

        *object = nullptr;
        return E_NOINTERFACE;
    }

    void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER* event_header) override;

private:
    std::atomic<ULONG> reference_count_{1};
    class media_foundation_audio_backend::implementation& owner_;
};

}  // namespace

class media_foundation_audio_backend::implementation {
public:
    implementation() {
        const HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(com_result)) {
            should_uninitialize_com_ = true;
        } else if (com_result != RPC_E_CHANGED_MODE) {
            throw make_hresult_error("Инициализация COM для плеера", com_result);
        }

        throw_if_failed(MFStartup(MF_VERSION), "Запуск Media Foundation");
        media_foundation_started_ = true;

        callback_ = new media_player_callback(*this);
        const HRESULT create_player_result = MFPCreateMediaPlayer(
            nullptr,
            FALSE,
            MFP_OPTION_NONE,
            callback_,
            nullptr,
            media_player_.put());
        throw_if_failed(create_player_result, "Создание Media Foundation player");
    }

    ~implementation() {
        media_player_.reset();

        if (callback_ != nullptr) {
            callback_->Release();
            callback_ = nullptr;
        }

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

        std::scoped_lock lock(state_mutex_);
        pending_stream_url_ = stream_url;
        current_stream_url_.clear();
        media_item_ready_ = false;
        should_start_playback_when_ready_ = false;
        playback_state_.status = core::domain::playback_status::loading;
        playback_state_.stream_url = stream_url;
        playback_state_.error_message.clear();

        const HRESULT create_item_result = media_player_.get()->CreateMediaItemFromURL(
            wide_stream_url.c_str(),
            FALSE,
            0,
            nullptr);
        throw_if_failed(create_item_result, "Подготовка media item из stream URL");
    }

    void play() {
        std::scoped_lock lock(state_mutex_);

        if (!pending_stream_url_.empty() && !media_item_ready_) {
            should_start_playback_when_ready_ = true;
            playback_state_.status = core::domain::playback_status::loading;
            return;
        }

        if (current_stream_url_.empty()) {
            playback_state_.status = core::domain::playback_status::error;
            playback_state_.error_message = "Плеер ещё не загрузил аудиопоток.";
            throw std::runtime_error("Плеер ещё не загрузил аудиопоток.");
        }

        throw_if_failed(media_player_.get()->Play(), "Запуск воспроизведения");
        playback_state_.status = core::domain::playback_status::playing;
        playback_state_.stream_url = current_stream_url_;
        playback_state_.error_message.clear();
    }

    void pause() {
        std::scoped_lock lock(state_mutex_);
        should_start_playback_when_ready_ = false;

        if (current_stream_url_.empty() || !media_item_ready_) {
            return;
        }

        throw_if_failed(media_player_.get()->Pause(), "Пауза воспроизведения");
        playback_state_.status = core::domain::playback_status::paused;
        playback_state_.stream_url = current_stream_url_;
        playback_state_.error_message.clear();
    }

    [[nodiscard]] core::domain::playback_state get_playback_state() const {
        std::scoped_lock lock(state_mutex_);
        return playback_state_;
    }

    void on_media_player_event(MFP_EVENT_HEADER* event_header) {
        if (event_header == nullptr) {
            return;
        }

        try {
            switch (event_header->eEventType) {
                case MFP_EVENT_TYPE_MEDIAITEM_CREATED:
                    handle_media_item_created(
                        reinterpret_cast<MFP_MEDIAITEM_CREATED_EVENT*>(event_header));
                    break;
                case MFP_EVENT_TYPE_MEDIAITEM_SET:
                    handle_media_item_set(
                        reinterpret_cast<MFP_MEDIAITEM_SET_EVENT*>(event_header));
                    break;
                default:
                    break;
            }
        } catch (const std::exception& exception) {
            std::scoped_lock lock(state_mutex_);
            playback_state_.status = core::domain::playback_status::error;
            last_error_message_ = exception.what();
            playback_state_.error_message = last_error_message_;
        }
    }

private:
    void handle_media_item_created(MFP_MEDIAITEM_CREATED_EVENT* event_data) {
        if (event_data == nullptr) {
            return;
        }

        throw_if_failed(event_data->header.hrEvent, "Создание media item");
        throw_if_failed(
            media_player_.get()->SetMediaItem(event_data->pMediaItem),
            "Установка media item в player");
    }

    void handle_media_item_set(MFP_MEDIAITEM_SET_EVENT* event_data) {
        if (event_data == nullptr) {
            return;
        }

        throw_if_failed(event_data->header.hrEvent, "Применение media item");

        bool should_play = false;
        {
            std::scoped_lock lock(state_mutex_);
            current_stream_url_ = pending_stream_url_;
            pending_stream_url_.clear();
            media_item_ready_ = true;
            playback_state_.stream_url = current_stream_url_;
            should_play = should_start_playback_when_ready_;
        }

        if (should_play) {
            throw_if_failed(media_player_.get()->Play(), "Автоматический старт воспроизведения");
            std::scoped_lock lock(state_mutex_);
            playback_state_.status = core::domain::playback_status::playing;
            playback_state_.error_message.clear();
        } else {
            std::scoped_lock lock(state_mutex_);
            playback_state_.status = core::domain::playback_status::paused;
            playback_state_.error_message.clear();
        }
    }

    mutable std::mutex state_mutex_;
    com_ptr<IMFPMediaPlayer> media_player_;
    media_player_callback* callback_ = nullptr;
    std::string pending_stream_url_;
    std::string current_stream_url_;
    std::string last_error_message_;
    core::domain::playback_state playback_state_{};
    bool should_start_playback_when_ready_ = false;
    bool media_item_ready_ = false;
    bool should_uninitialize_com_ = false;
    bool media_foundation_started_ = false;
};

void media_player_callback::OnMediaPlayerEvent(MFP_EVENT_HEADER* event_header) {
    owner_.on_media_player_event(event_header);
}

media_foundation_audio_backend::media_foundation_audio_backend()
    : implementation_(std::make_unique<implementation>()) {}

media_foundation_audio_backend::~media_foundation_audio_backend() = default;

void media_foundation_audio_backend::load(const std::string& stream_url) {
    implementation_->load(stream_url);
}

void media_foundation_audio_backend::play() {
    implementation_->play();
}

void media_foundation_audio_backend::pause() {
    implementation_->pause();
}

core::domain::playback_state media_foundation_audio_backend::get_playback_state() const {
    return implementation_->get_playback_state();
}

}  // namespace soundcloud::player
