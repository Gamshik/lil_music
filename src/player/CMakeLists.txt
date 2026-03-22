add_library(soundcloud_player
    src/audio_player_service.cpp
)

add_library(soundcloud::player ALIAS soundcloud_player)

target_include_directories(soundcloud_player
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(soundcloud_player
    PUBLIC
        soundcloud::core
)

include(${PROJECT_SOURCE_DIR}/cmake/project_options.cmake)
soundcloud_configure_target(soundcloud_player)
