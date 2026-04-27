# Build system for cppwinrt.
#
# Supported configurations:
#   - Ninja Multi-Config with MSVC or ClangCL via CMakePresets.json
#   - MinGW / llvm-mingw (native or cross-compile from Linux)
#
# Recommended usage — pick a preset from CMakePresets.json:
#   cmake --preset msvc-x64
#   cmake --build --preset msvc-x64-release
#   ctest --preset msvc-x64-release
#
# For mingw/cross-compile:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cross-mingw-toolchain.cmake -B build/mingw
#   cmake --build build/mingw

cmake_minimum_required(VERSION 4.2)
cmake_policy(VERSION 4.2)

project(cppwinrt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

function(cppwinrt_normalize_arch INPUT OUTPUT)
    string(TOLOWER "${INPUT}" _arch)

    if(_arch STREQUAL "win32" OR _arch STREQUAL "x86" OR _arch STREQUAL "i386" OR _arch STREQUAL "i686")
        set(_normalized "x86")
    elseif(_arch STREQUAL "x64" OR _arch STREQUAL "amd64" OR _arch STREQUAL "x86_64")
        set(_normalized "x64")
    elseif(_arch MATCHES "^arm64")
        set(_normalized "arm64")
    else()
        set(_normalized "${_arch}")
    endif()

    set(${OUTPUT} "${_normalized}" PARENT_SCOPE)
endfunction()

set(CPPWINRT_TARGET_ARCH "")
foreach(_arch_candidate
        "${CMAKE_GENERATOR_PLATFORM}"
        "${CMAKE_VS_PLATFORM_NAME}"
        "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}"
        "${CMAKE_SYSTEM_PROCESSOR}")
    if(NOT _arch_candidate STREQUAL "")
        cppwinrt_normalize_arch("${_arch_candidate}" CPPWINRT_TARGET_ARCH)
        if(NOT CPPWINRT_TARGET_ARCH STREQUAL "")
            break()
        endif()
    endif()
endforeach()

cppwinrt_normalize_arch("${CMAKE_HOST_SYSTEM_PROCESSOR}" CPPWINRT_HOST_ARCH)

set(CPPWINRT_EXPECTED_TARGET_ARCH "" CACHE STRING "Expected target architecture for preset-driven Windows builds.")
if(NOT CPPWINRT_EXPECTED_TARGET_ARCH STREQUAL "")
    cppwinrt_normalize_arch("${CPPWINRT_EXPECTED_TARGET_ARCH}" _cppwinrt_expected_target_arch)
    if(NOT _cppwinrt_expected_target_arch STREQUAL CPPWINRT_TARGET_ARCH)
        message(FATAL_ERROR
            "Preset expects target architecture '${_cppwinrt_expected_target_arch}', "
            "but the current toolchain is targeting '${CPPWINRT_TARGET_ARCH}'. "
            "Run from a matching Visual Studio tools environment or use the root build scripts."
        )
    endif()
endif()

if(MSVC AND CPPWINRT_TARGET_ARCH STREQUAL "")
    message(FATAL_ERROR
        "Could not determine the active MSVC target architecture. "
        "Run from a matching Visual Studio tools environment or use the root build scripts."
    )
endif()

set(CPPWINRT_PREBUILD_TOOL "" CACHE FILEPATH "Path to a host cppwinrt-prebuild executable.")
if(MSVC AND CPPWINRT_TARGET_ARCH STREQUAL "arm64" AND NOT CPPWINRT_HOST_ARCH STREQUAL "arm64" AND CPPWINRT_PREBUILD_TOOL STREQUAL "")
    message(FATAL_ERROR
        "ARM64 builds from a non-ARM64 host require CPPWINRT_PREBUILD_TOOL to point to a host-built cppwinrt-prebuild executable. "
        "Use the root build scripts, which bootstrap that tool automatically."
    )
endif()

set(CPPWINRT_HOST_EXE "" CACHE FILEPATH "Path to a host cppwinrt executable for code generation.")
if(MSVC AND CPPWINRT_TARGET_ARCH STREQUAL "arm64" AND NOT CPPWINRT_HOST_ARCH STREQUAL "arm64" AND CPPWINRT_HOST_EXE STREQUAL "")
    message(FATAL_ERROR
        "ARM64 builds from a non-ARM64 host require CPPWINRT_HOST_EXE to point to a host-built cppwinrt executable. "
        "Use the root build scripts, which bootstrap that tool automatically."
    )
endif()

if(MSVC)
    set(CPPWINRT_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/_build/${CPPWINRT_TARGET_ARCH}")
    foreach(_config Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER "${_config}" _config_upper)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_config_upper} "${CPPWINRT_OUTPUT_DIR}/${_config}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_config_upper} "${CPPWINRT_OUTPUT_DIR}/${_config}")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_config_upper} "${CPPWINRT_OUTPUT_DIR}/${_config}")
        set(CMAKE_PDB_OUTPUT_DIRECTORY_${_config_upper} "${CPPWINRT_OUTPUT_DIR}/${_config}")
        set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY_${_config_upper} "${CPPWINRT_OUTPUT_DIR}/${_config}")
    endforeach()
