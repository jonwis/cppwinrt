@echo off
setlocal

set target_platform=%1
set target_configuration=%2
set target_version=%3

if "%target_platform%"=="" set target_platform=x64
if "%target_configuration%"=="" set target_configuration=Release

if /i "%target_configuration%"=="debug" (
    set ctest_preset_suffix=debug
) else (
    set ctest_preset_suffix=release
)

set cmake_arch=%target_platform%
if /i "%target_platform%"=="win32" set cmake_arch=x86
set cmake_preset=msvc-%cmake_arch%

set ctest_preset=%cmake_preset%-%ctest_preset_suffix%

ctest --preset %ctest_preset% --output-on-failure -j %NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
