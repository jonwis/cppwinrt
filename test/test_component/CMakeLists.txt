# test_component — WinRT in-proc server DLL (main test component)
#
# Build steps:
#   1. midl: test_component.idl → test_component.winmd
#   2. cppwinrt: generate SDK projection + component stubs into "Generated Files/"
#   3. Compile everything into test_component.dll

include(CppWinRTHelpers)

set(_bindir "${CMAKE_CURRENT_BINARY_DIR}")
set(_gendir "${_bindir}")
set(_winmd  "${_bindir}/test_component.winmd")

# Step 1 — MIDL
winrt_midl_compile(
    TARGET     test_component
    IDL_FILE   test_component.idl
    WINMD_OUT  "${_winmd}"
    HEADER_OUT "nul"
)

# Step 2 — cppwinrt: SDK projection + component stubs
#   -in local            — also project the SDK metadata
#   -input <winmd>       — component metadata
#   -comp <dir>          — write implementation stubs here
#   -out  <dir>          — write projection headers here (same dir as stubs)
#   Additional flags matching the original vcxproj:
#     -prefix -opt -lib test -fastabi -overwrite -name test_component
set(_module_g "${_gendir}/module.g.cpp")

add_custom_command(
    OUTPUT
        "${_bindir}/winrt/base.h"
        "${_module_g}"
    BYPRODUCTS
        "${_bindir}/winrt/test_component.h"
    COMMAND "$<TARGET_FILE:cppwinrt>"
            -in local
            -out "${_bindir}"
            -verbose
    COMMAND "$<TARGET_FILE:cppwinrt>"
            -input "${_winmd}"
            -comp  "${_bindir}/stubs"
            -out   "${_bindir}"
            -ref sdk
            -include test_component
            -prefix -opt -lib test -fastabi -overwrite
            -name test_component
            -verbose
    DEPENDS
        cppwinrt
        "${_winmd}"
    COMMENT "cppwinrt: projecting + generating test_component stubs"
    VERBATIM
)
set_source_files_properties("${_module_g}" "${_bindir}/winrt/base.h" PROPERTIES GENERATED TRUE)

# Add a named target so dependents can wait for both the projection and the stubs
add_custom_target(test_component_codegen
    DEPENDS "${_bindir}/winrt/base.h" "${_module_g}"
)

add_library(test_component SHARED
    Class.cpp
    module.cpp
    Optional.cpp
    OverloadClass.cpp
    test_overload.cpp
    Simple.cpp
    test_auto.cpp
    Velocity.Class1.cpp
    Velocity.Class2.cpp
    Velocity.Class4.cpp
    Windows.Class.cpp
    "${_module_g}"
    pch.cpp
)

add_dependencies(test_component test_component_codegen)

target_include_directories(test_component PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_bindir}"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
    "${CMAKE_BINARY_DIR}/winmd-prefix/src/winmd/src"
)

target_compile_options(test_component PRIVATE
    /Zc:threadSafeInit-
    /wd4640
)

target_compile_definitions(test_component PRIVATE
    WINRT_NO_MAKE_DETECTION
    _MBCS
)

target_precompile_headers(test_component PRIVATE pch.h)
set_source_files_properties("${_module_g}" PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)

set_target_properties(test_component PROPERTIES
    LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/exports.def\""
)
