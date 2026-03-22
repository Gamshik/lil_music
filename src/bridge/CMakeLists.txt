add_library(soundcloud_bridge
    src/app_bridge.cpp
)

add_library(soundcloud::bridge ALIAS soundcloud_bridge)

target_include_directories(soundcloud_bridge
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(soundcloud_bridge
    PUBLIC
        soundcloud::core
)

include(${PROJECT_SOURCE_DIR}/cmake/project_options.cmake)
soundcloud_configure_target(soundcloud_bridge)
