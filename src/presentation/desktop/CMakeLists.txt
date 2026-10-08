add_library(crucible_scene_painter ScenePainter.cpp frame_completion_observer.cpp)
add_library(Crucible::ScenePainter ALIAS crucible_scene_painter)
target_include_directories(crucible_scene_painter PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(crucible_scene_painter PUBLIC crucible_options Crucible::Presentation
    PRIVATE SDL3::SDL3)
set_target_properties(crucible_scene_painter PROPERTIES CXX_EXTENSIONS OFF)
if(BUILD_TESTING)
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests/presentation/desktop
                     ${PROJECT_BINARY_DIR}/tests/presentation/desktop)
endif()
