# Pure packet receiving has no native/GPU dependency and may join headless CI.
add_executable(crucible_production_packet_tests
    "${CMAKE_CURRENT_LIST_DIR}/test_frame_validation.cpp")
target_compile_features(crucible_production_packet_tests PRIVATE cxx_std_23)
target_include_directories(crucible_production_packet_tests PRIVATE "${PROJECT_SOURCE_DIR}/include")
add_test(NAME production_scene_packet COMMAND crucible_production_packet_tests)
set_tests_properties(production_scene_packet PROPERTIES LABELS "presentation;production;contract")

# Header math only; this fixture never initializes a framework engine or GPU.
if(TARGET Crucible::FilamentRuntime)
    add_executable(crucible_production_transform_tests
        "${CMAKE_CURRENT_LIST_DIR}/test_native_transforms.cpp")
    target_compile_features(crucible_production_transform_tests PRIVATE cxx_std_23)
    target_include_directories(crucible_production_transform_tests PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(crucible_production_transform_tests PRIVATE Crucible::FilamentRuntime)
    add_test(NAME production_native_transforms COMMAND crucible_production_transform_tests)
    set_tests_properties(production_native_transforms PROPERTIES LABELS "presentation;production;contract")
endif()
