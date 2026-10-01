add_executable(crucible_bench integration.cpp)
target_link_libraries(crucible_bench PRIVATE Crucible::Core)
# Preserve the capture script's established single-config executable location.
set_target_properties(crucible_bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
