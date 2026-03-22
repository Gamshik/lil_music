#include "soundcloud/database/sqlite_library_repository.h"

namespace soundcloud::database {

bool sqlite_library_repository::is_favorite(const std::string& track_id) const {
    return favorite_track_ids_.contains(track_id);
}

bool sqlite_library_repository::set_favorite(const std::string& track_id, bool is_favorite) {
    if (is_favorite) {
        favorite_track_ids_.insert(track_id);
        return true;
    }

    favorite_track_ids_.erase(track_id);
    return false;
}

}  // namespace soundcloud::database
