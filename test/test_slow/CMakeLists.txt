# test_slow — comprehensive test suite (non-fastabi)
#
# Consumes test_component_fast.winmd but WITHOUT -fastabi for slower/full ABI.

include(CppWinRTHelpers)

set(_bindir  "${CMAKE_CURRENT_BINARY_DIR}")
set(_fast_winmd "${CMAKE_BINARY_DIR}/test/test_component_fast/test_component_fast.winmd")

winrt_cppwinrt_projection_target(
    NAME       test_slow_projection
    WINMD      "${_fast_winmd}"
    OUTPUT_DIR "${_bindir}"
    BYPRODUCTS
        "${_bindir}/winrt/test_component_fast.h"
        "${_bindir}/winrt/test_component_fast.Composition.h"
)
add_dependencies(test_slow_projection test_component_fast)

add_executable(test_slow
    Composition.cpp
    Simple.cpp
    pch.cpp
    main.cpp
)

set_source_files_properties(main.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)
target_precompile_headers(test_slow PRIVATE pch.h)

add_dependencies(test_slow test_slow_projection test_component_fast)

target_compile_definitions(test_slow PRIVATE
    WINRT_DIAGNOSTICS
    NOMINMAX
    _MBCS
)

target_include_directories(test_slow PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_bindir}"
    "${CMAKE_BINARY_DIR}/test/test_component_fast"
    "${CMAKE_SOURCE_DIR}/test"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
)

add_test(
    NAME test_slow
    COMMAND "$<TARGET_FILE:test_slow>" ${TEST_COLOR_ARG}
)
