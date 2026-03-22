add_library(soundcloud_platform
    src/application_paths_resolver.cpp
    src/window_controller.cpp
)

add_library(soundcloud::platform ALIAS soundcloud_platform)

target_include_directories(soundcloud_platform
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(soundcloud_platform
    PUBLIC
        webview::core
    PRIVATE
        soundcloud::bridge
)

include(${PROJECT_SOURCE_DIR}/cmake/project_options.cmake)
soundcloud_configure_target(soundcloud_platform)
