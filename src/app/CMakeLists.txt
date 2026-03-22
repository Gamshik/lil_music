add_executable(soundcloud_desktop
    src/main.cpp
    src/desktop_application.cpp
)

set(SOUNDCLOUD_UI_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/ui)

file(GLOB_RECURSE SOUNDCLOUD_UI_SOURCE_FILES CONFIGURE_DEPENDS
    "${SOUNDCLOUD_UI_SOURCE_DIR}/*.html"
    "${SOUNDCLOUD_UI_SOURCE_DIR}/*.css"
    "${SOUNDCLOUD_UI_SOURCE_DIR}/*.js"
)

set(SOUNDCLOUD_UI_OUTPUT_FILES)

foreach(ui_source_file IN LISTS SOUNDCLOUD_UI_SOURCE_FILES)
    file(RELATIVE_PATH ui_relative_path "${SOUNDCLOUD_UI_SOURCE_DIR}" "${ui_source_file}")
    set(ui_output_file "${CMAKE_CURRENT_BINARY_DIR}/ui/${ui_relative_path}")
    get_filename_component(ui_output_directory "${ui_output_file}" DIRECTORY)

    add_custom_command(
        OUTPUT "${ui_output_file}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ui_output_directory}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ui_source_file}" "${ui_output_file}"
        DEPENDS "${ui_source_file}"
        COMMENT "Copying UI asset ${ui_relative_path}"
        VERBATIM
    )

    list(APPEND SOUNDCLOUD_UI_OUTPUT_FILES "${ui_output_file}")
endforeach()

add_custom_target(soundcloud_ui_assets ALL
    DEPENDS ${SOUNDCLOUD_UI_OUTPUT_FILES}
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
