if(NOT TARGET Crucible::FilamentRuntime OR NOT TARGET Crucible::ImGui OR NOT TARGET SDL3::SDL3)
    message(FATAL_ERROR "Production renderer requires the qualified Filament runtime and SDL3 platform targets.")
endif()
if(NOT EXISTS "${CRUCIBLE_FILAMENT_BUILD_DIR}/tools/matc/matc${CMAKE_EXECUTABLE_SUFFIX}" OR
   NOT EXISTS "${CRUCIBLE_FILAMENT_BUILD_DIR}/tools/resgen/resgen${CMAKE_EXECUTABLE_SUFFIX}")
    message(FATAL_ERROR "Production materials require the exact received runtime's matc/resgen tools.")
endif()

set(material_directory "${CMAKE_CURRENT_BINARY_DIR}/crucible/presentation/production/resources")
file(MAKE_DIRECTORY "${material_directory}")
if(APPLE)
    set(material_backend metal)
else()
    set(material_backend vulkan)
endif()
set(material_outputs)
foreach(name IN ITEMS nanite floor ui_blit)
    set(output "${material_directory}/${name}.filamat")
    add_custom_command(OUTPUT "${output}"
        COMMAND "${CRUCIBLE_FILAMENT_BUILD_DIR}/tools/matc/matc${CMAKE_EXECUTABLE_SUFFIX}"
            -p all -a "${material_backend}" -o "${output}" "${CMAKE_CURRENT_SOURCE_DIR}/materials/${name}.mat"
        DEPENDS "materials/${name}.mat"
            "${CRUCIBLE_FILAMENT_BUILD_DIR}/tools/matc/matc${CMAKE_EXECUTABLE_SUFFIX}"
        VERBATIM)
    list(APPEND material_outputs "${output}")
endforeach()
add_custom_command(OUTPUT "${material_directory}/crucible_materials.c" "${material_directory}/crucible_materials.h"
    COMMAND "${CRUCIBLE_FILAMENT_BUILD_DIR}/tools/resgen/resgen${CMAKE_EXECUTABLE_SUFFIX}"
        --quiet --cfile --package=crucible_materials "--deploy=${material_directory}" ${material_outputs}
    DEPENDS ${material_outputs}
        "${CRUCIBLE_FILAMENT_BUILD_DIR}/tools/resgen/resgen${CMAKE_EXECUTABLE_SUFFIX}"
    VERBATIM)
file(WRITE "${material_directory}/material_package.cpp"
    "#include \"crucible_materials.h\"\nextern \"C\" {\n#include \"crucible_materials.c\"\n}\n")
set_source_files_properties("${material_directory}/material_package.cpp" PROPERTIES
    OBJECT_DEPENDS "${material_directory}/crucible_materials.c;${material_directory}/crucible_materials.h")

add_library(crucible_production_renderer STATIC production_renderer.cpp
    "${material_directory}/material_package.cpp"
    "${material_directory}/crucible_materials.c" "${material_directory}/crucible_materials.h")
set_source_files_properties("${material_directory}/crucible_materials.c" PROPERTIES HEADER_FILE_ONLY TRUE)
add_library(Crucible::ProductionRenderer ALIAS crucible_production_renderer)
target_compile_features(crucible_production_renderer PUBLIC cxx_std_23)
target_include_directories(crucible_production_renderer PUBLIC "${PROJECT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(crucible_production_renderer PUBLIC Crucible::FilamentRuntime Crucible::ImGui SDL3::SDL3)
