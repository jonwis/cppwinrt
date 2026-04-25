# test_module_lock_none — tests the no-module-lock code path
# Depends on test_component_base and test_component_derived DLLs being present.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(test_module_lock_none main.cpp)

target_compile_definitions(test_module_lock_none PRIVATE NOMINMAX _MBCS)

target_include_directories(test_module_lock_none PRIVATE
    "${CMAKE_SOURCE_DIR}/test"
    "${CMAKE_SOURCE_DIR}/cppwinrt"
    "${CMAKE_BINARY_DIR}/test/test_component_base"
    "${CMAKE_BINARY_DIR}/test/test_component_derived"
)

add_dependencies(test_module_lock_none test_component_base test_component_derived)

add_test(
    NAME test_module_lock_none
    COMMAND "$<TARGET_FILE:test_module_lock_none>"
)
