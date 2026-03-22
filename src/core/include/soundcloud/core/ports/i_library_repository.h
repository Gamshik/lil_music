#pragma once

#include <string>

namespace soundcloud::core::ports {

/**
 * Порт локальной библиотеки.
 * Инкапсулирует хранение пользовательских сущностей вне зависимости от СУБД.
 */
class i_library_repository {
public:
    virtual ~i_library_repository() = default;

    /**
     * Проверяет, отмечен ли трек как избранный в локальной библиотеке.
     */
    virtual bool is_favorite(const std::string& track_id) const = 0;

    /**
     * Переключает флаг избранного и возвращает новое состояние.
     */
    virtual bool set_favorite(const std::string& track_id, bool is_favorite) = 0;
};

}  // namespace soundcloud::core::ports
