add_library(soundcloud_api
    src/soundcloud_api_client.cpp
)

add_library(soundcloud::api ALIAS soundcloud_api)

target_include_directories(soundcloud_api
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(soundcloud_api
    PUBLIC
        soundcloud::core
)

include(${PROJECT_SOURCE_DIR}/cmake/project_options.cmake)
soundcloud_configure_target(soundcloud_api)
