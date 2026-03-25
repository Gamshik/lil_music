#include "soundcloud/database/sqlite_library_repository.h"

namespace soundcloud::database {

bool sqlite_library_repository::is_favorite(const std::string& track_id) const {
    // В skeleton-версии repository работает как in-memory хранилище,
    // но внешний контракт уже совпадает с будущей SQLite-реализацией.
    return favorite_track_ids_.contains(track_id);
}

bool sqlite_library_repository::set_favorite(const std::string& track_id, bool is_favorite) {
    if (is_favorite) {
        // Состояние избранного хранится локально и не зависит от SoundCloud API.
        favorite_track_ids_.insert(track_id);
        return true;
    }

    // Удаление из избранного симметрично добавлению: просто убираем id из локального набора.
    favorite_track_ids_.erase(track_id);
    return false;
}

}  // namespace soundcloud::database
