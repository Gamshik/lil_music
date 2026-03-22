add_library(soundcloud_database
    src/sqlite_library_repository.cpp
)

add_library(soundcloud::database ALIAS soundcloud_database)

target_include_directories(soundcloud_database
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(soundcloud_database
    PUBLIC
        soundcloud::core
)

include(${PROJECT_SOURCE_DIR}/cmake/project_options.cmake)
soundcloud_configure_target(soundcloud_database)
