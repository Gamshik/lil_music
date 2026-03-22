add_library(soundcloud_api
    src/soundcloud_http_client.cpp
    src/soundcloud_track_search_response_parser.cpp
    src/soundcloud_transcoding_response_parser.cpp
    src/soundcloud_api_client.cpp
)

add_library(soundcloud::api ALIAS soundcloud_api)

target_include_directories(soundcloud_api
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/nlohmann_json/include
)

target_link_libraries(soundcloud_api
    PUBLIC
        soundcloud::core
)

if(WIN32)
    target_link_libraries(soundcloud_api PRIVATE winhttp)
endif()

include(${PROJECT_SOURCE_DIR}/cmake/project_options.cmake)
soundcloud_configure_target(soundcloud_api)
