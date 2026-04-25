# test_fast — fast-ABI test executable
#
# Consumes test_component_fast.winmd to generate projection headers with -fastabi,
# then compiles the test suite.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(CppWinRTHelpers)

set(_bindir  "${CMAKE_CURRENT_BINARY_DIR}")
set(_fast_winmd "${CMAKE_BINARY_DIR}/test/test_component_fast/test_component_fast.winmd")
set(_projection_stamp "${_bindir}/test_fast_projection.stamp")

# Generate fast-ABI projection headers from the component .winmd
add_custom_command(
    OUTPUT  "${_projection_stamp}"
    BYPRODUCTS
            "${_bindir}/winrt/test_component_fast.h"
            "${_bindir}/winrt/test_component_fast.Composition.h"
    COMMAND "$<TARGET_FILE:cppwinrt>"
            -input "${_fast_winmd}"
            -out   "${_bindir}"
            -ref sdk
            -fastabi
            -verbose
    COMMAND "${CMAKE_COMMAND}" -E touch "${_projection_stamp}"
    DEPENDS
        cppwinrt
        "${_fast_winmd}"
    COMMENT "cppwinrt: generating fast-ABI headers for test_fast"
    VERBATIM
)
set_source_files_properties("${_projection_stamp}" PROPERTIES GENERATED TRUE)

add_custom_target(test_fast_projection DEPENDS "${_projection_stamp}")
add_dependencies(test_fast_projection test_component_fast)

add_executable(test_fast
    Composition.cpp
    Nomadic.cpp
    Simple.cpp
    pch.cpp
    main.cpp
)

set_source_files_properties(main.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)
target_precompile_headers(test_fast PRIVATE pch.h)

add_dependencies(test_fast test_fast_projection test_component_fast)

target_compile_definitions(test_fast PRIVATE
    WINRT_DIAGNOSTICS
    NOMINMAX
    _MBCS
)

target_include_directories(test_fast PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_bindir}"
    "${CMAKE_BINARY_DIR}/test/test_component_fast"
    "${CMAKE_SOURCE_DIR}/test"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
)

# Ensure component DLL is next to the test binary at runtime
add_dependencies(test_fast test_component_fast)

add_test(
    NAME test_fast
    COMMAND "$<TARGET_FILE:test_fast>" ${TEST_COLOR_ARG}
)
