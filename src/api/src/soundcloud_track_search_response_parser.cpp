#include "soundcloud/api/soundcloud_track_search_response_parser.h"

#include <cstdint>
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
    const std::string track_urn = read_optional_string_field(track_json, "urn");
    if (!track_urn.empty()) {
        return track_urn;
    }

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

std::vector<track_transcoding_reference> read_transcoding_references(const json& track_json) {
    const auto media_iterator = track_json.find("media");
    if (media_iterator == track_json.end() || !media_iterator->is_object()) {
        return {};
    }

    const auto transcodings_iterator = media_iterator->find("transcodings");
    if (transcodings_iterator == media_iterator->end() || !transcodings_iterator->is_array()) {
        return {};
    }

    std::vector<track_transcoding_reference> transcodings;
    transcodings.reserve(transcodings_iterator->size());

    for (const json& transcoding_json : *transcodings_iterator) {
        if (!transcoding_json.is_object() || transcoding_json.value("snipped", false)) {
            continue;
        }

        const auto format_iterator = transcoding_json.find("format");
        if (format_iterator == transcoding_json.end() || !format_iterator->is_object()) {
            continue;
        }

        const std::string url = read_optional_string_field(transcoding_json, "url");
        if (url.empty()) {
            continue;
        }

        transcodings.push_back(track_transcoding_reference{
            .url = url,
            .protocol = read_optional_string_field(*format_iterator, "protocol"),
            .mime_type = read_optional_string_field(*format_iterator, "mime_type"),
            .is_legacy_transcoding = transcoding_json.value("is_legacy_transcoding", false),
        });
    }

    return transcodings;
}

core::domain::track map_track(const json& track_json) {
    return core::domain::track{
        .id = read_track_id(track_json),
        .title = read_optional_string_field(track_json, "title"),
        .artist_name = select_artist_name(track_json),
        .stream_url = {},
    };
}

track_playback_reference map_playback_reference(const json& track_json) {
    return track_playback_reference{
        .track_id = read_track_id(track_json),
        .track_authorization = read_optional_string_field(track_json, "track_authorization"),
        .transcodings = read_transcoding_references(track_json),
    };
}

}  // namespace

parsed_track_search_payload soundcloud_track_search_response_parser::parse(
    const std::string& payload) const {
    try {
        const json root_json = json::parse(payload);
        const auto collection_iterator = root_json.find("collection");
        if (collection_iterator == root_json.end() || !collection_iterator->is_array()) {
            throw std::runtime_error(
                "Ответ SoundCloud API не содержит ожидаемое поле collection.");
        }

        parsed_track_search_payload parsed_payload;
        parsed_payload.tracks.reserve(collection_iterator->size());
        parsed_payload.playback_references.reserve(collection_iterator->size());

        for (const json& track_json : *collection_iterator) {
            if (!track_json.is_object()) {
                continue;
            }

            if (track_json.value("kind", std::string{}) != "track") {
                continue;
            }

            core::domain::track track = map_track(track_json);
            if (track.id.empty() || track.title.empty()) {
                continue;
            }

            track_playback_reference playback_reference = map_playback_reference(track_json);
            if (playback_reference.track_authorization.empty() ||
                playback_reference.transcodings.empty()) {
                continue;
            }

            parsed_payload.tracks.push_back(std::move(track));
            parsed_payload.playback_references.push_back(std::move(playback_reference));
        }

        return parsed_payload;
    } catch (const json::exception& exception) {
        throw std::runtime_error(
            std::string("Не удалось разобрать JSON-ответ SoundCloud API: ") +
            exception.what());
    }
}

}  // namespace soundcloud::api
