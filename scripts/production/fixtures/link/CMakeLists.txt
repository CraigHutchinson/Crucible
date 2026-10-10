cmake_minimum_required(VERSION 3.22.1)
project(Phase15FilamentLink LANGUAGES CXX)

if(NOT MSVC OR NOT WIN32)
    message(FATAL_ERROR "This qualification receives the isolated Windows MSVC archive closure.")
endif()
if(NOT FILAMENT_SOURCE OR NOT FILAMENT_BUILD)
    message(FATAL_ERROR "Pass the exact received Filament source and Release build paths.")
endif()

add_library(phase15_filament_runtime INTERFACE)
target_include_directories(phase15_filament_runtime SYSTEM INTERFACE
    "${FILAMENT_SOURCE}/filament/include"
    "${FILAMENT_SOURCE}/filament/backend/include"
    "${FILAMENT_SOURCE}/libs/filabridge/include"
    "${FILAMENT_SOURCE}/libs/math/include"
    "${FILAMENT_SOURCE}/libs/utils/include"
    "${FILAMENT_SOURCE}/third_party/imgui")
target_link_libraries(phase15_filament_runtime INTERFACE
    "${FILAMENT_BUILD}/filament/filament.lib"
    "${FILAMENT_BUILD}/filament/backend/backend.lib"
    "${FILAMENT_BUILD}/libs/math/math.lib"
    "${FILAMENT_BUILD}/libs/utils/utils.lib"
    "${FILAMENT_BUILD}/libs/filaflat/filaflat.lib"
    "${FILAMENT_BUILD}/libs/filabridge/filabridge.lib"
    "${FILAMENT_BUILD}/third_party/zstd/tnt/zstd.lib"
    "${FILAMENT_BUILD}/libs/bluevk/bluevk.lib"
    "${FILAMENT_BUILD}/third_party/smol-v/tnt/smol-v.lib"
    "${FILAMENT_BUILD}/third_party/getopt/getopt.lib"
    "${FILAMENT_BUILD}/third_party/imgui/tnt/imgui.lib"
    Shlwapi)

add_executable(phase15_filament_link qualification_link.cpp)
target_compile_features(phase15_filament_link PRIVATE cxx_std_23)
set_property(TARGET phase15_filament_link PROPERTY MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
target_link_libraries(phase15_filament_link PRIVATE phase15_filament_runtime)
target_link_options(phase15_filament_link PRIVATE "/MAP:${CMAKE_CURRENT_BINARY_DIR}/qualification.map")
# The qualification helper builds this consumer but never executes it.
