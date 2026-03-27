#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace soundcloud::core::domain {

enum class equalizer_preset_id {
    flat,
    bass_boost,
    treble_boost,
    vocal,
    pop,
    rock,
    electronic,
    hip_hop,
    jazz,
    classical,
    acoustic,
    dance,
    piano,
    spoken_podcast,
    loudness,
    custom,
};

/**
 * Встроенный пресет эквалайзера.
 * Хранит только data model без UI-логики.
 * Важно, что preset не живёт в persistence: он часть доменной модели приложения,
 * а не пользовательские данные из БД.
 */
struct equalizer_preset {
    equalizer_preset_id id = equalizer_preset_id::flat;
    const char* title = "Flat";
    std::array<float, 10> gains_db{};
};

/**
 * Нормализует enum preset-а в строковый id для bridge/persistence слоя.
 */
[[nodiscard]] inline std::string_view to_string(const equalizer_preset_id preset_id) {
    switch (preset_id) {
        case equalizer_preset_id::flat:
            return "flat";
        case equalizer_preset_id::bass_boost:
            return "bass_boost";
        case equalizer_preset_id::treble_boost:
            return "treble_boost";
        case equalizer_preset_id::vocal:
            return "vocal";
        case equalizer_preset_id::pop:
            return "pop";
        case equalizer_preset_id::rock:
            return "rock";
        case equalizer_preset_id::electronic:
            return "electronic";
        case equalizer_preset_id::hip_hop:
            return "hip_hop";
        case equalizer_preset_id::jazz:
            return "jazz";
        case equalizer_preset_id::classical:
            return "classical";
        case equalizer_preset_id::acoustic:
            return "acoustic";
        case equalizer_preset_id::dance:
            return "dance";
        case equalizer_preset_id::piano:
            return "piano";
        case equalizer_preset_id::spoken_podcast:
            return "spoken_podcast";
        case equalizer_preset_id::loudness:
            return "loudness";
        case equalizer_preset_id::custom:
            return "custom";
    }

    return "custom";
}

/**
 * Обратное преобразование строкового id в доменный enum.
 * Используется при чтении сохранённого состояния эквалайзера из persistence.
 */
[[nodiscard]] inline std::optional<equalizer_preset_id> equalizer_preset_id_from_string(
    const std::string_view value) {
    if (value == "flat") {
        return equalizer_preset_id::flat;
    }
    if (value == "bass_boost") {
        return equalizer_preset_id::bass_boost;
    }
    if (value == "treble_boost") {
        return equalizer_preset_id::treble_boost;
    }
    if (value == "vocal") {
        return equalizer_preset_id::vocal;
    }
    if (value == "pop") {
        return equalizer_preset_id::pop;
    }
    if (value == "rock") {
        return equalizer_preset_id::rock;
    }
    if (value == "electronic") {
        return equalizer_preset_id::electronic;
    }
    if (value == "hip_hop") {
        return equalizer_preset_id::hip_hop;
    }
    if (value == "jazz") {
        return equalizer_preset_id::jazz;
    }
    if (value == "classical") {
        return equalizer_preset_id::classical;
    }
    if (value == "acoustic") {
        return equalizer_preset_id::acoustic;
    }
    if (value == "dance") {
        return equalizer_preset_id::dance;
    }
    if (value == "piano") {
        return equalizer_preset_id::piano;
    }
    if (value == "spoken_podcast") {
        return equalizer_preset_id::spoken_podcast;
    }
    if (value == "loudness") {
        return equalizer_preset_id::loudness;
    }
    if (value == "custom") {
        return equalizer_preset_id::custom;
    }

    return std::nullopt;
}

}  // namespace soundcloud::core::domain
