#include "soundcloud/core/use_cases/toggle_favorite_use_case.h"

namespace soundcloud::core::use_cases {

toggle_favorite_use_case::toggle_favorite_use_case(ports::i_library_repository& library_repository)
    : library_repository_(library_repository) {}

bool toggle_favorite_use_case::execute(const std::string& track_id) const {
    const bool current_state = library_repository_.is_favorite(track_id);
    return library_repository_.set_favorite(track_id, !current_state);
}

}  // namespace soundcloud::core::use_cases
