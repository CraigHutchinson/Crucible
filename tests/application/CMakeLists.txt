add_executable(crucible_game_journey_tests test_game_journey.cpp)
target_link_libraries(crucible_game_journey_tests PRIVATE Crucible::Application)
add_test(NAME application_game_journey COMMAND crucible_game_journey_tests)
set_tests_properties(application_game_journey PROPERTIES LABELS "application;mission;lifetime" TIMEOUT 180)

add_executable(crucible_profile_store_tests test_profile_store.cpp)
target_link_libraries(crucible_profile_store_tests PRIVATE Crucible::Application)
add_test(NAME application_profile_store COMMAND crucible_profile_store_tests ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(application_profile_store PROPERTIES LABELS "application;profile;lifetime" TIMEOUT 30)

add_test(NAME application_journey_passive_cli COMMAND crucible --journey)
set_tests_properties(application_journey_passive_cli PROPERTIES LABELS "application;integration;mission"
    TIMEOUT 180 PASS_REGULAR_EXPRESSION "outcome=LOST tick=900 attempts=2")
add_test(NAME application_journey_route_cli COMMAND crucible --journey --route)
set_tests_properties(application_journey_route_cli PROPERTIES LABELS "application;integration;mission"
    TIMEOUT 180 PASS_REGULAR_EXPRESSION "outcome=WON tick=.*attempts=2")
add_test(NAME application_journey_profile_cli COMMAND crucible --journey --route
    --profile ${CMAKE_CURRENT_BINARY_DIR}/journey-progress.txt)
set_tests_properties(application_journey_profile_cli PROPERTIES LABELS "application;integration;mission;profile"
    TIMEOUT 180 PASS_REGULAR_EXPRESSION "outcome=WON tick=.*persistent_progress=received")
