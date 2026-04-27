# old_tests/Component — legacy WinRT component DLL
# References Composable.winmd and includes Composable's Generated Files.

include(CppWinRTHelpers)

set(_bindir          "${CMAKE_CURRENT_BINARY_DIR}")
set(_composable_dir  "${CMAKE_BINARY_DIR}/test/old_tests/Composable")
set(_gendir          "${_bindir}")
set(_winmd           "${_bindir}/Component.winmd")
set(_composable_winmd "${_composable_dir}/Composable.winmd")

winrt_midl_compile(
    TARGET     Component
    IDL_FILE   Component.idl
    WINMD_OUT  "${_winmd}"
    HEADER_OUT "nul"
    EXTRA_IDL_DIRS "${CMAKE_SOURCE_DIR}/test/old_tests/Composable"
)

set(_module_g "${_gendir}/module.g.cpp")

add_custom_command(
    OUTPUT  "${_module_g}"
    COMMAND "${CPPWINRT_TOOL_COMMAND}"
            -input "${_winmd}"
            -ref   "${_composable_winmd}"
            -ref sdk
            -comp  "${_bindir}/stubs"
            -out   "${_bindir}"
            -verbose
    DEPENDS
        ${CPPWINRT_TOOL_DEPENDS}
        "${_winmd}"
        "${_composable_winmd}"
    COMMENT "cppwinrt: generating Component stubs"
    VERBATIM
)
set_source_files_properties("${_module_g}" PROPERTIES GENERATED TRUE)

add_library(Component SHARED
    CustomBase.cpp
    Errors.cpp
    Events.cpp
    FastInput.cpp
    FastInputMap.cpp
    FastInputVector.cpp
    NonCachedStatic.cpp
    Parameters.cpp
    Static.cpp
    Structures.cpp
    pch.cpp
    "${_module_g}"
)
add_dependencies(Component cppwinrt Composable)

target_include_directories(Component PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_bindir}"
    "${_composable_dir}"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
    "${CMAKE_BINARY_DIR}/winmd-prefix/src/winmd/src"
)

target_precompile_headers(Component PRIVATE pch.h)
set_source_files_properties("${_module_g}" PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)

set_target_properties(Component PROPERTIES
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/module.def\""
)
