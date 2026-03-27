#include "dsp/equalizer_chain.h"

namespace soundcloud::player::dsp {

std::vector<soundcloud::core::domain::equalizer_preset> get_builtin_equalizer_presets() {
    using preset = soundcloud::core::domain::equalizer_preset;
    using preset_id = soundcloud::core::domain::equalizer_preset_id;

    // Все встроенные пресеты живут здесь как data-only наборы gain-ов.
    // Это упрощает сравнение с Custom и не заставляет UI хардкодить значения локально.
    return {
        preset{preset_id::flat, "Flat", {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F}},
        preset{preset_id::bass_boost, "Bass Boost", {5.0F, 4.5F, 3.5F, 2.0F, 1.0F, 0.0F, -1.0F, -1.5F, -2.0F, -2.5F}},
        preset{preset_id::treble_boost, "Treble Boost", {-2.0F, -1.5F, -1.0F, -0.5F, 0.0F, 1.0F, 2.0F, 3.5F, 4.5F, 5.0F}},
        preset{preset_id::vocal, "Vocal", {-2.0F, -1.5F, -0.5F, 1.0F, 2.5F, 3.5F, 3.0F, 1.5F, 0.5F, -1.0F}},
        preset{preset_id::pop, "Pop", {-1.0F, 1.5F, 3.0F, 3.5F, 2.0F, -0.5F, -1.0F, 1.0F, 2.0F, 2.5F}},
        preset{preset_id::rock, "Rock", {4.0F, 3.0F, 1.0F, -1.0F, -1.5F, 0.5F, 2.0F, 3.0F, 3.5F, 4.0F}},
        preset{preset_id::electronic, "Electronic", {4.0F, 3.5F, 2.0F, 0.5F, -1.0F, -0.5F, 1.5F, 3.0F, 3.5F, 3.0F}},
        preset{preset_id::hip_hop, "Hip-Hop", {5.0F, 4.5F, 2.5F, 1.0F, -0.5F, -1.0F, 0.5F, 1.5F, 2.0F, 1.0F}},
        preset{preset_id::jazz, "Jazz", {1.0F, 1.5F, 1.0F, 0.5F, 1.5F, 2.5F, 2.0F, 1.5F, 1.0F, 1.0F}},
        preset{preset_id::classical, "Classical", {0.0F, 0.0F, 0.5F, 1.5F, 2.0F, 1.5F, 0.5F, 0.0F, 0.5F, 1.0F}},
        preset{preset_id::acoustic, "Acoustic", {-0.5F, 0.5F, 1.5F, 2.5F, 2.0F, 1.0F, 0.5F, 1.0F, 1.5F, 1.0F}},
        preset{preset_id::dance, "Dance", {4.5F, 4.0F, 2.5F, 0.5F, -0.5F, 1.0F, 2.5F, 3.5F, 4.0F, 3.0F}},
        preset{preset_id::piano, "Piano", {-1.0F, -0.5F, 0.5F, 1.5F, 2.5F, 3.0F, 2.0F, 1.0F, 0.5F, 0.0F}},
        preset{preset_id::spoken_podcast, "Spoken / Podcast", {-4.0F, -3.0F, -1.0F, 1.0F, 3.0F, 4.5F, 4.0F, 2.5F, 0.0F, -1.0F}},
        preset{preset_id::loudness, "Loudness", {3.0F, 2.5F, 2.0F, 1.0F, 0.0F, 0.5F, 1.5F, 2.5F, 3.0F, 3.0F}},
        preset{preset_id::custom, "Custom", {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F}},
    };
}

}  // namespace soundcloud::player::dsp
