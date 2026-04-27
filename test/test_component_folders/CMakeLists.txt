# test_component_folders — WinRT component testing namespace/folder layout

include(CppWinRTHelpers)

set(_bindir "${CMAKE_CURRENT_BINARY_DIR}")
set(_gendir "${_bindir}")
set(_winmd  "${_bindir}/test_component_folders.winmd")

winrt_midl_compile(
    TARGET     test_component_folders
    IDL_FILE   test_component_folders.idl
    WINMD_OUT  "${_winmd}"
    HEADER_OUT "nul"
)

set(_module_g "${_gendir}/module.g.cpp")

add_custom_command(
    OUTPUT  "${_module_g}"
    BYPRODUCTS
        "${_bindir}/winrt/test_component_folders.h"
        "${_bindir}/winrt/test_component_folders.Nested.h"
    COMMAND "${CPPWINRT_TOOL_COMMAND}"
            -input "${_winmd}"
            -comp  "${_bindir}/stubs"
            -out   "${_bindir}"
            -ref sdk
            -verbose -overwrite
            -name test_component_folders
    DEPENDS ${CPPWINRT_TOOL_DEPENDS} "${_winmd}"
    COMMENT "cppwinrt: generating test_component_folders stubs"
    VERBATIM
)
set_source_files_properties("${_module_g}" PROPERTIES GENERATED TRUE)

add_library(test_component_folders SHARED
    Class.cpp
    "Nested.NestedClass.cpp"
    "${_module_g}"
    pch.cpp
)
add_dependencies(test_component_folders cppwinrt)

target_include_directories(test_component_folders PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_bindir}"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
    "${CMAKE_BINARY_DIR}/winmd-prefix/src/winmd/src"
)

target_compile_options(test_component_folders PRIVATE /wd4100)

target_precompile_headers(test_component_folders PRIVATE pch.h)
set_source_files_properties("${_module_g}" PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)

set_target_properties(test_component_folders PROPERTIES
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/exports.def\""
)
