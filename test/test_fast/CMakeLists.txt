# test_fast — fast-ABI test executable
#
# Consumes test_component_fast.winmd to generate projection headers with -fastabi,
# then compiles the test suite.

include(CppWinRTHelpers)

set(_bindir  "${CMAKE_CURRENT_BINARY_DIR}")
set(_fast_winmd "${CMAKE_BINARY_DIR}/test/test_component_fast/test_component_fast.winmd")

winrt_cppwinrt_projection_target(
    NAME       test_fast_projection
    WINMD      "${_fast_winmd}"
    OUTPUT_DIR "${_bindir}"
    FASTABI
    BYPRODUCTS
        "${_bindir}/winrt/test_component_fast.h"
        "${_bindir}/winrt/test_component_fast.Composition.h"
)
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

add_test(
    NAME test_fast
    COMMAND "$<TARGET_FILE:test_fast>" ${TEST_COLOR_ARG}
)
