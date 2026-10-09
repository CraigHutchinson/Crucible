if(CRUCIBLE_BUILD_DESKTOP)
    execute_process(COMMAND git status --porcelain --untracked-files=all --
        include src cmake CMakeLists.txt benchmarks/CMakeLists.txt benchmarks/phase14_frame.cpp benchmarks/phase14_frame.cmake
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" RESULT_VARIABLE phase14_status_result
        OUTPUT_VARIABLE phase14_source_status OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT phase14_status_result EQUAL 0 OR NOT phase14_source_status STREQUAL "")
        message(FATAL_ERROR "Commit Phase14 compiled sources before configuring the native capture target.")
    endif()
    execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        RESULT_VARIABLE phase14_source_result OUTPUT_VARIABLE phase14_source_sha OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT phase14_source_result EQUAL 0)
        message(FATAL_ERROR "Phase14 native capture needs a source commit receipt.")
    endif()
    execute_process(COMMAND git rev-parse "HEAD^{tree}" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        RESULT_VARIABLE phase14_tree_result OUTPUT_VARIABLE phase14_source_tree OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT phase14_tree_result EQUAL 0)
        message(FATAL_ERROR "Phase14 native capture needs a source tree receipt.")
    endif()
    add_executable(crucible_phase14_frame_bench phase14_frame.cpp)
    target_link_libraries(crucible_phase14_frame_bench PRIVATE crucible_desktop_adapter Crucible::Fields)
    if(WIN32)
        target_compile_definitions(crucible_phase14_frame_bench PRIVATE NOMINMAX PSAPI_VERSION=2)
        target_link_libraries(crucible_phase14_frame_bench PRIVATE kernel32)
    endif()
    target_compile_definitions(crucible_phase14_frame_bench PRIVATE CRUCIBLE_PHASE14_BUILD_SOURCE="${phase14_source_sha}")
    target_compile_definitions(crucible_phase14_frame_bench PRIVATE CRUCIBLE_PHASE14_BUILD_TREE="${phase14_source_tree}")
endif()
