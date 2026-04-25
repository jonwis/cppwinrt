cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED PROJECT_NAME)
    project(cppwinrt-tests)
    set(STANDALONE_TESTING 1)
    include(CTest)
    if(NOT BUILD_TESTING)
        message(NOTICE "BUILD_TESTING is OFF, nothing to do.")
        return()
    endif()
endif()

# Default all tests to C++17; cpp20-specific test projects opt into C++20.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../cppwinrt")

function(TestIsX64 OUTPUT_VARNAME)
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("
#if !defined(__x86_64__) && !defined(_M_X64)
#  error Not x86_64
#endif
int main() {}
    " ${OUTPUT_VARNAME})
endfunction()
TestIsX64(TARGET_IS_X64)
if(TARGET_IS_X64)
    # -mcx16 is a GCC/Clang flag; MSVC/ClangCL already has __int128/cx16 support
    if(NOT MSVC)
        add_compile_options(-mcx16)
    endif()
endif()


# Some tests requires windowsnumerics.impl.h, but mingw-w64 didn't have this
# header until very recently. In case it is not present, download a copy if
# DOWNLOAD_WINDOWSNUMERICS is true, otherwise skip the tests which depend on
# this header.
function(TestHasWindowsnumerics OUTPUT_VARNAME)
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("
#define _WINDOWS_NUMERICS_NAMESPACE_ winrt::Windows::Foundation::Numerics
#define _WINDOWS_NUMERICS_BEGIN_NAMESPACE_ namespace winrt::Windows::Foundation::Numerics
#define _WINDOWS_NUMERICS_END_NAMESPACE_
#include <windowsnumerics.impl.h>
int main() {}
    " ${OUTPUT_VARNAME})
endfunction()
TestHasWindowsnumerics(HAS_WINDOWSNUMERICS)
set(DOWNLOAD_WINDOWSNUMERICS FALSE CACHE BOOL "Whether to download a copy of mingw-w64's windowsnumerics.impl.h if not available.")
if(NOT HAS_WINDOWSNUMERICS AND DOWNLOAD_WINDOWSNUMERICS)
    file(
        DOWNLOAD https://github.com/mingw-w64/mingw-w64/raw/2b6272b31132e156dd1fc3722c1aa96b705a90dd/mingw-w64-headers/include/windowsnumerics.impl.h
        "${CMAKE_CURRENT_BINARY_DIR}/windowsnumerics/windowsnumerics.impl.h"
        EXPECTED_HASH SHA256=aff42491e57583c8ad8ca8e71d417a553bd1215ee9a71378679400ecded4b1ab
        SHOW_PROGRESS
    )
    include_directories("${CMAKE_CURRENT_BINARY_DIR}/windowsnumerics")
    set(HAS_WINDOWSNUMERICS TRUE)
    message(STATUS "Using windowsnumerics.impl.h downloaded from mingw-w64")
endif()


if(STANDALONE_TESTING)
    add_custom_target(build-cppwinrt-projection)
    set(CPPWINRT_PROJECTION_INCLUDE_DIR "" CACHE PATH "Include path for the C++/WinRT projection headers")
    if(NOT CPPWINRT_PROJECTION_INCLUDE_DIR)
        message(FATAL_ERROR "CPPWINRT_PROJECTION_INCLUDE_DIR is not specified.")
    endif()
else()
    set(CPPWINRT_PROJECTION_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/cppwinrt"
        CACHE PATH "Include path for the generated C++/WinRT projection headers" FORCE)
    add_custom_command(
        OUTPUT
            "${CMAKE_CURRENT_BINARY_DIR}/cppwinrt/winrt/base.h"
        COMMAND cppwinrt -input local -output "${CPPWINRT_PROJECTION_INCLUDE_DIR}" -verbose
        DEPENDS
            cppwinrt
        VERBATIM
    )
    add_custom_target(build-cppwinrt-projection
        DEPENDS
            "${CMAKE_CURRENT_BINARY_DIR}/cppwinrt/winrt/base.h"
    )
endif()
include_directories("${CPPWINRT_PROJECTION_INCLUDE_DIR}")


set(ENABLE_TEST_SANITIZERS FALSE CACHE BOOL "Enable ASan and UBSan for the tests.")
if(ENABLE_TEST_SANITIZERS)
    # Disable the 'vptr' check because it seems to produce false-positives when using COM classes.
    add_compile_options(-fsanitize=undefined,address -fno-sanitize=vptr)
    add_link_options(-fsanitize=undefined,address)
endif()


set(USE_ANSI_COLOR FALSE CACHE BOOL "Enable ANSI colour output for Catch2 test runner.")
if(USE_ANSI_COLOR)
    add_compile_definitions(CATCH_CONFIG_COLOUR_ANSI)
    set(TEST_COLOR_ARG "--use-colour yes")
endif()


set(SKIP_LARGE_PCH FALSE CACHE BOOL "Skip building large precompiled headers.")


add_subdirectory(test)
add_subdirectory(test_nocoro)
add_subdirectory(test_cpp20)
add_subdirectory(test_cpp20_no_sourcelocation)

if(HAS_WINDOWSNUMERICS AND NOT (MSVC AND NOT CMAKE_CROSSCOMPILING))
    add_subdirectory(old_tests)
endif()

# ---------------------------------------------------------------------------
# MSVC-only test targets that require MIDL + WinRT component DLLs
# ---------------------------------------------------------------------------
if(MSVC AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(test_component)
    add_subdirectory(test_component_no_pch)
    add_subdirectory(test_component_fast)
    add_subdirectory(test_component_base)
    add_subdirectory(test_component_derived)
    add_subdirectory(test_component_folders)
    add_subdirectory(test_fast)
    add_subdirectory(test_slow)
    add_subdirectory(test_fast_fwd)
    add_subdirectory(test_module_lock_custom)
    add_subdirectory(test_module_lock_none)

    # old_tests component DLLs (MSVC builds get the full set).
    # Add these before old_tests/UnitTests so add_dependencies() in UnitTests
    # can resolve the targets.
    if(HAS_WINDOWSNUMERICS)
        add_subdirectory(old_tests/Composable)
        add_subdirectory(old_tests/Component)
        add_subdirectory(old_tests)
    endif()
endif()

# ---------------------------------------------------------------------------
# NuGet integration test suite — invoke msbuild from a custom target.
# Not added to ALL; run manually with: cmake --build <dir> --target nuget_tests
# ---------------------------------------------------------------------------
if(MSVC AND NOT CMAKE_CROSSCOMPILING)
    find_program(MSBUILD_EXECUTABLE msbuild)
    if(MSBUILD_EXECUTABLE)
        set(_nuget_sln "${CMAKE_SOURCE_DIR}/test/nuget/NuGetTest.sln")
        set(_nuget_platform "${CMAKE_GENERATOR_PLATFORM}")
        if(_nuget_platform STREQUAL "Win32")
            # msbuild uses "Win32" already
        elseif(_nuget_platform STREQUAL "")
            set(_nuget_platform "x64")
        endif()
        add_custom_target(nuget_tests
            COMMAND "${MSBUILD_EXECUTABLE}"
                    /m /restore
                    "/p:Configuration=$<CONFIG>"
                    "/p:Platform=${_nuget_platform}"
                    "/p:CppWinRTPath=$<SHELL_PATH:$<TARGET_FILE_DIR:cppwinrt>>/"
                    "${_nuget_sln}"
            COMMENT "Building NuGet integration test suite"
            VERBATIM
        )
    else()
        message(STATUS "msbuild not found -- nuget_tests target will not be available")
    endif()
endif()
