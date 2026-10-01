crucible_add_workstream(crucible_swarm Swarm Steering.cpp)
target_link_libraries(crucible_swarm PUBLIC Crucible::Contracts
    PRIVATE Crucible::Spatial Crucible::Fields)
