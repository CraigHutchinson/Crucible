add_executable(crucible_swarm_tests Steering.cpp)
target_link_libraries(crucible_swarm_tests PRIVATE Crucible::Swarm Crucible::Spatial Crucible::Fields)
add_test(NAME swarm_steering COMMAND crucible_swarm_tests)
set_tests_properties(swarm_steering PROPERTIES LABELS swarm)
