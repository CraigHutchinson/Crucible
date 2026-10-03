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

add_executable(crucible_phase3_test phase3.cpp)
target_link_libraries(crucible_phase3_test PRIVATE Crucible::Runtime Crucible::Presentation)
add_test(NAME phase3_full_state COMMAND crucible_phase3_test)
set_tests_properties(phase3_full_state PROPERTIES LABELS "integration;presentation;resources")

if(NOT CRUCIBLE_BUILD_DESKTOP)
    add_test(NAME challenge_passive_cli COMMAND crucible --mission)
    add_test(NAME challenge_route_cli COMMAND crucible --mission --route)
    set_tests_properties(challenge_passive_cli PROPERTIES LABELS "integration;mission"
        PASS_REGULAR_EXPRESSION "LOST tick=900 .*conserved=1 replay=full-state")
    set_tests_properties(challenge_route_cli PROPERTIES LABELS "integration;mission"
        PASS_REGULAR_EXPRESSION "WON tick=.*conserved=1 replay=full-state")
endif()

if(CRUCIBLE_BUILD_DESKTOP)
    add_executable(crucible_desktop_event_test desktop.cpp)
    target_link_libraries(crucible_desktop_event_test PRIVATE crucible_desktop_adapter)
    add_test(NAME desktop_events COMMAND crucible_desktop_event_test)
    set_tests_properties(desktop_events PROPERTIES LABELS "integration;desktop")
endif()
