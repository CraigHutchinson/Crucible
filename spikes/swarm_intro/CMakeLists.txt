add_executable(crucible_swarm_intro main.cpp SwarmIntro.cpp SwarmDiagnostics.cpp Wordmark.cpp)
target_link_libraries(crucible_swarm_intro PRIVATE crucible_options Sub0ECS::Sub0ECS Sub0Log::Sub0Log)
if(BUILD_TESTING)
    foreach(build IN ITEMS lying raise climb hybrid scaffold)
        add_test(NAME swarm_intro_${build} COMMAND crucible_swarm_intro --verify --build ${build})
        set_tests_properties(swarm_intro_${build} PROPERTIES LABELS "presentation;spike" TIMEOUT 300)
    endforeach()
    # 3D tessellation: two staggered depth layers (2,146 tiles), twice the units.
    foreach(build IN ITEMS climb scaffold)
        add_test(NAME swarm_intro_layers2_${build} COMMAND crucible_swarm_intro --verify --build ${build} --layers 2 --count 3072)
        set_tests_properties(swarm_intro_layers2_${build} PROPERTIES LABELS "presentation;spike" TIMEOUT 600)
    endforeach()
    # Independent receiver for the Sub0Log JSONL diagnostics (standard-library JSON parser).
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    foreach(build IN ITEMS lying raise climb hybrid scaffold)
        add_test(NAME swarm_intro_diagnostics_${build}
            COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/verify_diagnostics.py $<TARGET_FILE:crucible_swarm_intro> --build ${build})
        set_tests_properties(swarm_intro_diagnostics_${build} PROPERTIES LABELS "presentation;spike;diagnostics" TIMEOUT 300)
    endforeach()
    # Nanite body proxy: mesh invariants (base is the tile, outward faces, one core inside the shell) and
    # parity with NaniteBody.hpp. Skipped (77) where numpy is not installed.
    add_test(NAME swarm_intro_nanite_mesh COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/test_nanite_mesh.py)
    set_tests_properties(swarm_intro_nanite_mesh PROPERTIES LABELS "presentation;spike" SKIP_RETURN_CODE 77 TIMEOUT 60)
    # Presentation-rate playback: interpolation and extrapolation between captures.
    add_test(NAME swarm_intro_playback COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/test_playback.py)
    set_tests_properties(swarm_intro_playback PROPERTIES LABELS "presentation;spike" TIMEOUT 60)
    add_test(NAME swarm_intro_diagnostics_layers2_scaffold
        COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/verify_diagnostics.py $<TARGET_FILE:crucible_swarm_intro> --build scaffold --layers 2 --count 3072)
    set_tests_properties(swarm_intro_diagnostics_layers2_scaffold PROPERTIES LABELS "presentation;spike;diagnostics" TIMEOUT 600)
endif()
