add_executable(crucible_tests simulation.cpp)
target_link_libraries(crucible_tests PRIVATE Crucible::Core)
add_test(NAME simulation COMMAND crucible_tests)
set_tests_properties(simulation PROPERTIES LABELS integration)

add_executable(crucible_stack_test stack.cpp)
target_link_libraries(crucible_stack_test PRIVATE Crucible::Core Sub0Pipeline::Sub0Pipeline
    Sub0Pipeline::Headless Sub0Pub::Sub0Pub Sub0Log::Sub0Log)
add_test(NAME stack_round_trip COMMAND crucible_stack_test)
set_tests_properties(stack_round_trip PROPERTIES LABELS integration)

add_executable(crucible_scenario_test scenario.cpp)
target_link_libraries(crucible_scenario_test PRIVATE Crucible::Core)
add_test(NAME scenario_integration COMMAND crucible_scenario_test)
set_tests_properties(scenario_integration PROPERTIES LABELS integration)

add_executable(crucible_phase2_test phase2.cpp)
target_link_libraries(crucible_phase2_test PRIVATE Crucible::Runtime Crucible::Presentation)
add_test(NAME phase2_full_state COMMAND crucible_phase2_test)
set_tests_properties(phase2_full_state PROPERTIES LABELS "integration;presentation")