endif()

set(CPPWINRT_BUILD_VERSION "999.999.999.999" CACHE STRING "The version string used for cppwinrt.")
if(CPPWINRT_BUILD_VERSION STREQUAL "999.999.999.999" OR CPPWINRT_BUILD_VERSION STREQUAL "0.0.0.0")
    message(WARNING "CPPWINRT_BUILD_VERSION has been set to a dummy version string. Do not use in production!")
endif()
message(STATUS "Using version string: ${CPPWINRT_BUILD_VERSION}")

# ---------------------------------------------------------------------------
# Global compiler flags when building with MSVC / ClangCL
# ---------------------------------------------------------------------------
if(MSVC)
    # Make runtime/debug info defaults explicit for reproducible builds.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")

    # Apply C/C++ compiler flags only to CXX sources so MASM/MARMASM don't
    # receive unsupported switches.
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:/W4>           # warnings-as-errors, level 4
        $<$<COMPILE_LANGUAGE:CXX>:/WX>
        $<$<COMPILE_LANGUAGE:CXX>:/permissive->  # conformance mode
        $<$<COMPILE_LANGUAGE:CXX>:/bigobj>       # large object files (test suites need this)
        $<$<COMPILE_LANGUAGE:CXX>:/MP>           # multi-processor compilation
    )
    # /await:strict is the C++17 coroutine mode; skip for C++20 and Clang
    # (Clang uses standard coroutines without the flag).
    if(NOT CPPWINRT_CLANG)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/await:strict>)
    endif()
    if(CPPWINRT_CLANG)
        add_compile_options(
            -Wno-unused-command-line-argument
            -fno-delayed-template-parsing
        )
        # mcx16 is x86/x64-specific; ClangCL ARM64 doesn't need it
        if(NOT CPPWINRT_TARGET_ARCH STREQUAL "arm64")
            add_compile_options(-mcx16)
        endif()
    endif()
    add_link_options(/defaultlib:onecore.lib)
endif()

# Minimum Windows version: 10.0.18362.0 (Windows 10 1903)
if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)
endif()


# ---------------------------------------------------------------------------
# === prebuild: Generator tool for strings.cpp, strings.h, version.rc ===
# ---------------------------------------------------------------------------

if(NOT CPPWINRT_PREBUILD_TOOL STREQUAL "")
    if(NOT EXISTS "${CPPWINRT_PREBUILD_TOOL}")
        message(FATAL_ERROR "CPPWINRT_PREBUILD_TOOL does not exist: ${CPPWINRT_PREBUILD_TOOL}")
    endif()
    set(PREBUILD_TOOL "${CPPWINRT_PREBUILD_TOOL}")
    set(PREBUILD_TOOL_DEPENDS "${CPPWINRT_PREBUILD_TOOL}")
elseif(CMAKE_CROSSCOMPILING)
    include(ExternalProject)
    ExternalProject_Add(cppwinrt-prebuild
        SOURCE_DIR "${PROJECT_SOURCE_DIR}/prebuild"
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> "-DCPPWINRT_BUILD_VERSION=${CPPWINRT_BUILD_VERSION}"
    )
    ExternalProject_Get_Property(cppwinrt-prebuild INSTALL_DIR)
    set(PREBUILD_TOOL "${INSTALL_DIR}/bin/cppwinrt-prebuild")
    set(PREBUILD_TOOL_DEPENDS cppwinrt-prebuild)
    unset(INSTALL_DIR)
