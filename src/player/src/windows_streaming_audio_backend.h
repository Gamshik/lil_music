#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "soundcloud/core/domain/equalizer_preset.h"
#include "soundcloud/core/domain/equalizer_state.h"
#include "soundcloud/core/domain/playback_state.h"

namespace soundcloud::player {

class windows_streaming_audio_backend {
public:
    class implementation;

    windows_streaming_audio_backend();
    ~windows_streaming_audio_backend();

    windows_streaming_audio_backend(const windows_streaming_audio_backend&) = delete;
    windows_streaming_audio_backend& operator=(const windows_streaming_audio_backend&) = delete;

    void load(const std::string& stream_url);
    void play();
    void pause();
    void seek_to(std::int64_t position_ms);
    void set_volume_percent(int volume_percent);
    void set_equalizer_enabled(bool enabled);
    void select_equalizer_preset(core::domain::equalizer_preset_id preset_id);
    void set_equalizer_band_gain(std::size_t band_index, float gain_db);
    void reset_equalizer();
    void set_equalizer_output_gain(float output_gain_db);
    void apply_equalizer_state(const core::domain::equalizer_state& equalizer_state);
    [[nodiscard]] core::domain::playback_state get_playback_state() const;
    [[nodiscard]] core::domain::equalizer_state get_equalizer_state() const;

private:
    std::unique_ptr<implementation> implementation_;
};

}  // namespace soundcloud::player
