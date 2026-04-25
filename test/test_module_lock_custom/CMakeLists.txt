# test_module_lock_custom — tests custom module lock implementation
# Single source, no PCH, no component dependencies.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(test_module_lock_custom main.cpp)

target_compile_definitions(test_module_lock_custom PRIVATE NOMINMAX _MBCS)

target_include_directories(test_module_lock_custom PRIVATE
    "${CMAKE_SOURCE_DIR}/test"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
)

add_dependencies(test_module_lock_custom build-cppwinrt-projection)
target_include_directories(test_module_lock_custom PRIVATE "${CMAKE_BINARY_DIR}/test/cppwinrt")

add_test(
    NAME test_module_lock_custom
    COMMAND "$<TARGET_FILE:test_module_lock_custom>"
)
