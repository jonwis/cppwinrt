# cmake/CppWinRTHelpers.cmake
#
# Helper functions for building WinRT test components with MIDL + cppwinrt.
# Only meaningful on Windows (MSVC / ClangCL) builds.

cmake_minimum_required(VERSION 4.2)
cmake_policy(VERSION 4.2)
include_guard(GLOBAL)

# ---------------------------------------------------------------------------
# winrt_midl_compile
#
# Run midl.exe on an IDL file to produce a .winmd metadata file.
#
# Parameters
#   TARGET        – the CMake target that depends on the .winmd output
#   IDL_FILE      – source IDL path (absolute or relative to calling dir)
#   WINMD_OUT     – desired absolute output path for the .winmd file
#   REF_WINMDS    – (optional multi-value) extra /reference winmd paths
#   HEADER_OUT    – header file to emit (pass "nul" to suppress)
#   EXTRA_DEPENDS – (optional multi-value) additional files the IDL depends on
#                   (e.g. imported IDL files resolved via EXTRA_IDL_DIRS)
#
# The caller must declare the WINMD_OUT path as a source with the GENERATED
# property so CMake doesn't complain at configure time.
# ---------------------------------------------------------------------------
function(winrt_midl_compile)
    cmake_parse_arguments(MIDL "" "TARGET;IDL_FILE;WINMD_OUT;HEADER_OUT" "REF_WINMDS;EXTRA_IDL_DIRS;EXTRA_DEPENDS" ${ARGN})

    if(NOT MIDL_HEADER_OUT)
        set(MIDL_HEADER_OUT "nul")
    endif()

    find_program(MIDL_EXECUTABLE midl REQUIRED
        HINTS
            "C:/Program Files (x86)/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64"
            "C:/Program Files (x86)/Windows Kits/10/bin/x64"
    )

    # Map the active target architecture to midl /env.
    if(CPPWINRT_TARGET_ARCH STREQUAL "x86")
        set(_midl_env "win32")
    elseif(CPPWINRT_TARGET_ARCH STREQUAL "arm64")
        set(_midl_env "arm64")
    else()
        set(_midl_env "amd64")
    endif()

    # Windows metadata directory
    set(_meta_dir "$ENV{SystemRoot}/System32/WinMetadata")

    # Build the reference list
    set(_ref_args)
    foreach(_ref IN LISTS MIDL_REF_WINMDS)
        list(APPEND _ref_args /reference "${_ref}")
    endforeach()

    # Make IDL absolute - use backslashes for Windows
    get_filename_component(_idl_abs "${MIDL_IDL_FILE}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    string(REPLACE "/" "\\" _idl_abs "${_idl_abs}")
    
    get_filename_component(_winmd_dir "${MIDL_WINMD_OUT}" DIRECTORY)
    string(REPLACE "/" "\\" _winmd_dir "${_winmd_dir}")
    
    string(REPLACE "/" "\\" _src_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    string(REPLACE "/" "\\" _meta_dir "${_meta_dir}")

    if(NOT MIDL_HEADER_OUT STREQUAL "nul")
        string(REPLACE "/" "\\" MIDL_HEADER_OUT "${MIDL_HEADER_OUT}")
    endif()

    set(_extra_inc_args)
    foreach(_idir IN LISTS MIDL_EXTRA_IDL_DIRS)
        string(REPLACE "/" "\\" _idir_bs "${_idir}")
        list(APPEND _extra_inc_args /I "${_idir_bs}")
    endforeach()

    add_custom_command(
        OUTPUT  "${MIDL_WINMD_OUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_winmd_dir}"
        COMMAND "${MIDL_EXECUTABLE}"
                /winrt
                /W1 /nologo
                /nomidl
                /env "${_midl_env}"
                /metadata_dir "${_meta_dir}"
                /h "${MIDL_HEADER_OUT}"
                /out "${_winmd_dir}"
                /I "${_src_dir}"
                ${_ref_args}
                ${_extra_inc_args}
                "${_idl_abs}"
        DEPENDS
            "${_idl_abs}"
            ${MIDL_EXTRA_DEPENDS}
        COMMENT "MIDL: ${MIDL_IDL_FILE}"
        VERBATIM
    )

    set_source_files_properties("${MIDL_WINMD_OUT}" PROPERTIES GENERATED TRUE)
endfunction()


# ---------------------------------------------------------------------------
# winrt_cppwinrt_component
#
# Run cppwinrt.exe to generate component projection + implementation stubs
# (module.g.cpp and friends) from a .winmd file.
#
# Parameters
#   TARGET        – the CMake target that will compile the generated sources
#   WINMD         – absolute path to the input .winmd
#   GENERATED_DIR – absolute path for the "Generated Files" output dir
#   EXTRA_ARGS    – (optional multi-value) forwarded verbatim to cppwinrt
#   REF_WINMDS    – (optional multi-value) extra -ref winmd paths
#   DEPENDS_WINMDS– (optional multi-value) additional winmd files the command
#                   depends on (beyond WINMD itself)
# ---------------------------------------------------------------------------
function(winrt_cppwinrt_component)
    cmake_parse_arguments(COMP "" "TARGET;WINMD;GENERATED_DIR" "EXTRA_ARGS;REF_WINMDS;DEPENDS_WINMDS" ${ARGN})

    set(_stubs_dir "${COMP_GENERATED_DIR}/stubs")
    set(_module_g  "${COMP_GENERATED_DIR}/module.g.cpp")

    set(_ref_args)
    foreach(_ref IN LISTS COMP_REF_WINMDS)
        list(APPEND _ref_args -ref "${_ref}")
    endforeach()

    add_custom_command(
        OUTPUT  "${_module_g}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${COMP_GENERATED_DIR}"
        COMMAND "${CPPWINRT_TOOL_COMMAND}"
                -input "${COMP_WINMD}"
                ${_ref_args}
                -ref sdk
                -comp "${_stubs_dir}"
                -out  "${COMP_GENERATED_DIR}"
                -verbose
                ${COMP_EXTRA_ARGS}
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${_stubs_dir}/module.g.cpp"
                "${_module_g}"
        DEPENDS
            ${CPPWINRT_TOOL_DEPENDS}
            "${COMP_WINMD}"
            ${COMP_DEPENDS_WINMDS}
        COMMENT "cppwinrt component stubs: ${COMP_TARGET}"
        VERBATIM
    )

    set_source_files_properties("${_module_g}" PROPERTIES GENERATED TRUE)
endfunction()


# ---------------------------------------------------------------------------
# winrt_cppwinrt_projection_target
#
# Create a custom target that generates projection headers from a .winmd input
# using cppwinrt and tracks completion with a stamp file.
#
# Parameters
#   NAME        – custom target name to create
#   WINMD       – absolute path to input .winmd
#   OUTPUT_DIR  – output directory passed to cppwinrt -out
#   STAMP_FILE  – (optional) explicit stamp file path
#   FASTABI     – (optional flag) add -fastabi
#   REF_WINMDS  – (optional multi-value) additional -ref winmd paths
#   DEPENDS     – (optional multi-value) additional dependencies
#   BYPRODUCTS  – (optional multi-value) generated files for build tracking
#   EXTRA_ARGS  – (optional multi-value) additional cppwinrt args
# ---------------------------------------------------------------------------
function(winrt_cppwinrt_projection_target)
    cmake_parse_arguments(PROJ "FASTABI" "NAME;WINMD;OUTPUT_DIR;STAMP_FILE" "REF_WINMDS;DEPENDS;BYPRODUCTS;EXTRA_ARGS" ${ARGN})

    if(NOT PROJ_NAME)
        message(FATAL_ERROR "winrt_cppwinrt_projection_target: NAME is required")
    endif()
    if(NOT PROJ_WINMD)
        message(FATAL_ERROR "winrt_cppwinrt_projection_target: WINMD is required")
    endif()
    if(NOT PROJ_OUTPUT_DIR)
        message(FATAL_ERROR "winrt_cppwinrt_projection_target: OUTPUT_DIR is required")
    endif()

    if(PROJ_STAMP_FILE)
        set(_projection_stamp "${PROJ_STAMP_FILE}")
    else()
        set(_projection_stamp "${PROJ_OUTPUT_DIR}/${PROJ_NAME}.stamp")
    endif()

    set(_ref_args)
    foreach(_ref IN LISTS PROJ_REF_WINMDS)
        list(APPEND _ref_args -ref "${_ref}")
    endforeach()

    set(_fastabi_args)
    if(PROJ_FASTABI)
        list(APPEND _fastabi_args -fastabi)
    endif()

    add_custom_command(
        OUTPUT  "${_projection_stamp}"
        BYPRODUCTS ${PROJ_BYPRODUCTS}
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJ_OUTPUT_DIR}"
        COMMAND "${CPPWINRT_TOOL_COMMAND}"
                -input "${PROJ_WINMD}"
                -out   "${PROJ_OUTPUT_DIR}"
                ${_ref_args}
                -ref sdk
                ${_fastabi_args}
                -verbose
                ${PROJ_EXTRA_ARGS}
        COMMAND "${CMAKE_COMMAND}" -E touch "${_projection_stamp}"
        DEPENDS
            ${CPPWINRT_TOOL_DEPENDS}
            "${PROJ_WINMD}"
            ${PROJ_DEPENDS}
        COMMENT "cppwinrt: generating projection for ${PROJ_NAME}"
        VERBATIM
    )

    set_source_files_properties("${_projection_stamp}" PROPERTIES GENERATED TRUE)
    add_custom_target(${PROJ_NAME} DEPENDS "${_projection_stamp}")
endfunction()
