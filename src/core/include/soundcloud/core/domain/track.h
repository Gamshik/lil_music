#pragma once

#include <string>

namespace soundcloud::core::domain {

/**
 * Базовая доменная модель трека.
 * Хранит только те данные, которые важны для сценариев приложения,
 * а не для транспортного формата API.
 */
struct track {
    std::string id;
    std::string title;
    std::string artist_name;
    std::string artwork_url;
    std::string stream_url;
};

}  // namespace soundcloud::core::domain
