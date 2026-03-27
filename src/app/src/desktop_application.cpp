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
#include "soundcloud/core/use_cases/set_playback_volume_use_case.h"
#include "soundcloud/core/use_cases/toggle_favorite_use_case.h"
#include "soundcloud/platform/window_configuration.h"

namespace soundcloud::app {
namespace {

api::soundcloud_api_configuration build_soundcloud_api_configuration() {
    // Конфигурация API вынесена отдельно, чтобы composition root было проще читать
    // и чтобы host/client_id не размазывались по разным use case-ам.
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
          core::use_cases::set_playback_volume_use_case(audio_player_),
          core::use_cases::search_tracks_use_case(api_client_),
          core::use_cases::toggle_favorite_use_case(library_repository_)) {}

int desktop_application::run() {
    try {
        // Здесь происходит composition root в чистом виде:
        // resolve runtime paths -> prepare window config -> run desktop shell.
        const platform::application_paths application_paths = application_paths_resolver_.resolve();

        const platform::window_configuration main_window_configuration{
            .title = "LilMusic",
            .width = 1360,
            .height = 860,
            .entry_file_path = application_paths.ui_entry_file,
        };

        // Player создан как часть графа зависимостей выше и остаётся живым на всё время процесса.
        (void)audio_player_;

        window_controller_.run(main_window_configuration, bridge_);
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Не удалось запустить desktop shell: " << exception.what() << '\n';
        return 1;
    }
}

}  // namespace soundcloud::app
