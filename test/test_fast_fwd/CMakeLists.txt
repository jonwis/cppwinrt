# test_fast_fwd — fast-forwarder tests
#
# Has its own IDL (FastForwarderTest.idl) compiled to a .winmd, then also
# consumes test_component_fast.winmd with -fastabi.
# Links cppwinrt_fast_forwarder directly.

include(CppWinRTHelpers)

set(_bindir      "${CMAKE_CURRENT_BINARY_DIR}")
set(_fast_winmd  "${CMAKE_BINARY_DIR}/test/test_component_fast/test_component_fast.winmd")
set(_fwd_winmd   "${_bindir}/FastForwarderTest.winmd")
set(_projection_stamp "${_bindir}/test_fast_fwd_projection.stamp")

# Compile FastForwarderTest.idl → FastForwarderTest.winmd
winrt_midl_compile(
    TARGET     test_fast_fwd
    IDL_FILE   FastForwarderTest.idl
    WINMD_OUT  "${_fwd_winmd}"
    HEADER_OUT "nul"
)

# Generate fast-ABI projection using test_component_fast.winmd
# and project FastForwarderTest.winmd for its own types
add_custom_command(
    OUTPUT  "${_projection_stamp}"
    BYPRODUCTS
            "${_bindir}/winrt/fast_forward.h"
            "${_bindir}/winrt/FastForwarderTest.h"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_bindir}"
    COMMAND "${CPPWINRT_TOOL_COMMAND}"
            -input "${_fast_winmd}"
            -out   "${_bindir}"
            -ref sdk
            -fastabi
            -verbose
    COMMAND "${CPPWINRT_TOOL_COMMAND}"
            -input "${_fwd_winmd}"
            -out   "${_bindir}"
            -comp  "${_bindir}/stubs"
            -ref   "${_fast_winmd}"
            -ref sdk
            -fastabi -prefix -opt
            -name FastForwarderTest
            -verbose
    COMMAND "${CMAKE_COMMAND}" -E touch "${_projection_stamp}"
    DEPENDS
        ${CPPWINRT_TOOL_DEPENDS}
        "${_fast_winmd}"
        "${_fwd_winmd}"
    COMMENT "cppwinrt: generating fast-ABI headers for test_fast_fwd"
    VERBATIM
)
set_source_files_properties("${_projection_stamp}" PROPERTIES GENERATED TRUE)

add_custom_target(test_fast_fwd_projection DEPENDS "${_projection_stamp}")
add_dependencies(test_fast_fwd_projection test_component_fast)

add_executable(test_fast_fwd
    FastForwarderTests.cpp
    pch.cpp
    main.cpp
)
set_source_files_properties(main.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)
target_precompile_headers(test_fast_fwd PRIVATE pch.h)

add_dependencies(test_fast_fwd test_fast_fwd_projection test_component_fast)

target_include_directories(test_fast_fwd PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_bindir}"
    "${CMAKE_BINARY_DIR}/test/test_component_fast"
    "${CMAKE_SOURCE_DIR}/test"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
)

target_compile_definitions(test_fast_fwd PRIVATE NOMINMAX _MBCS)

target_link_libraries(test_fast_fwd PRIVATE cppwinrt_fast_forwarder)

add_test(
    NAME test_fast_fwd
    COMMAND "$<TARGET_FILE:test_fast_fwd>" ${TEST_COLOR_ARG}
)
