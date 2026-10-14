# Pure packet receiving has no native/GPU dependency and may join headless CI.
add_executable(crucible_production_packet_tests
    "${CMAKE_CURRENT_LIST_DIR}/test_frame_validation.cpp")
target_compile_features(crucible_production_packet_tests PRIVATE cxx_std_23)
target_include_directories(crucible_production_packet_tests PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(crucible_production_packet_tests PRIVATE crucible_options)
add_test(NAME production_scene_packet COMMAND crucible_production_packet_tests)
set_tests_properties(production_scene_packet PROPERTIES LABELS "presentation;production;contract")

add_executable(crucible_production_mesh_packet_tests "${CMAKE_CURRENT_LIST_DIR}/test_mesh_validation.cpp")
target_link_libraries(crucible_production_mesh_packet_tests PRIVATE crucible_options)
target_include_directories(crucible_production_mesh_packet_tests PRIVATE "${PROJECT_SOURCE_DIR}/include")
add_test(NAME production_mesh_packet COMMAND crucible_production_mesh_packet_tests)
set_tests_properties(production_mesh_packet PROPERTIES LABELS "presentation;production;contract")

# Header math only; this fixture never initializes a framework engine or GPU.
if(TARGET Crucible::FilamentRuntime)
    set(transform_modes precise)
    if(MSVC)
        list(APPEND transform_modes fast)
    endif()
    foreach(mode IN LISTS transform_modes)
        set(target "crucible_production_transform_${mode}_tests")
        add_executable("${target}" "${CMAKE_CURRENT_LIST_DIR}/test_native_transforms.cpp")
        target_link_libraries("${target}" PRIVATE crucible_options Crucible::FilamentRuntime)
        target_include_directories("${target}" PRIVATE "${PROJECT_SOURCE_DIR}/include")
        if(MSVC)
            target_compile_options("${target}" PRIVATE "/fp:${mode}")
        endif()
        add_test(NAME "production_native_transforms_${mode}" COMMAND "${target}")
        set_tests_properties("production_native_transforms_${mode}" PROPERTIES LABELS "presentation;production;contract")
    endforeach()
endif()

if(TARGET Crucible::SwarmIntroAdapter)
    add_test(NAME production_nanite_proxy_reference_identity COMMAND Python3::Interpreter
        "${CMAKE_CURRENT_LIST_DIR}/fixtures/verify_nanite_proxy_reference.py")
    set_tests_properties(production_nanite_proxy_reference_identity PROPERTIES
        LABELS "presentation;production;contract")
    add_executable(crucible_swarm_model_mesh_tests "${CMAKE_CURRENT_LIST_DIR}/test_swarm_model_mesh.cpp")
    target_link_libraries(crucible_swarm_model_mesh_tests PRIVATE
        Crucible::SwarmIntroAdapter Crucible::SwarmIntroPrototype Crucible::NaniteModel)
    add_test(NAME production_swarm_model_mesh COMMAND crucible_swarm_model_mesh_tests)
    set_tests_properties(production_swarm_model_mesh PROPERTIES
        LABELS "presentation;production;contract;lifetime" TIMEOUT 120)
    add_executable(crucible_swarm_intro_adapter_tests "${CMAKE_CURRENT_LIST_DIR}/test_swarm_intro_adapter.cpp")
    target_link_libraries(crucible_swarm_intro_adapter_tests PRIVATE
        Crucible::SwarmIntroAdapter Crucible::SwarmIntroPrototype)
    add_test(NAME production_swarm_intro_adapter COMMAND crucible_swarm_intro_adapter_tests)
    set_tests_properties(production_swarm_intro_adapter PROPERTIES
        LABELS "presentation;production;contract;lifetime" TIMEOUT 120)
    add_executable(crucible_nanite_proxy_mesh_tests "${CMAKE_CURRENT_LIST_DIR}/test_nanite_proxy_mesh.cpp")
    target_include_directories(crucible_nanite_proxy_mesh_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/presentation/production")
    target_link_libraries(crucible_nanite_proxy_mesh_tests PRIVATE
        Crucible::NaniteProxyFixture Crucible::SwarmIntroPrototype)
    add_test(NAME production_nanite_proxy_reference COMMAND crucible_nanite_proxy_mesh_tests
        "${CMAKE_CURRENT_LIST_DIR}/fixtures/nanite_proxy_reference.txt")
    set_tests_properties(production_nanite_proxy_reference PROPERTIES
        LABELS "presentation;production;contract" TIMEOUT 120)
endif()
