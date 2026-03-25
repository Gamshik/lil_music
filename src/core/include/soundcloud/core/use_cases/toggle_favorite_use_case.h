#pragma once

#include <string>

#include "soundcloud/core/ports/i_library_repository.h"

namespace soundcloud::core::use_cases {

/**
 * Use case локального избранного.
 * Изолирует решение о переключении состояния от UI и инфраструктуры.
 */
class toggle_favorite_use_case {
public:
    /**
     * Создаёт use case поверх локального repository избранного.
     */
    explicit toggle_favorite_use_case(ports::i_library_repository& library_repository);

    /**
     * Меняет состояние избранного и возвращает новое значение.
     */
    bool execute(const std::string& track_id) const;

private:
    ports::i_library_repository& library_repository_;
};

}  // namespace soundcloud::core::use_cases
