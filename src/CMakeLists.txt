set(WEBVIEW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WEBVIEW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WEBVIEW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(WEBVIEW_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
set(WEBVIEW_BUILD_STATIC_LIBRARY OFF CACHE BOOL "" FORCE)
set(WEBVIEW_ENABLE_CHECKS OFF CACHE BOOL "" FORCE)
set(SONOTIDE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SONOTIDE_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/webview
    ${CMAKE_BINARY_DIR}/third_party/webview
    EXCLUDE_FROM_ALL
)

add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/sonotide
    ${CMAKE_BINARY_DIR}/third_party/sonotide
    EXCLUDE_FROM_ALL
)

add_subdirectory(core)
add_subdirectory(api)
add_subdirectory(player)
add_subdirectory(database)
add_subdirectory(bridge)
add_subdirectory(platform)
add_subdirectory(app)
