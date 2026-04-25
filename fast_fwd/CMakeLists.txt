# fast_fwd — cppwinrt_fast_forwarder static library
#
# Contains per-architecture assembly thunks. MASM is used for x86/x64;
# ARM64 requires preprocessing with cl.exe before the MARMASM pass (exactly
# as the original fast_fwd.vcxproj did).

# Determine target architecture from the VS generator platform.
string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" _arch)

if(_arch STREQUAL "win32")
    set(_asm_src "${CMAKE_CURRENT_SOURCE_DIR}/win32/thunks.asm")
    enable_language(ASM_MASM)
    add_library(cppwinrt_fast_forwarder STATIC "${_asm_src}")
    set_source_files_properties("${_asm_src}" PROPERTIES
        LANGUAGE ASM_MASM
        COMPILE_OPTIONS "/safeseh /I\"${CMAKE_CURRENT_SOURCE_DIR}\""   # x86 safe SEH + thunks.inc include path
    )
elseif(_arch STREQUAL "x64")
    set(_asm_src "${CMAKE_CURRENT_SOURCE_DIR}/x64/thunks.asm")
    enable_language(ASM_MASM)
    add_library(cppwinrt_fast_forwarder STATIC "${_asm_src}")
    set_source_files_properties("${_asm_src}" PROPERTIES
        LANGUAGE ASM_MASM
        COMPILE_OPTIONS "/I\"${CMAKE_CURRENT_SOURCE_DIR}\""
    )
elseif(_arch STREQUAL "arm64")
    # ARM64: preprocess the ASM source with cl /E, then assemble with MARMASM.
    # This replicates the CppWinRTPreprocessArmThunk custom target in the .vcxproj.
    enable_language(ASM_MARMASM)

    set(_asm_src   "${CMAKE_CURRENT_SOURCE_DIR}/arm64/thunks.asm")
    set(_pp_output "${CMAKE_CURRENT_BINARY_DIR}/thunks.pp")

    add_custom_command(
        OUTPUT  "${_pp_output}"
        COMMAND cl /nologo
                   /I "${CMAKE_CURRENT_SOURCE_DIR}"
                   /E /Tc "${_asm_src}"
                   > "${_pp_output}"
        DEPENDS "${_asm_src}"
                "${CMAKE_CURRENT_SOURCE_DIR}/thunks.inc"
        COMMENT "Preprocessing ARM64 thunks (cl /E)"
        VERBATIM
    )

    add_library(cppwinrt_fast_forwarder STATIC "${_pp_output}")
    set_source_files_properties("${_pp_output}" PROPERTIES
        LANGUAGE ASM_MARMASM
        GENERATED TRUE
    )
else()
    message(WARNING "fast_fwd: unrecognised platform '${CMAKE_GENERATOR_PLATFORM}'; "
                    "skipping cppwinrt_fast_forwarder")
    add_library(cppwinrt_fast_forwarder INTERFACE)
endif()

set_target_properties(cppwinrt_fast_forwarder PROPERTIES OUTPUT_NAME "cppwinrt_fast_forwarder")

install(TARGETS cppwinrt_fast_forwarder
    ARCHIVE DESTINATION lib
)