else()
    add_subdirectory(prebuild)
    set(PREBUILD_TOOL cppwinrt-prebuild)
    set(PREBUILD_TOOL_DEPENDS cppwinrt-prebuild)
endif()


# ---------------------------------------------------------------------------
# === Step to create autogenerated files ===
# ---------------------------------------------------------------------------

file(GLOB PREBUILD_STRINGS_FILES
    LIST_DIRECTORIES false
    CONFIGURE_DEPENDS
    strings/*.h
)
add_custom_command(
    OUTPUT
        ${PROJECT_BINARY_DIR}/strings.cpp
        ${PROJECT_BINARY_DIR}/version.rc
    COMMAND "${PREBUILD_TOOL}" ARGS "${PROJECT_SOURCE_DIR}/strings" "${PROJECT_BINARY_DIR}"
    DEPENDS
        ${PREBUILD_TOOL_DEPENDS}
        ${PREBUILD_STRINGS_FILES}
    VERBATIM
)


# === cppwinrt ===

set(CPPWINRT_SRCS
    cppwinrt/main.cpp
    "${PROJECT_BINARY_DIR}/strings.cpp"
)

set(CPPWINRT_HEADERS
    cppwinrt/pch.h
    cppwinrt/cmd_reader.h
    cppwinrt/code_writers.h
    cppwinrt/component_writers.h
    cppwinrt/file_writers.h
    cppwinrt/helpers.h
    cppwinrt/settings.h
    cppwinrt/task_group.h
    cppwinrt/text_writer.h
    cppwinrt/type_writers.h
)

if(WIN32)
    add_custom_command(
        OUTPUT
            "${PROJECT_BINARY_DIR}/app.manifest"
        COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/cppwinrt/app.manifest" "${PROJECT_BINARY_DIR}/app.manifest"
        DEPENDS "${PROJECT_SOURCE_DIR}/cppwinrt/app.manifest"
        VERBATIM
    )
    if(MSVC)
        set(CPPWINRT_RESOURCES
            "${PROJECT_BINARY_DIR}/app.manifest"
            "${PROJECT_BINARY_DIR}/version.rc"
        )
    else()
        # Do the configure_file dance so that app.manifest.rc doesn't get modified
        # every single time the project is reconfigured and trigger a rebuild.
        file(WRITE "${PROJECT_BINARY_DIR}/app.manifest.rc.in" "1 24 \"app.manifest\"\n")
        configure_file(
            "${PROJECT_BINARY_DIR}/app.manifest.rc.in"
            "${PROJECT_BINARY_DIR}/app.manifest.rc"
            COPYONLY
        )

        set(CPPWINRT_RESOURCES
            "${PROJECT_BINARY_DIR}/app.manifest"
            "${PROJECT_BINARY_DIR}/app.manifest.rc"
            "${PROJECT_BINARY_DIR}/version.rc"
        )
    endif()
endif()

add_executable(cppwinrt ${CPPWINRT_SRCS} ${CPPWINRT_RESOURCES} ${CPPWINRT_HEADERS})
target_compile_definitions(cppwinrt PRIVATE CPPWINRT_VERSION_STRING="${CPPWINRT_BUILD_VERSION}")
target_include_directories(cppwinrt PRIVATE ${PROJECT_BINARY_DIR})

if(WIN32)
    target_link_libraries(cppwinrt PRIVATE shlwapi)
endif()

install(TARGETS cppwinrt)

if(NOT CPPWINRT_HOST_EXE STREQUAL "")
    if(NOT EXISTS "${CPPWINRT_HOST_EXE}")
        message(FATAL_ERROR "CPPWINRT_HOST_EXE does not exist: ${CPPWINRT_HOST_EXE}")
    endif()
    set(CPPWINRT_TOOL_COMMAND "${CPPWINRT_HOST_EXE}")
    set(CPPWINRT_TOOL_DEPENDS "${CPPWINRT_HOST_EXE}")
else()
    set(CPPWINRT_TOOL_COMMAND "$<TARGET_FILE:cppwinrt>")
    set(CPPWINRT_TOOL_DEPENDS cppwinrt)
endif()


# ---------------------------------------------------------------------------
# xmllite import lib — mingw-w64 only; MSVC gets it from the SDK.
# ---------------------------------------------------------------------------

# mingw-w64 before commit 5ac1a2c is missing the import lib for xmllite. This
# checks whether the current build environment provides libxmllite.a, and
# generates the import lib if needed.

set(XMLLITE_LIBRARY xmllite)
if(MINGW)
    function(TestLinkXmlLite OUTPUT_VARNAME)
        include(CheckCXXSourceCompiles)
        set(CMAKE_REQUIRED_LIBRARIES xmllite)
        check_cxx_source_compiles("
#include <xmllite.h>
int main() {
    CreateXmlReader(__uuidof(IXmlReader), nullptr, nullptr);
}
        " ${OUTPUT_VARNAME})
    endfunction()

    function(TestIsI386 OUTPUT_VARNAME)
        include(CheckCXXSourceCompiles)
        check_cxx_source_compiles("
#if !defined(__i386__) && !defined(_M_IX86)
#  error Not i386
#endif
int main() {}
        " ${OUTPUT_VARNAME})
    endfunction()

    TestLinkXmlLite(HAS_LIBXMLLITE)
    if(NOT HAS_LIBXMLLITE)
        TestIsI386(TARGET_IS_I386)
        if(TARGET_IS_I386)
            set(XMLLITE_DEF_FILE xmllite_i386)
        else()
            set(XMLLITE_DEF_FILE xmllite)
        endif()
        include(CMakeFindBinUtils)
        add_custom_command(
            OUTPUT
                "${PROJECT_BINARY_DIR}/libxmllite.a"
            COMMAND "${CMAKE_DLLTOOL}" -k -d "${PROJECT_SOURCE_DIR}/mingw-support/${XMLLITE_DEF_FILE}.def" -l "${PROJECT_BINARY_DIR}/libxmllite.a"
            DEPENDS "${PROJECT_SOURCE_DIR}/mingw-support/${XMLLITE_DEF_FILE}.def"
            VERBATIM
        )
        add_custom_target(gen-libxmllite
            DEPENDS "${PROJECT_BINARY_DIR}/libxmllite.a"
        )
        set(XMLLITE_LIBRARY "${PROJECT_BINARY_DIR}/libxmllite.a")
        add_dependencies(cppwinrt gen-libxmllite)
    endif()
endif()
if(WIN32)
    target_link_libraries(cppwinrt PRIVATE "${XMLLITE_LIBRARY}")
endif()


# ---------------------------------------------------------------------------
# === winmd: External header-only library for reading winmd files ===
# ---------------------------------------------------------------------------

set(EXTERNAL_WINMD_INCLUDE_DIR "" CACHE PATH "Path to the include dir of an\
 external copy of the winmd library headers. Leave empty (default) to have\
 it downloaded as ExternalProject during build.")

if(EXTERNAL_WINMD_INCLUDE_DIR STREQUAL "")
    message(STATUS "The winmd library will be downloaded using ExternalProject.")
    include(ExternalProject)
    ExternalProject_Add(winmd
        GIT_REPOSITORY https://github.com/microsoft/winmd.git
        GIT_TAG 0f1eae3bfa63fa2ba3c2912cbfe72a01db94cc5a
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        UPDATE_COMMAND ""
    )
    add_dependencies(cppwinrt winmd)
    ExternalProject_Get_Property(winmd SOURCE_DIR)
    set(winmd_INCLUDE_DIR "${SOURCE_DIR}/src")
else()
    message(STATUS "Using winmd library headers at ${EXTERNAL_WINMD_INCLUDE_DIR}")
    set(winmd_INCLUDE_DIR "${EXTERNAL_WINMD_INCLUDE_DIR}")
endif()
target_include_directories(cppwinrt PRIVATE "${winmd_INCLUDE_DIR}")


# ---------------------------------------------------------------------------
# === fast_fwd static library (MASM / MARMASM) ===
# ---------------------------------------------------------------------------
if(MSVC)
    add_subdirectory(fast_fwd)
endif()


# ---------------------------------------------------------------------------
# === scratch developer sandbox ===
# ---------------------------------------------------------------------------
if(MSVC)
    add_subdirectory(scratch)
endif()


# ---------------------------------------------------------------------------
# Load the MIDL + cppwinrt helper functions used by test component targets.
# ---------------------------------------------------------------------------
if(WIN32 AND MSVC)
    list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
    include(CppWinRTHelpers)
endif()


# ---------------------------------------------------------------------------
# === Tests ===
# ---------------------------------------------------------------------------
if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(test)
    endif()
endif()
