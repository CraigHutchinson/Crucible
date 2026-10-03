# Shader compilation is explicit and opt-in; retain the actual compiler identity.
find_program(CRUCIBLE_GLSLANG_VALIDATOR NAMES glslangValidator REQUIRED)
execute_process(COMMAND "${CRUCIBLE_GLSLANG_VALIDATOR}" --version
    OUTPUT_VARIABLE CRUCIBLE_GLSLANG_VERSION RESULT_VARIABLE shader_version_result)
if(NOT shader_version_result EQUAL 0)
    message(FATAL_ERROR "Cannot inspect the configured glslangValidator")
endif()
file(WRITE "${PROJECT_BINARY_DIR}/gpu-shader-toolchain.txt" "${CRUCIBLE_GLSLANG_VERSION}")
set(CRUCIBLE_GPU_VERTEX_SPIRV "${PROJECT_BINARY_DIR}/gpu-shaders/world.vert.spv")
set(CRUCIBLE_GPU_FRAGMENT_SPIRV "${PROJECT_BINARY_DIR}/gpu-shaders/world.frag.spv")
foreach(stage IN ITEMS vert frag)
    add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/gpu-shaders/world.${stage}.spv"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/gpu-shaders"
        COMMAND "${CRUCIBLE_GLSLANG_VALIDATOR}" -V --target-env vulkan1.0
            "${PROJECT_SOURCE_DIR}/src/presentation/gpu/shaders/world.${stage}"
            -o "${PROJECT_BINARY_DIR}/gpu-shaders/world.${stage}.spv"
        DEPENDS "${PROJECT_SOURCE_DIR}/src/presentation/gpu/shaders/world.${stage}"
        VERBATIM)
endforeach()
add_custom_target(crucible_gpu_shaders DEPENDS "${CRUCIBLE_GPU_VERTEX_SPIRV}" "${CRUCIBLE_GPU_FRAGMENT_SPIRV}")
