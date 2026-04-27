file(GLOB TEST_SRCS
    LIST_DIRECTORIES false
    CONFIGURE_DEPENDS
    *.cpp
)
list(FILTER TEST_SRCS EXCLUDE REGEX "/(main|pch)\\.cpp")


# We can't build test_component[*] for mingw-w64 because it doesn't have an
# alternative to midl that can produce winmd files. Also, even if we do manage
# to reuse the MSVC-compiled binaries, mingw-w64 is still missing
# windowsnumerics.impl.h which is needed to provide the types
# winrt::Windows::Foundation::Numerics::float2 and friends that the components
# use.
list(APPEND BROKEN_TESTS
    agility
    delegates
    enum
    event_deferral
    in_params
    in_params_abi
    no_make_detection
    noexcept
    optional
    out_params
    out_params_abi
    parent_includes
    rational
    return_params
    return_params_abi
    struct_delegate
    structs
    uniform_in_params
    velocity
)

list(APPEND BROKEN_TESTS
    # depends on pplawait.h
    when
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # FIXME: GCC does not compile co_await on thread_pool because it wants
    # a copy constructor. Disabling this test for now.
    # This might be related to upstream bug:
    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103963
    list(APPEND BROKEN_TESTS
        thread_pool
    )
endif()

# Exclude broken tests
foreach(TEST_SRCS_EXCLUDE_ITEM IN LISTS BROKEN_TESTS)
    list(FILTER TEST_SRCS EXCLUDE REGEX "/${TEST_SRCS_EXCLUDE_ITEM}\\.cpp")
endforeach()

add_executable(test-vanilla main.cpp ${TEST_SRCS})
set_target_properties(test-vanilla PROPERTIES OUTPUT_NAME "test")
target_link_libraries(test-vanilla runtimeobject synchronization)
target_compile_definitions(test-vanilla PRIVATE
    CPPWINRT_TEST_BASE_PROJECTION_DIR="${CPPWINRT_PROJECTION_INCLUDE_DIR}"
    CPPWINRT_TEST_RANGE_PROJECTION_DIR="${CPPWINRT_RANGE_PROJECTION_INCLUDE_DIR}"
)

target_precompile_headers(test-vanilla PRIVATE pch.h)
set_source_files_properties(
    main.cpp
    coro_foundation.cpp
    coro_system.cpp
    coro_threadpool.cpp
    coro_uicore.cpp
    custom_activation.cpp
    generic_type_names.cpp
    guid_include.cpp
    inspectable_interop.cpp
    module_lock_dll.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS true
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC seems to miscompile out_params_bad.cpp if -fdevirtualize is enabled.
    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108040
    set_source_files_properties(out_params_bad.cpp PROPERTIES COMPILE_OPTIONS "-fno-devirtualize")

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # GCC miscompiles multi_threaded_vector.cpp with -O3
        set_source_files_properties(multi_threaded_vector.cpp PROPERTIES COMPILE_OPTIONS "-O2")
    endif()
endif()

add_dependencies(test-vanilla build-cppwinrt-projection)
add_dependencies(test-vanilla build-cppwinrt-range-projection)

add_test(
    NAME test
    COMMAND "$<TARGET_FILE:test-vanilla>" ${TEST_COLOR_ARG}
)

add_executable(test-projection-range main.cpp projection_range.cpp)
set_target_properties(test-projection-range PROPERTIES OUTPUT_NAME "test-projection-range")
target_link_libraries(test-projection-range runtimeobject synchronization)
target_compile_definitions(test-projection-range PRIVATE
    CPPWINRT_TEST_BASE_PROJECTION_DIR="${CPPWINRT_PROJECTION_INCLUDE_DIR}"
    CPPWINRT_TEST_RANGE_PROJECTION_DIR="${CPPWINRT_RANGE_PROJECTION_INCLUDE_DIR}"
)
target_precompile_headers(test-projection-range PRIVATE pch.h)
set_source_files_properties(
    main.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS true
)
add_dependencies(test-projection-range build-cppwinrt-projection)
add_dependencies(test-projection-range build-cppwinrt-range-projection)

add_test(
    NAME test-projection-range
    COMMAND "$<TARGET_FILE:test-projection-range>" ${TEST_COLOR_ARG} "[projection-range]"
)

add_executable(test-projection-range-plan-full main.cpp projection_range_plan_full.cpp)
set_target_properties(test-projection-range-plan-full PROPERTIES OUTPUT_NAME "test-projection-range-plan-full")
target_link_libraries(test-projection-range-plan-full runtimeobject synchronization)
target_include_directories(test-projection-range-plan-full PRIVATE
    "${CPPWINRT_RANGE_PLAN_FULL_PROJECTION_INCLUDE_DIR}"
    "${CPPWINRT_PROJECTION_INCLUDE_DIR}"
)
target_precompile_headers(test-projection-range-plan-full PRIVATE pch.h)
set_source_files_properties(
    main.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS true
)
add_dependencies(test-projection-range-plan-full build-cppwinrt-range-plan-projection-full)

add_test(
    NAME test-projection-range-plan-full
    COMMAND "$<TARGET_FILE:test-projection-range-plan-full>" ${TEST_COLOR_ARG} "[projection-range-plan]"
)

add_executable(test-projection-range-plan-filtered main.cpp projection_range_plan_filtered.cpp)
set_target_properties(test-projection-range-plan-filtered PROPERTIES OUTPUT_NAME "test-projection-range-plan-filtered")
target_link_libraries(test-projection-range-plan-filtered runtimeobject synchronization)
target_include_directories(test-projection-range-plan-filtered PRIVATE
    "${CPPWINRT_RANGE_PLAN_FILTERED_PROJECTION_INCLUDE_DIR}"
    "${CPPWINRT_PROJECTION_INCLUDE_DIR}"
)
target_compile_definitions(test-projection-range-plan-filtered PRIVATE
    CPPWINRT_RANGE_PLAN_FILTERED_HEADER="${CPPWINRT_RANGE_PLAN_FILTERED_HEADER}"
)
target_precompile_headers(test-projection-range-plan-filtered PRIVATE pch.h)
set_source_files_properties(
    main.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS true
)
add_dependencies(test-projection-range-plan-filtered build-cppwinrt-range-plan-projection-filtered)

add_test(
    NAME test-projection-range-plan-filtered
    COMMAND "$<TARGET_FILE:test-projection-range-plan-filtered>" ${TEST_COLOR_ARG} "[projection-range-plan]"
)
