#include "soundcloud/api/soundcloud_http_client.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#endif

namespace soundcloud::api {
namespace {

std::string url_encode_component(const std::string& value) {
    constexpr std::array<char, 16> hex_digits{
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    std::ostringstream encoded_value;
    for (const unsigned char character : value) {
        if (std::isalnum(character) != 0 ||
            character == '-' ||
            character == '_' ||
            character == '.' ||
            character == '~') {
            encoded_value << static_cast<char>(character);
            continue;
        }

        encoded_value << '%' << hex_digits[(character >> 4U) & 0x0FU] << hex_digits[character & 0x0FU];
    }

    return encoded_value.str();
}

#ifdef _WIN32

class winhttp_handle {
public:
    explicit winhttp_handle(void* native_handle = nullptr) : native_handle_(native_handle) {}

    winhttp_handle(const winhttp_handle&) = delete;
    winhttp_handle& operator=(const winhttp_handle&) = delete;

    winhttp_handle(winhttp_handle&& other) noexcept : native_handle_(other.native_handle_) {
        other.native_handle_ = nullptr;
    }

    winhttp_handle& operator=(winhttp_handle&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        reset();
        native_handle_ = other.native_handle_;
        other.native_handle_ = nullptr;
        return *this;
    }

    ~winhttp_handle() {
        reset();
    }

    [[nodiscard]] HINTERNET get() const {
        return static_cast<HINTERNET>(native_handle_);
    }

private:
    void reset() {
        if (native_handle_ != nullptr) {
            WinHttpCloseHandle(static_cast<HINTERNET>(native_handle_));
            native_handle_ = nullptr;
        }
    }

    void* native_handle_;
};

std::runtime_error make_winhttp_error(const std::string_view operation_name) {
    const DWORD error_code = GetLastError();

    std::ostringstream message;
    message << operation_name << " завершилась ошибкой WinHTTP " << error_code << ": "
            << std::system_category().message(static_cast<int>(error_code));
    return std::runtime_error(message.str());
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
        throw make_winhttp_error("Преобразование UTF-8 в UTF-16");
    }

    std::wstring result(static_cast<std::size_t>(wide_size), L'\0');
    const int converted_size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.c_str(),
        static_cast<int>(value.size()),
        result.data(),
        wide_size);
    if (converted_size != wide_size) {
        throw make_winhttp_error("Преобразование UTF-8 в UTF-16");
    }

    return result;
}

DWORD query_status_code(const winhttp_handle& request_handle) {
    DWORD status_code = 0;
    DWORD status_code_size = sizeof(status_code);

    if (WinHttpQueryHeaders(
            request_handle.get(),
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status_code,
            &status_code_size,
            WINHTTP_NO_HEADER_INDEX) == FALSE) {
        throw make_winhttp_error("Чтение HTTP status code");
    }

    return status_code;
}

std::string read_response_body(const winhttp_handle& request_handle) {
    std::string response_body;

    while (true) {
        DWORD available_bytes = 0;
        if (WinHttpQueryDataAvailable(request_handle.get(), &available_bytes) == FALSE) {
            throw make_winhttp_error("Чтение размера HTTP body");
        }

        if (available_bytes == 0) {
            break;
        }

        std::string chunk(static_cast<std::size_t>(available_bytes), '\0');
        DWORD downloaded_bytes = 0;
        if (WinHttpReadData(
                request_handle.get(),
                chunk.data(),
                available_bytes,
                &downloaded_bytes) == FALSE) {
            throw make_winhttp_error("Чтение HTTP body");
        }

        chunk.resize(static_cast<std::size_t>(downloaded_bytes));
        response_body.append(chunk);
    }

    return response_body;
}

std::string build_http_error_message(const DWORD status_code, const std::string& response_body) {
    constexpr std::size_t max_body_snippet_length = 240;

    std::ostringstream message;
    message << "SoundCloud API вернул HTTP " << status_code;

    if (!response_body.empty()) {
        message << ". Ответ: "
                << response_body.substr(
                       0,
                       (std::min)(max_body_snippet_length, response_body.size()));
    }

    return message.str();
}

#endif

}  // namespace

