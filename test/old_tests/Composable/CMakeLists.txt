# old_tests/Composable — legacy composable WinRT component DLL
# Non-standard PCH: precomp.hpp (not pch.h)

include(CppWinRTHelpers)

set(_bindir "${CMAKE_CURRENT_BINARY_DIR}")
set(_gendir "${_bindir}")
set(_winmd  "${_bindir}/Composable.winmd")

winrt_midl_compile(
    TARGET     Composable
    IDL_FILE   Composable.idl
    WINMD_OUT  "${_winmd}"
    HEADER_OUT "nul"
)

set(_module_g "${_gendir}/module.g.cpp")

add_custom_command(
    OUTPUT  "${_module_g}"
    COMMAND "$<TARGET_FILE:cppwinrt>"
            -input "${_winmd}"
            -comp  "${_bindir}/stubs"
            -out   "${_bindir}"
            -ref sdk
            -pch precomp.hpp
            -name Composable
            -verbose
    DEPENDS cppwinrt "${_winmd}"
    COMMENT "cppwinrt: generating Composable stubs"
    VERBATIM
)
set_source_files_properties("${_module_g}" PROPERTIES GENERATED TRUE)

add_library(Composable SHARED
    Base.cpp
    Derived.cpp
    precomp.cpp
    "${_module_g}"
)
add_dependencies(Composable cppwinrt)

target_include_directories(Composable PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_bindir}"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
    "${CMAKE_BINARY_DIR}/winmd-prefix/src/winmd/src"
)

# PCH uses non-standard header name
target_precompile_headers(Composable PRIVATE precomp.hpp)
set_source_files_properties("${_module_g}" PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)

set_target_properties(Composable PROPERTIES
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/module.def\""
)
