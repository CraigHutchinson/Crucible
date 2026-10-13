add_executable(crucible_game game_main.cpp)
target_link_libraries(crucible_game PRIVATE crucible_options Crucible::Application
    Crucible::Frontend Crucible::CinematicChoreography Crucible::ProductionRenderer
    Crucible::ImGuiSDL3 Crucible::Presentation)
set_target_properties(crucible_game PROPERTIES CXX_EXTENSIONS OFF)
foreach(font_path IN ITEMS
    rajdhani/Rajdhani-SemiBold.ttf rajdhani/OFL.txt
    source-sans/SourceSans3-Regular.ttf source-sans/LICENSE.md)
    get_filename_component(font_parent "${font_path}" DIRECTORY)
    add_custom_command(TARGET crucible_game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:crucible_game>/assets/production/fonts/${font_parent}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_SOURCE_DIR}/assets/production/fonts/${font_path}"
            "$<TARGET_FILE_DIR:crucible_game>/assets/production/fonts/${font_path}"
        VERBATIM)
endforeach()
