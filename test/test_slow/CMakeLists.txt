# test_slow — comprehensive test suite (non-fastabi)
#
# Consumes test_component_fast.winmd but WITHOUT -fastabi for slower/full ABI.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(CppWinRTHelpers)

set(_bindir  "${CMAKE_CURRENT_BINARY_DIR}")
set(_fast_winmd "${CMAKE_BINARY_DIR}/test/test_component_fast/test_component_fast.winmd")
set(_projection_stamp "${_bindir}/test_slow_projection.stamp")

add_custom_command(
    OUTPUT  "${_projection_stamp}"
    BYPRODUCTS
            "${_bindir}/winrt/test_component_fast.h"
            "${_bindir}/winrt/test_component_fast.Composition.h"
    COMMAND "$<TARGET_FILE:cppwinrt>"
            -input "${_fast_winmd}"
            -out   "${_bindir}"
            -ref sdk
            -verbose
    COMMAND "${CMAKE_COMMAND}" -E touch "${_projection_stamp}"
    DEPENDS
        cppwinrt
        "${_fast_winmd}"
    COMMENT "cppwinrt: generating headers for test_slow"
    VERBATIM
)
set_source_files_properties("${_projection_stamp}" PROPERTIES GENERATED TRUE)

add_custom_target(test_slow_projection DEPENDS "${_projection_stamp}")
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
