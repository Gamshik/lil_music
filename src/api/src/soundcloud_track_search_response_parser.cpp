#include "soundcloud/api/soundcloud_track_search_response_parser.h"

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "nlohmann/json.hpp"

namespace soundcloud::api {
namespace {

using json = nlohmann::json;

std::string read_optional_string_field(const json& object, const char* field_name) {
    const auto field_iterator = object.find(field_name);
    if (field_iterator == object.end() || !field_iterator->is_string()) {
        return {};
    }

    return field_iterator->get<std::string>();
}

std::string read_track_id(const json& track_json) {
    const auto id_iterator = track_json.find("id");
    if (id_iterator == track_json.end()) {
        return {};
    }

    if (id_iterator->is_string()) {
        return id_iterator->get<std::string>();
    }

    if (id_iterator->is_number_unsigned()) {
        return std::to_string(id_iterator->get<std::uint64_t>());
    }

    if (id_iterator->is_number_integer()) {
        return std::to_string(id_iterator->get<std::int64_t>());
    }

    return {};
}

std::string select_artist_name(const json& track_json) {
    const auto publisher_metadata_iterator = track_json.find("publisher_metadata");
    if (publisher_metadata_iterator != track_json.end() &&
        publisher_metadata_iterator->is_object()) {
        const std::string publisher_artist =
            read_optional_string_field(*publisher_metadata_iterator, "artist");
        if (!publisher_artist.empty()) {
            return publisher_artist;
        }
    }

    const auto user_iterator = track_json.find("user");
    if (user_iterator != track_json.end() && user_iterator->is_object()) {
        const std::string username = read_optional_string_field(*user_iterator, "username");
        if (!username.empty()) {
            return username;
        }
    }

    return "Неизвестный артист";
}

std::string append_client_id_to_url(const std::string& url, const std::string& client_id) {
    if (url.empty()) {
        return {};
    }

    const char separator = url.find('?') == std::string::npos ? '?' : '&';
    std::ostringstream stream_url;
    stream_url << url << separator << "client_id=" << client_id;
    return stream_url.str();
}

std::string select_stream_url(const json& track_json, const std::string& client_id) {
    const auto media_iterator = track_json.find("media");
    if (media_iterator == track_json.end() || !media_iterator->is_object()) {
        return {};
    }

    const auto transcodings_iterator = media_iterator->find("transcodings");
    if (transcodings_iterator == media_iterator->end() || !transcodings_iterator->is_array()) {
        return {};
    }

    std::string fallback_url;

    for (const json& transcoding_json : *transcodings_iterator) {
        if (!transcoding_json.is_object() || transcoding_json.value("snipped", false)) {
            continue;
        }

        const std::string transcoding_url =
            read_optional_string_field(transcoding_json, "url");
        if (transcoding_url.empty()) {
            continue;
        }

        if (fallback_url.empty()) {
            fallback_url = transcoding_url;
        }

        const auto format_iterator = transcoding_json.find("format");
        if (format_iterator == transcoding_json.end() || !format_iterator->is_object()) {
            continue;
        }

        if (read_optional_string_field(*format_iterator, "protocol") == "progressive") {
            return append_client_id_to_url(transcoding_url, client_id);
        }
    }

    return append_client_id_to_url(fallback_url, client_id);
}

core::domain::track map_track(const json& track_json, const std::string& client_id) {
    return core::domain::track{
        .id = read_track_id(track_json),
        .title = read_optional_string_field(track_json, "title"),
        .artist_name = select_artist_name(track_json),
        .stream_url = select_stream_url(track_json, client_id),
    };
}

}  // namespace

soundcloud_track_search_response_parser::soundcloud_track_search_response_parser(
    std::string client_id)
    : client_id_(std::move(client_id)) {}

std::vector<core::domain::track> soundcloud_track_search_response_parser::parse(
    const std::string& payload) const {
    try {
        const json root_json = json::parse(payload);
        const auto collection_iterator = root_json.find("collection");
        if (collection_iterator == root_json.end() || !collection_iterator->is_array()) {
            throw std::runtime_error(
                "Ответ SoundCloud API не содержит ожидаемое поле collection.");
        }

        std::vector<core::domain::track> tracks;
        tracks.reserve(collection_iterator->size());

        for (const json& track_json : *collection_iterator) {
            if (!track_json.is_object()) {
                continue;
            }

            if (track_json.value("kind", std::string{}) != "track") {
                continue;
            }

            core::domain::track track = map_track(track_json, client_id_);
            if (track.id.empty() || track.title.empty()) {
                continue;
            }

            tracks.push_back(std::move(track));
        }

        return tracks;
    } catch (const json::exception& exception) {
        throw std::runtime_error(
            std::string("Не удалось разобрать JSON-ответ SoundCloud API: ") +
            exception.what());
    }
}

}  // namespace soundcloud::api
