add_library(soundcloud_core
    src/search_tracks_use_case.cpp
    src/toggle_favorite_use_case.cpp
)

add_library(soundcloud::core ALIAS soundcloud_core)

target_include_directories(soundcloud_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include(${PROJECT_SOURCE_DIR}/cmake/project_options.cmake)
soundcloud_configure_target(soundcloud_core)
