#include "soundcloud/database/sqlite_equalizer_settings_repository.h"

#include <array>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <winsqlite/winsqlite3.h>
#else
#error "sqlite_equalizer_settings_repository currently supports only Windows."
#endif

namespace soundcloud::database {
namespace {

using soundcloud::core::domain::equalizer_band;
using soundcloud::core::domain::equalizer_state;

class sqlite_statement final {
public:
    explicit sqlite_statement(sqlite3_stmt* statement) : statement_(statement) {}

    sqlite_statement(const sqlite_statement&) = delete;
    sqlite_statement& operator=(const sqlite_statement&) = delete;

    sqlite_statement(sqlite_statement&& other) noexcept : statement_(other.statement_) {
        other.statement_ = nullptr;
    }

    sqlite_statement& operator=(sqlite_statement&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        reset();
        statement_ = other.statement_;
        other.statement_ = nullptr;
        return *this;
    }

    ~sqlite_statement() {
        reset();
    }

    [[nodiscard]] sqlite3_stmt* get() const {
        return statement_;
    }

private:
    void reset() {
        if (statement_ != nullptr) {
            sqlite3_finalize(statement_);
            statement_ = nullptr;
        }
    }

    sqlite3_stmt* statement_ = nullptr;
};

class sqlite_connection final {
public:
    explicit sqlite_connection(const std::filesystem::path& database_file_path) {
        const std::wstring wide_path = database_file_path.wstring();
        if (sqlite3_open16(wide_path.c_str(), &connection_) != SQLITE_OK) {
            const std::string error_message =
                connection_ != nullptr ? sqlite3_errmsg(connection_) : "sqlite3_open16 failed";
            if (connection_ != nullptr) {
                sqlite3_close(connection_);
                connection_ = nullptr;
            }

            throw std::runtime_error("Не удалось открыть SQLite-базу EQ: " + error_message);
        }
    }

    sqlite_connection(const sqlite_connection&) = delete;
    sqlite_connection& operator=(const sqlite_connection&) = delete;

    ~sqlite_connection() {
        if (connection_ != nullptr) {
            sqlite3_close(connection_);
        }
    }

    [[nodiscard]] sqlite3* get() const {
        return connection_;
    }

private:
    sqlite3* connection_ = nullptr;
};

void execute_sql(sqlite3* connection, const char* sql) {
    char* error_message = nullptr;
    const int result = sqlite3_exec(connection, sql, nullptr, nullptr, &error_message);
    if (result != SQLITE_OK) {
        const std::string message = error_message != nullptr ? error_message : "sqlite3_exec failed";
        if (error_message != nullptr) {
            sqlite3_free(error_message);
        }

        throw std::runtime_error(message);
    }
}

sqlite_statement prepare_statement(sqlite3* connection, const char* sql) {
    sqlite3_stmt* statement = nullptr;
    const int result = sqlite3_prepare_v2(connection, sql, -1, &statement, nullptr);
    if (result != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection));
    }

    return sqlite_statement(statement);
}

std::string serialize_band_gains(const std::array<equalizer_band, 10>& bands) {
    // В persistence мы держим gains как компактную CSV-строку:
    // этого достаточно для одного settings-record и не требует отдельной таблицы полос.
    std::ostringstream serialized;
    for (std::size_t index = 0; index < bands.size(); ++index) {
        if (index != 0) {
            serialized << ',';
        }

        serialized << bands[index].gain_db;
    }

    return serialized.str();
}

std::string serialize_band_gains(const std::array<float, 10>& gains_db) {
    std::ostringstream serialized;
    for (std::size_t index = 0; index < gains_db.size(); ++index) {
        if (index != 0) {
            serialized << ',';
        }

        serialized << gains_db[index];
    }

    return serialized.str();
}

std::array<float, 10> deserialize_band_gains(const std::string_view serialized_gains) {
    std::array<float, 10> gains_db{};
    std::stringstream stream{std::string(serialized_gains)};
    std::string token;

    for (std::size_t index = 0; index < gains_db.size(); ++index) {
        if (!std::getline(stream, token, ',')) {
            return gains_db;
        }

        gains_db[index] = std::stof(token);
    }

    return gains_db;
}

void ensure_schema(sqlite3* connection) {
    // В таблице всегда одна logical запись с id = 1.
    // Так хранилище эквалайзера работает как app-level settings store.
    execute_sql(
        connection,
        "CREATE TABLE IF NOT EXISTS equalizer_settings ("
        "id INTEGER PRIMARY KEY CHECK(id = 1),"
        "enabled INTEGER NOT NULL,"
        "active_preset_id TEXT NOT NULL,"
        "custom_band_gains TEXT NOT NULL,"
        "last_nonflat_user_state TEXT NOT NULL,"
        "output_gain_db REAL NOT NULL DEFAULT 0.0,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");");

    // Небольшая inline-миграция для старых локальных БД:
    // если output_gain_db уже есть, SQLite вернёт duplicate column name, что здесь допустимо.
    char* error_message = nullptr;
    const int alter_result = sqlite3_exec(
        connection,
        "ALTER TABLE equalizer_settings ADD COLUMN output_gain_db REAL NOT NULL DEFAULT 0.0;",
        nullptr,
        nullptr,
        &error_message);
    if (alter_result != SQLITE_OK && error_message != nullptr) {
        const std::string message = error_message;
        sqlite3_free(error_message);
        if (message.find("duplicate column name") == std::string::npos) {
            throw std::runtime_error(message);
        }
    }
}

}  // namespace

