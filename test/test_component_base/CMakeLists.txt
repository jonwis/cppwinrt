# test_component_base — base class WinRT component DLL

include(CppWinRTHelpers)

set(_bindir "${CMAKE_CURRENT_BINARY_DIR}")
set(_gendir "${_bindir}")
set(_winmd  "${_bindir}/test_component_base.winmd")

winrt_midl_compile(
    TARGET     test_component_base
    IDL_FILE   test_component_base.idl
    WINMD_OUT  "${_winmd}"
    HEADER_OUT "nul"
)

set(_module_g "${_gendir}/module.g.cpp")

add_custom_command(
    OUTPUT  "${_module_g}"
    BYPRODUCTS
        "${_bindir}/winrt/test_component_base.h"
    COMMAND "${CPPWINRT_TOOL_COMMAND}"
            -input "${_winmd}"
            -comp  "${_bindir}/stubs"
            -out   "${_bindir}"
            -ref sdk
            -name test_component_base
            -verbose
    DEPENDS ${CPPWINRT_TOOL_DEPENDS} "${_winmd}"
    COMMENT "cppwinrt: generating test_component_base stubs"
    VERBATIM
)
set_source_files_properties("${_module_g}" PROPERTIES GENERATED TRUE)

add_library(test_component_base SHARED
    HierarchyA.cpp
    HierarchyB.cpp
    "${_module_g}"
    pch.cpp
)
add_dependencies(test_component_base cppwinrt)

target_include_directories(test_component_base PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_bindir}"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
    "${CMAKE_BINARY_DIR}/winmd-prefix/src/winmd/src"
)

target_precompile_headers(test_component_base PRIVATE pch.h)
set_source_files_properties("${_module_g}" PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)

target_compile_definitions(test_component_base PRIVATE WINRT_NO_MODULE_LOCK)

# Suppress C4005 (macro redefinition) since pch.h also defines WINRT_NO_MODULE_LOCK
target_compile_options(test_component_base PRIVATE /wd4005)

set_target_properties(test_component_base PROPERTIES
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/exports.def\""
)
