add_library(crucible_application game_journey.cpp profile_store.cpp)
add_library(Crucible::Application ALIAS crucible_application)
target_include_directories(crucible_application PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(crucible_application PUBLIC crucible_options Crucible::Runtime)
set_target_properties(crucible_application PROPERTIES CXX_EXTENSIONS OFF)

if(CRUCIBLE_BUILD_GAME)
    add_library(crucible_frontend frontend.cpp)
    add_library(Crucible::Frontend ALIAS crucible_frontend)
    target_include_directories(crucible_frontend PUBLIC ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(crucible_frontend PUBLIC crucible_options Crucible::Contracts Crucible::ImGui)
    set_target_properties(crucible_frontend PROPERTIES CXX_EXTENSIONS OFF)

    add_library(crucible_imgui_sdl3 STATIC
        "${CRUCIBLE_FILAMENT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_sdl3.cpp")
    add_library(Crucible::ImGuiSDL3 ALIAS crucible_imgui_sdl3)
    target_include_directories(crucible_imgui_sdl3 SYSTEM PUBLIC
        "${CRUCIBLE_FILAMENT_SOURCE_DIR}/third_party/imgui/backends")
    target_link_libraries(crucible_imgui_sdl3 PUBLIC Crucible::ImGui SDL3::SDL3)
endif()
