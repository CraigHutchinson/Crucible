add_executable(crucible_gpu_packet_test PacketTests.cpp)
target_link_libraries(crucible_gpu_packet_test PRIVATE Crucible::GpuPacket Crucible::Core)
add_test(NAME gpu_packet_numeric COMMAND crucible_gpu_packet_test)
set_tests_properties(gpu_packet_numeric PROPERTIES LABELS "presentation;gpu;numeric")
add_executable(crucible_gpu_slot_test SlotTests.cpp)
target_include_directories(crucible_gpu_slot_test PRIVATE ${PROJECT_SOURCE_DIR}/src/presentation/gpu)
target_link_libraries(crucible_gpu_slot_test PRIVATE Crucible::GpuPacket)
add_test(NAME gpu_slot_retirement COMMAND crucible_gpu_slot_test)
set_tests_properties(gpu_slot_retirement PROPERTIES LABELS "presentation;gpu;lifetime")
if(CRUCIBLE_BUILD_GPU_RECEIVER)
    add_executable(crucible_gpu_readback_test ReadbackTests.cpp)
    target_link_libraries(crucible_gpu_readback_test PRIVATE Crucible::GpuOffscreen Crucible::Core Crucible::ScenePainter SDL3::SDL3)
    add_dependencies(crucible_gpu_readback_test crucible_gpu_shaders)
    add_test(NAME gpu_instanced_readback COMMAND crucible_gpu_readback_test
        ${CRUCIBLE_GPU_VERTEX_SPIRV} ${CRUCIBLE_GPU_FRAGMENT_SPIRV})
    set_tests_properties(gpu_instanced_readback PROPERTIES LABELS "gpu;readback;lifetime" TIMEOUT 120)
endif()
if(CRUCIBLE_BUILD_GPU_RECEIVER AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_executable(crucible_gpu_fault_test FaultTests.cpp FaultController.cpp)
    target_link_libraries(crucible_gpu_fault_test PRIVATE Crucible::GpuOffscreen Crucible::Core SDL3::SDL3)
    add_dependencies(crucible_gpu_fault_test crucible_gpu_shaders)
    foreach(symbol IN ITEMS
        SDL_CreateGPUShader SDL_CreateGPUGraphicsPipeline SDL_CreateGPUBuffer
        SDL_CreateGPUTransferBuffer SDL_CreateGPUTexture SDL_ReleaseGPUShader
        SDL_ReleaseGPUGraphicsPipeline SDL_ReleaseGPUBuffer SDL_ReleaseGPUTransferBuffer
        SDL_ReleaseGPUTexture SDL_ReleaseGPUFence SDL_AcquireGPUCommandBuffer
        SDL_CancelGPUCommandBuffer SDL_MapGPUTransferBuffer SDL_UnmapGPUTransferBuffer
        SDL_BeginGPUCopyPass SDL_BeginGPURenderPass SDL_SubmitGPUCommandBufferAndAcquireFence
        SDL_WaitForGPUIdle SDL_QueryGPUFence)
        target_link_options(crucible_gpu_fault_test PRIVATE "LINKER:--wrap=${symbol}")
    endforeach()
    add_test(NAME gpu_fault_receiving COMMAND crucible_gpu_fault_test
        ${CRUCIBLE_GPU_VERTEX_SPIRV} ${CRUCIBLE_GPU_FRAGMENT_SPIRV} normal)
    add_test(NAME gpu_fault_process COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/run_fault_processes.py $<TARGET_FILE:crucible_gpu_fault_test>
        ${CRUCIBLE_GPU_VERTEX_SPIRV} ${CRUCIBLE_GPU_FRAGMENT_SPIRV})
    set_tests_properties(gpu_fault_receiving gpu_fault_process PROPERTIES
        LABELS "gpu;fault;lifetime" TIMEOUT 120)
endif()

if(CRUCIBLE_BUILD_GPU_RECEIVER AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_tests_properties(gpu_fault_process PROPERTIES
        REQUIRED_FILES $<TARGET_FILE:crucible_gpu_fault_test>)
endif()
