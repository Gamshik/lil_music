#include "soundcloud/app/desktop_application.h"

#include <exception>
#include <iostream>

#include "soundcloud/api/soundcloud_api_configuration.h"
#include "soundcloud/core/use_cases/get_playback_state_use_case.h"
#include "soundcloud/core/use_cases/list_featured_tracks_use_case.h"
#include "soundcloud/core/use_cases/pause_playback_use_case.h"
#include "soundcloud/core/use_cases/play_track_use_case.h"
#include "soundcloud/core/use_cases/resume_playback_use_case.h"
#include "soundcloud/core/use_cases/search_tracks_use_case.h"
#include "soundcloud/core/use_cases/seek_playback_use_case.h"
#include "soundcloud/core/use_cases/toggle_favorite_use_case.h"
#include "soundcloud/platform/window_configuration.h"

namespace soundcloud::app {
namespace {

api::soundcloud_api_configuration build_soundcloud_api_configuration() {
    return api::soundcloud_api_configuration{
        .api_host = "api-v2.soundcloud.com",
        .client_id = "IvZsSdfTxP6ovYz9Nn4XGqmQVKs1vzbB",
        .default_search_limit = 24,
    };
}

}  // namespace

desktop_application::desktop_application()
    : api_client_(build_soundcloud_api_configuration()),
      bridge_(
          core::use_cases::get_playback_state_use_case(audio_player_),
          core::use_cases::list_featured_tracks_use_case(api_client_),
          core::use_cases::play_track_use_case(api_client_, audio_player_),
          core::use_cases::pause_playback_use_case(audio_player_),
          core::use_cases::resume_playback_use_case(audio_player_),
          core::use_cases::seek_playback_use_case(audio_player_),
          core::use_cases::search_tracks_use_case(api_client_),
          core::use_cases::toggle_favorite_use_case(library_repository_)) {}

int desktop_application::run() {
    try {
        const platform::application_paths application_paths = application_paths_resolver_.resolve();

        const platform::window_configuration main_window_configuration{
            .title = "Lil Music",
            .width = 1360,
            .height = 860,
            .entry_file_path = application_paths.ui_entry_file,
        };

        // Bridge и player уже собраны в composition root и будут подключены к UI
        // по мере появления JS API и playback-сценариев.
        (void)audio_player_;

        window_controller_.run(main_window_configuration, bridge_);
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Не удалось запустить desktop shell: " << exception.what() << '\n';
        return 1;
    }
}

}  // namespace soundcloud::app