sqlite_equalizer_settings_repository::sqlite_equalizer_settings_repository(
    std::filesystem::path database_file_path)
    : database_file_path_(std::move(database_file_path)) {}

std::optional<equalizer_state> sqlite_equalizer_settings_repository::load_equalizer_state() const {
    std::filesystem::create_directories(database_file_path().parent_path());
    sqlite_connection connection(database_file_path());
    ensure_schema(connection.get());

    sqlite_statement statement = prepare_statement(
        connection.get(),
        "SELECT enabled, active_preset_id, custom_band_gains, last_nonflat_user_state, output_gain_db "
        "FROM equalizer_settings WHERE id = 1;");

    const int step_result = sqlite3_step(statement.get());
    if (step_result == SQLITE_DONE) {
        return std::nullopt;
    }

    if (step_result != SQLITE_ROW) {
        throw std::runtime_error(sqlite3_errmsg(connection.get()));
    }

    equalizer_state state;
    state.enabled = sqlite3_column_int(statement.get(), 0) != 0;

    const unsigned char* preset_text = sqlite3_column_text(statement.get(), 1);
    if (preset_text != nullptr) {
        if (const auto preset_id = soundcloud::core::domain::equalizer_preset_id_from_string(
                reinterpret_cast<const char*>(preset_text));
            preset_id.has_value()) {
            state.active_preset_id = *preset_id;
        }
    }

    const unsigned char* band_gains_text = sqlite3_column_text(statement.get(), 2);
    const unsigned char* last_nonflat_text = sqlite3_column_text(statement.get(), 3);
    state.output_gain_db = static_cast<float>(sqlite3_column_double(statement.get(), 4));

    const std::array<float, 10> gains_db = deserialize_band_gains(
        band_gains_text != nullptr ? reinterpret_cast<const char*>(band_gains_text) : "");
    const std::array<float, 10> last_nonflat_gains_db = deserialize_band_gains(
        last_nonflat_text != nullptr ? reinterpret_cast<const char*>(last_nonflat_text) : "");

    // Частоты полос не храним в БД:
    // они фиксированы на уровне DSP/домена и лишь восстанавливаются здесь для готового snapshot-а.
    constexpr std::array<float, 10> frequencies_hz{
        60.0F,
        170.0F,
        310.0F,
        600.0F,
        1000.0F,
        3000.0F,
        6000.0F,
        12000.0F,
        14000.0F,
        16000.0F,
    };

    for (std::size_t index = 0; index < state.bands.size(); ++index) {
        state.bands[index] = equalizer_band{
            .center_frequency_hz = frequencies_hz[index],
            .gain_db = gains_db[index],
        };
        state.last_nonflat_band_gains_db[index] = last_nonflat_gains_db[index];
    }

    return state;
}

void sqlite_equalizer_settings_repository::save_equalizer_state(
    const equalizer_state& equalizer_state) {
    std::filesystem::create_directories(database_file_path().parent_path());
    sqlite_connection connection(database_file_path());
    ensure_schema(connection.get());

    sqlite_statement statement = prepare_statement(
        connection.get(),
        "INSERT INTO equalizer_settings (id, enabled, active_preset_id, custom_band_gains, "
        "last_nonflat_user_state, output_gain_db, updated_at) "
        "VALUES (1, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP) "
        "ON CONFLICT(id) DO UPDATE SET "
        "enabled = excluded.enabled, "
        "active_preset_id = excluded.active_preset_id, "
        "custom_band_gains = excluded.custom_band_gains, "
        "last_nonflat_user_state = excluded.last_nonflat_user_state, "
        "output_gain_db = excluded.output_gain_db, "
        "updated_at = CURRENT_TIMESTAMP;");

    const std::string active_preset_id =
        std::string(soundcloud::core::domain::to_string(equalizer_state.active_preset_id));
    // В БД пишем именно текущее состояние bands, даже если активен built-in preset:
    // это позволяет восстановить UI и DSP одинаково после перезапуска.
    const std::string custom_band_gains = serialize_band_gains(equalizer_state.bands);
    const std::string last_nonflat_user_state =
        serialize_band_gains(equalizer_state.last_nonflat_band_gains_db);

    sqlite3_bind_int(statement.get(), 1, equalizer_state.enabled ? 1 : 0);
    sqlite3_bind_text(
        statement.get(),
        2,
        active_preset_id.c_str(),
        static_cast<int>(active_preset_id.size()),
        SQLITE_TRANSIENT);
    sqlite3_bind_text(
        statement.get(),
        3,
        custom_band_gains.c_str(),
        static_cast<int>(custom_band_gains.size()),
        SQLITE_TRANSIENT);
    sqlite3_bind_text(
        statement.get(),
        4,
        last_nonflat_user_state.c_str(),
        static_cast<int>(last_nonflat_user_state.size()),
        SQLITE_TRANSIENT);
    sqlite3_bind_double(statement.get(), 5, equalizer_state.output_gain_db);

    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(connection.get()));
    }
}

std::filesystem::path sqlite_equalizer_settings_repository::database_file_path() const {
    return database_file_path_;
}

}  // namespace soundcloud::database
