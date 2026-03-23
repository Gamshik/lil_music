#pragma once

#include <string>

namespace soundcloud::api {

/**
 * Конфигурация публичного SoundCloud API.
 * Держит host и client_id отдельно от прикладной логики, чтобы composition root
 * мог явно управлять внешней интеграцией.
 */
struct soundcloud_api_configuration {
    std::string api_host;
    std::string client_id;
    int default_search_limit = 24;
};

}  // namespace soundcloud::api
