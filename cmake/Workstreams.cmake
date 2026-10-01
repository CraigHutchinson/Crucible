include_guard(GLOBAL)

# Explicit source lists belong to the owning workstream. An empty list reserves a
# build boundary without emitting dummy objects or claiming implemented gameplay.
function(crucible_add_workstream name alias)
    if(ARGN)
        add_library(${name} STATIC ${ARGN})
        set(scope PUBLIC)
        set_target_properties(${name} PROPERTIES CXX_EXTENSIONS OFF)
    else()
        add_library(${name} INTERFACE)
        set(scope INTERFACE)
    endif()
    add_library(Crucible::${alias} ALIAS ${name})
    target_include_directories(${name} ${scope} "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(${name} ${scope} crucible_options)
endfunction()
