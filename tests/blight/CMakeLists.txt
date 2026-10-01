add_executable(crucible_blight_tests GridTests.cpp)
target_link_libraries(crucible_blight_tests PRIVATE Crucible::Blight)
add_test(NAME blight_grid COMMAND crucible_blight_tests)
set_tests_properties(blight_grid PROPERTIES LABELS blight)
