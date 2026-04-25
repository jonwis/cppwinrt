cmake_minimum_required(VERSION 4.2)
cmake_policy(VERSION 4.2)

project(cppwinrt-prebuild LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED CPPWINRT_BUILD_VERSION)
    message(FATAL_ERROR "CPPWINRT_BUILD_VERSION has not been defined. You should build the top-level project instead.")
endif()


set(PREBUILD_SRCS
    main.cpp
    pch.h
)
add_executable(cppwinrt-prebuild ${PREBUILD_SRCS})
target_compile_definitions(cppwinrt-prebuild PRIVATE CPPWINRT_VERSION_STRING="${CPPWINRT_BUILD_VERSION}")
target_include_directories(cppwinrt-prebuild PRIVATE "${PROJECT_SOURCE_DIR}/../cppwinrt")

if(PROJECT_IS_TOP_LEVEL)
    install(TARGETS cppwinrt-prebuild)
endif()
