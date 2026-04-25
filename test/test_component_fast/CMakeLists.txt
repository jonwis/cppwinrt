# test_component_fast — WinRT fast-ABI component DLL
#
# Uses -fastabi flag and links the cppwinrt_fast_forwarder static library.
# Also defines WINRT_FAST_ABI_SIZE=50 (matches /DWINRT_FAST_ABI_SIZE=50 in vcxproj).

include(CppWinRTHelpers)

set(_bindir "${CMAKE_CURRENT_BINARY_DIR}")
set(_gendir "${_bindir}")
set(_winmd  "${_bindir}/test_component_fast.winmd")

winrt_midl_compile(
    TARGET     test_component_fast
    IDL_FILE   test_component_fast.idl
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
            -fastabi -prefix -opt
            -name test_component_fast
            -verbose
    DEPENDS cppwinrt "${_winmd}"
    COMMENT "cppwinrt: generating test_component_fast stubs"
    VERBATIM
)
set_source_files_properties("${_module_g}" PROPERTIES GENERATED TRUE)

add_library(test_component_fast SHARED
    Composition.CompositionObject.cpp
    Composition.Compositor.cpp
    Composition.SpriteVisual.cpp
    Composition.Visual.cpp
    Nomadic.cpp
    Simple.cpp
    "${_module_g}"
    pch.cpp
)
add_dependencies(test_component_fast cppwinrt)

target_compile_definitions(test_component_fast PRIVATE WINRT_FAST_ABI_SIZE=50)

target_include_directories(test_component_fast PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_bindir}"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
    "${CMAKE_BINARY_DIR}/winmd-prefix/src/winmd/src"
)

target_precompile_headers(test_component_fast PRIVATE pch.h)
set_source_files_properties("${_module_g}" PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)

target_link_libraries(test_component_fast PRIVATE cppwinrt_fast_forwarder)

set_target_properties(test_component_fast PROPERTIES
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/exports.def\""
)