soundcloud_http_client::soundcloud_http_client(soundcloud_api_configuration configuration)
    : configuration_(std::move(configuration)) {}

std::string soundcloud_http_client::fetch_search_tracks_payload(
    const core::domain::track_search_request& request) const {
    if (configuration_.api_host.empty()) {
        throw std::runtime_error("Не задан host публичного SoundCloud API.");
    }

    if (configuration_.client_id.empty()) {
        throw std::runtime_error("Не задан client_id публичного SoundCloud API.");
    }

    const int normalized_limit =
        request.limit > 0
            ? request.limit
            : (configuration_.default_search_limit > 0 ? configuration_.default_search_limit : 24);
    const int normalized_offset = (std::max)(request.offset, 0);

    std::ostringstream path;
    path << "/search/tracks?q=" << url_encode_component(request.query)
         << "&client_id=" << url_encode_component(configuration_.client_id)
         << "&limit=" << normalized_limit
         << "&offset=" << normalized_offset;

    return perform_get_request(path.str());
}

std::string soundcloud_http_client::fetch_transcoding_payload(
    const std::string& transcoding_url,
    const std::string& track_authorization) const {
    if (transcoding_url.empty()) {
        throw std::invalid_argument("Не передан transcoding URL для playback.");
    }

    if (track_authorization.empty()) {
        throw std::invalid_argument("Не передан track_authorization для playback.");
    }

    const std::string expected_prefix = "https://" + configuration_.api_host;
    if (!transcoding_url.starts_with(expected_prefix)) {
        throw std::runtime_error("Transcoding URL использует неожиданный host.");
    }

    std::ostringstream path;
    path << transcoding_url.substr(expected_prefix.size())
         << "?client_id=" << url_encode_component(configuration_.client_id)
         << "&track_authorization=" << url_encode_component(track_authorization);

    return perform_get_request(path.str());
}

std::string soundcloud_http_client::perform_get_request(const std::string& path_with_query) const {
#ifndef _WIN32
    (void)path_with_query;
    throw std::runtime_error(
        "HTTP-клиент SoundCloud пока реализован только для Windows-сборки.");
#else
    const std::wstring user_agent = L"Lil Music/0.1";
    const std::wstring wide_host = utf8_to_utf16(configuration_.api_host);
    const std::wstring wide_path = utf8_to_utf16(path_with_query);

    winhttp_handle session_handle(WinHttpOpen(
        user_agent.c_str(),
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0));
    if (session_handle.get() == nullptr) {
        throw make_winhttp_error("Создание WinHTTP session");
    }

    DWORD decompression_flags =
        WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;
    if (WinHttpSetOption(
            session_handle.get(),
            WINHTTP_OPTION_DECOMPRESSION,
            &decompression_flags,
            sizeof(decompression_flags)) == FALSE) {
        throw make_winhttp_error("Включение HTTP decompression");
    }

    if (WinHttpSetTimeouts(session_handle.get(), 5000, 5000, 10000, 10000) == FALSE) {
        throw make_winhttp_error("Настройка HTTP timeout");
    }

    winhttp_handle connection_handle(
        WinHttpConnect(session_handle.get(), wide_host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (connection_handle.get() == nullptr) {
        throw make_winhttp_error("Создание WinHTTP connection");
    }

    winhttp_handle request_handle(WinHttpOpenRequest(
        connection_handle.get(),
        L"GET",
        wide_path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE));
    if (request_handle.get() == nullptr) {
        throw make_winhttp_error("Создание HTTP request");
    }

    constexpr wchar_t request_headers[] = L"Accept: application/json\r\n";
    if (WinHttpSendRequest(
            request_handle.get(),
            request_headers,
            static_cast<DWORD>(std::size(request_headers) - 1),
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0) == FALSE) {
        throw make_winhttp_error("Отправка HTTP request");
    }

    if (WinHttpReceiveResponse(request_handle.get(), nullptr) == FALSE) {
        throw make_winhttp_error("Получение HTTP response");
    }

    const DWORD status_code = query_status_code(request_handle);
    const std::string response_body = read_response_body(request_handle);

    if (status_code != 200) {
        throw std::runtime_error(build_http_error_message(status_code, response_body));
    }

    return response_body;
#endif
}

}  // namespace soundcloud::api
