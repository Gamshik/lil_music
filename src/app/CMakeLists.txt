add_executable(soundcloud_desktop
    src/main.cpp
    src/desktop_application.cpp
)

set(SOUNDCLOUD_UI_PROJECT_DIR ${PROJECT_SOURCE_DIR}/src/ui)
set(SOUNDCLOUD_UI_DIST_DIR ${SOUNDCLOUD_UI_PROJECT_DIR}/dist)
set(SOUNDCLOUD_UI_STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/ui_build.stamp)

find_program(SOUNDCLOUD_NPM_EXECUTABLE NAMES npm.cmd npm REQUIRED)

file(GLOB_RECURSE SOUNDCLOUD_UI_SOURCE_FILES CONFIGURE_DEPENDS
    "${SOUNDCLOUD_UI_PROJECT_DIR}/index.html"
    "${SOUNDCLOUD_UI_PROJECT_DIR}/package.json"
    "${SOUNDCLOUD_UI_PROJECT_DIR}/package-lock.json"
    "${SOUNDCLOUD_UI_PROJECT_DIR}/tsconfig.json"
    "${SOUNDCLOUD_UI_PROJECT_DIR}/tsconfig.node.json"
    "${SOUNDCLOUD_UI_PROJECT_DIR}/vite.config.ts"
    "${SOUNDCLOUD_UI_PROJECT_DIR}/src/*"
)

add_custom_command(
    OUTPUT "${SOUNDCLOUD_UI_STAMP_FILE}"
    COMMAND ${SOUNDCLOUD_NPM_EXECUTABLE} install
    COMMAND ${SOUNDCLOUD_NPM_EXECUTABLE} run build
    COMMAND ${CMAKE_COMMAND} -E touch "${SOUNDCLOUD_UI_STAMP_FILE}"
    WORKING_DIRECTORY "${SOUNDCLOUD_UI_PROJECT_DIR}"
    DEPENDS ${SOUNDCLOUD_UI_SOURCE_FILES}
    COMMENT "Building React UI bundle"
    VERBATIM
)

add_custom_target(soundcloud_ui_assets ALL
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/ui"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/ui"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SOUNDCLOUD_UI_DIST_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/ui"
    DEPENDS "${SOUNDCLOUD_UI_STAMP_FILE}"
    COMMENT "Copying built UI bundle"
)

add_dependencies(soundcloud_desktop soundcloud_ui_assets)

target_include_directories(soundcloud_desktop
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(soundcloud_desktop
    PRIVATE
        soundcloud::api
        soundcloud::bridge
        soundcloud::database
        soundcloud::player
        soundcloud::platform
)

include(${PROJECT_SOURCE_DIR}/cmake/project_options.cmake)
soundcloud_configure_target(soundcloud_desktop)
