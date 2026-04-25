# test_component_no_pch — same as test_component but compiled without PCH.
# Tests the header-only / single-include usage path.

include(CppWinRTHelpers)

set(_bindir "${CMAKE_CURRENT_BINARY_DIR}")
set(_gendir "${_bindir}")
set(_winmd  "${_bindir}/test_component_no_pch.winmd")

winrt_midl_compile(
    TARGET     test_component_no_pch
    IDL_FILE   test_component_no_pch.idl
    WINMD_OUT  "${_winmd}"
    HEADER_OUT "nul"
)

set(_module_g "${_gendir}/module.g.cpp")

add_custom_command(
    OUTPUT  "${_module_g}"
    BYPRODUCTS
        "${_bindir}/winrt/test_component_no_pch.h"
        "${_bindir}/winrt/test_component_no_pch.Peer1.h"
        "${_bindir}/winrt/test_component_no_pch.Peer2.h"
    COMMAND "$<TARGET_FILE:cppwinrt>"
            -input "${_winmd}"
            -comp  "${_bindir}/stubs"
            -out   "${_bindir}"
            -ref sdk
            -verbose -overwrite
            -pch .
            -name test_component_no_pch
    DEPENDS cppwinrt "${_winmd}"
    COMMENT "cppwinrt: generating test_component_no_pch stubs"
    VERBATIM
)
set_source_files_properties("${_module_g}" PROPERTIES GENERATED TRUE)

# Source list mirrors the original test_component but built without PCH.
# Use file(GLOB ...) mirroring the original ItemGroup.
file(GLOB _srcs LIST_DIRECTORIES false CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(FILTER _srcs EXCLUDE REGEX "/pch\\.cpp$")

add_library(test_component_no_pch SHARED
    ${_srcs}
    "${_module_g}"
)
add_dependencies(test_component_no_pch cppwinrt)

target_include_directories(test_component_no_pch PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_bindir}"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
    "${CMAKE_BINARY_DIR}/winmd-prefix/src/winmd/src"
)

# No PCH — disable warning 4100 (unreferenced formal parameter) which fires
# more frequently without a shared prefix header.
target_compile_options(test_component_no_pch PRIVATE /wd4100)

set_target_properties(test_component_no_pch PROPERTIES
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/exports.def\""
)
