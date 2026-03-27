#pragma once

#include <filesystem>
#include <optional>

#include "soundcloud/core/ports/i_equalizer_settings_repository.h"

namespace soundcloud::database {

/**
 * SQLite persistence для пользовательских настроек эквалайзера.
 * Хранит единственную logical запись с последним EQ-состоянием приложения.
 */
class sqlite_equalizer_settings_repository final
    : public core::ports::i_equalizer_settings_repository {
public:
    explicit sqlite_equalizer_settings_repository(std::filesystem::path database_file_path);

    /**
     * Возвращает сохранённый EQ snapshot из локальной SQLite-базы, если он есть.
     */
    [[nodiscard]] std::optional<core::domain::equalizer_state> load_equalizer_state() const override;

    /**
     * Перезаписывает единственную строку со state эквалайзера актуальным snapshot-ом.
     */
    void save_equalizer_state(const core::domain::equalizer_state& equalizer_state) override;

private:
    [[nodiscard]] std::filesystem::path database_file_path() const;

    std::filesystem::path database_file_path_;
};

}  // namespace soundcloud::database
