crucible_add_workstream(crucible_runtime Runtime headless.cpp CommandIngress.cpp HeadlessSession.cpp ClockDriver.cpp InspectorSession.cpp ReferenceMissionRoute.cpp IntentDelivery.cpp BoundaryPipeline.cpp)
target_link_libraries(crucible_runtime PUBLIC Crucible::Core)
target_link_libraries(crucible_runtime PRIVATE Threads::Threads Sub0Pub::Sub0Pub Sub0Pipeline::Sub0Pipeline)

target_link_libraries(crucible_runtime PRIVATE Crucible::Presentation)
