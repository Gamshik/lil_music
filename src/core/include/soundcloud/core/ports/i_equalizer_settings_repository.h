#pragma once

#include <optional>

#include "soundcloud/core/domain/equalizer_state.h"

namespace soundcloud::core::ports {

/**
 * Порт persistence для пользовательских настроек эквалайзера.
 * Хранит только пользовательское состояние, не владея встроенными пресетами.
 */
class i_equalizer_settings_repository {
public:
    virtual ~i_equalizer_settings_repository() = default;

    /**
     * Загружает сохранённое EQ-состояние, если оно уже было записано локально.
     */
    [[nodiscard]] virtual std::optional<domain::equalizer_state> load_equalizer_state() const = 0;

    /**
     * Сохраняет пользовательское EQ-состояние после любых изменений.
     */
    virtual void save_equalizer_state(const domain::equalizer_state& equalizer_state) = 0;
};

}  // namespace soundcloud::core::ports
