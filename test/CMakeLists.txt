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

# The tests use newer C++ features.
set(CMAKE_CXX_STANDARD 20)
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
    add_compile_options(-mcx16)
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
    add_custom_target(build-cppwinrt-range-projection)
    add_custom_target(build-cppwinrt-range-plan-projection-full)
    add_custom_target(build-cppwinrt-range-plan-projection-filtered)
    set(CPPWINRT_PROJECTION_INCLUDE_DIR "" CACHE PATH "Include path for the C++/WinRT projection headers")
    set(CPPWINRT_RANGE_PROJECTION_INCLUDE_DIR "" CACHE PATH "Include path for the C++/WinRT range-filtered projection headers")
    set(CPPWINRT_RANGE_PLAN_FULL_PROJECTION_INCLUDE_DIR "" CACHE PATH "Include path for full plan projection headers")
    set(CPPWINRT_RANGE_PLAN_FILTERED_PROJECTION_INCLUDE_DIR "" CACHE PATH "Include path for filtered plan projection headers")
    set(CPPWINRT_RANGE_PLAN_FILTERED_HEADER "" CACHE FILEPATH "Path to the filtered Test.RangePlan header")
    if(NOT CPPWINRT_PROJECTION_INCLUDE_DIR)
        message(FATAL_ERROR "CPPWINRT_PROJECTION_INCLUDE_DIR is not specified.")
    endif()
else()
    set(CPPWINRT_PROJECTION_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/cppwinrt")
    set(CPPWINRT_RANGE_PROJECTION_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/cppwinrt_range")
    set(CPPWINRT_RANGE_PLAN_MIDL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/range_plan")
    set(CPPWINRT_RANGE_PLAN_WINMD "${CPPWINRT_RANGE_PLAN_MIDL_OUTPUT_DIR}/Test.RangePlan.winmd")
    set(CPPWINRT_RANGE_PLAN_FULL_PROJECTION_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/cppwinrt_range_plan_full")
    set(CPPWINRT_RANGE_PLAN_FILTERED_PROJECTION_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/cppwinrt_range_plan_filtered")
    set(CPPWINRT_RANGE_PLAN_FILTERED_HEADER "${CPPWINRT_RANGE_PLAN_FILTERED_PROJECTION_INCLUDE_DIR}/winrt/Test.RangePlan.h")
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

    add_custom_command(
        OUTPUT
            "${CPPWINRT_RANGE_PROJECTION_INCLUDE_DIR}/winrt/base.h"
        COMMAND cppwinrt -input local -output "${CPPWINRT_RANGE_PROJECTION_INCLUDE_DIR}" -max_platform "${CMAKE_CURRENT_SOURCE_DIR}/data/max_platform.xml" -verbose
        DEPENDS
            cppwinrt
            "${CMAKE_CURRENT_SOURCE_DIR}/data/max_platform.xml"
        VERBATIM
    )
    add_custom_target(build-cppwinrt-range-projection
        DEPENDS
            "${CPPWINRT_RANGE_PROJECTION_INCLUDE_DIR}/winrt/base.h"
    )

    add_custom_command(
        OUTPUT
            "${CPPWINRT_RANGE_PLAN_WINMD}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CPPWINRT_RANGE_PLAN_MIDL_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "C:/Program Files (x86)/Windows Kits/10/UnionMetadata/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/Windows.winmd" "${CPPWINRT_RANGE_PLAN_MIDL_OUTPUT_DIR}/Windows.winmd"
        COMMAND pwsh -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/run_msbuild_vs.ps1"
            "cd /d ${CMAKE_CURRENT_BINARY_DIR} && midlrt /winrt /nomidl /ns_prefix /h nul /env x64 /metadata_dir range_plan /out range_plan /winmd Test.RangePlan.winmd ../../test/data/range_plan.idl"
            -NoLogo -PassThruExitCode
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/data/range_plan.idl"
            "${CMAKE_SOURCE_DIR}/run_msbuild_vs.ps1"
        VERBATIM
    )

    add_custom_command(
        OUTPUT
            "${CPPWINRT_RANGE_PLAN_FULL_PROJECTION_INCLUDE_DIR}/winrt/base.h"
        COMMAND cppwinrt -input "${CPPWINRT_RANGE_PLAN_WINMD}" -output "${CPPWINRT_RANGE_PLAN_FULL_PROJECTION_INCLUDE_DIR}" -verbose
        DEPENDS
            cppwinrt
            "${CPPWINRT_RANGE_PLAN_WINMD}"
        VERBATIM
    )

    add_custom_command(
        OUTPUT
            "${CPPWINRT_RANGE_PLAN_FILTERED_PROJECTION_INCLUDE_DIR}/winrt/base.h"
        COMMAND cppwinrt -input "${CPPWINRT_RANGE_PLAN_WINMD}" -output "${CPPWINRT_RANGE_PLAN_FILTERED_PROJECTION_INCLUDE_DIR}" -min_platform "${CMAKE_CURRENT_SOURCE_DIR}/data/range_plan_min_platform.xml" -max_platform "${CMAKE_CURRENT_SOURCE_DIR}/data/range_plan_max_platform.xml" -verbose
        DEPENDS
            cppwinrt
            "${CPPWINRT_RANGE_PLAN_WINMD}"
            "${CMAKE_CURRENT_SOURCE_DIR}/data/range_plan_min_platform.xml"
            "${CMAKE_CURRENT_SOURCE_DIR}/data/range_plan_max_platform.xml"
        VERBATIM
    )

    add_custom_target(build-cppwinrt-range-plan-projection-full
        DEPENDS
            "${CPPWINRT_RANGE_PLAN_FULL_PROJECTION_INCLUDE_DIR}/winrt/base.h"
    )

    add_custom_target(build-cppwinrt-range-plan-projection-filtered
        DEPENDS
            "${CPPWINRT_RANGE_PLAN_FILTERED_PROJECTION_INCLUDE_DIR}/winrt/base.h"
    )
endif()
include_directories("${CPPWINRT_PROJECTION_INCLUDE_DIR}")


set(ENABLE_TEST_SANITIZERS FALSE CACHE BOOL "Enable ASan and UBSan for the tests.")
if(ENABLE_TEST_SANITIZERS)
    # Disable the 'vptr' check because it seems to produce false-positives when using COM classes.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined,address -fno-sanitize=vptr")
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

if(HAS_WINDOWSNUMERICS)
    add_subdirectory(old_tests)
endif()
