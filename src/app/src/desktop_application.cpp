#include "soundcloud/app/desktop_application.h"

#include <exception>
#include <iostream>

#include "soundcloud/core/use_cases/search_tracks_use_case.h"
#include "soundcloud/core/use_cases/toggle_favorite_use_case.h"
#include "soundcloud/platform/window_configuration.h"

namespace soundcloud::app {

desktop_application::desktop_application()
    : api_client_("replace_with_client_id"),
      bridge_(
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
        (void)bridge_;

        window_controller_.run(main_window_configuration);
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Не удалось запустить desktop shell: " << exception.what() << '\n';
        return 1;
    }
}

}  // namespace soundcloud::app
