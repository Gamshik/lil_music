#pragma once

#include <string>
#include <unordered_set>

#include "soundcloud/core/ports/i_library_repository.h"

namespace soundcloud::database {

/**
 * Заготовка SQLite-репозитория локальной библиотеки.
 * В текущем skeleton хранит состояние в памяти, чтобы проверить контракты и wiring.
 * Реальная SQLite-интеграция будет добавлена без изменения core-интерфейса.
 */
class sqlite_library_repository final : public core::ports::i_library_repository {
public:
    sqlite_library_repository() = default;

    /**
     * Проверяет наличие трека в локальном избранном.
     */
    bool is_favorite(const std::string& track_id) const override;

    /**
     * Обновляет локальное состояние избранного.
     */
    bool set_favorite(const std::string& track_id, bool is_favorite) override;

private:
    std::unordered_set<std::string> favorite_track_ids_;
};

}  // namespace soundcloud::database
