# test_component_derived — derived class WinRT component DLL
# Depends on test_component_base for the base .winmd reference.

include(CppWinRTHelpers)

set(_bindir      "${CMAKE_CURRENT_BINARY_DIR}")
set(_base_bindir "${CMAKE_BINARY_DIR}/test/test_component_base")
set(_winmd       "${_bindir}/test_component_derived.winmd")
set(_base_winmd  "${_base_bindir}/test_component_base.winmd")

winrt_midl_compile(
    TARGET     test_component_derived
    IDL_FILE   test_component_derived.idl
    WINMD_OUT  "${_winmd}"
    HEADER_OUT "nul"
    EXTRA_IDL_DIRS "${CMAKE_SOURCE_DIR}/test/test_component_base"
    EXTRA_DEPENDS  "${CMAKE_SOURCE_DIR}/test/test_component_base/test_component_base.idl"
)

set(_module_g "${_bindir}/module.g.cpp")

add_custom_command(
    OUTPUT  "${_module_g}"
    BYPRODUCTS
        "${_bindir}/winrt/test_component_derived.h"
        "${_bindir}/winrt/test_component_derived.Nested.h"
    COMMAND "${CPPWINRT_TOOL_COMMAND}"
            -input "${_winmd}"
            -ref   "${_base_winmd}"
            -ref sdk
            -comp  "${_bindir}/stubs"
            -out   "${_bindir}"
            -name test_component_derived
            -verbose
    DEPENDS
        ${CPPWINRT_TOOL_DEPENDS}
        "${_winmd}"
        "${_base_winmd}"
    COMMENT "cppwinrt: generating test_component_derived stubs"
    VERBATIM
)
set_source_files_properties("${_module_g}" PROPERTIES GENERATED TRUE)

add_library(test_component_derived SHARED
    "Nested.HierarchyC.cpp"
    "Nested.HierarchyD.cpp"
    "${_module_g}"
    pch.cpp
)
add_dependencies(test_component_derived cppwinrt test_component_base)

# Include generated headers from base and derived
target_include_directories(test_component_derived PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_bindir}"
    "${_base_bindir}"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
    "${CMAKE_BINARY_DIR}/winmd-prefix/src/winmd/src"
)

target_precompile_headers(test_component_derived PRIVATE pch.h)
set_source_files_properties("${_module_g}" PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)

set_target_properties(test_component_derived PROPERTIES
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/exports.def\""
)
