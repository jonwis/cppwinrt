@echo off
setlocal

rem build_test_all.cmd [platform] [config] [version] [clean_intermediate_files]
rem
rem  platform : x64 | x86 | arm64  (default: x64)
rem  config   : Debug | Release     (default: Release)
rem  version  : build version string (default: 999.999.999.999)
rem
rem Uses cmake presets to configure/build, then ctest to execute tests.

set target_platform=%1
set target_configuration=%2
set target_version=%3
set clean_intermediate_files=%4

if "%target_platform%"==""      set target_platform=x64
if "%target_configuration%"=="" set target_configuration=Release
if "%target_version%"==""       set target_version=999.999.999.999

if /i "%clean_intermediate_files%"=="clean" (
    echo Cleaning intermediate files...
    git clean -dfx build/ >nul
)

set cmake_arch=%target_platform%
if /i "%target_platform%"=="win32" set cmake_arch=x86

set cmake_preset=msvc-%cmake_arch%

echo.
echo === Configuring [%cmake_preset%] version=%target_version% ===
cmake --preset %cmake_preset% -DCPPWINRT_BUILD_VERSION=%target_version%
if %ERRORLEVEL% NEQ 0 goto :error

echo.
echo === Building [%cmake_preset% / %target_configuration%] ===
cmake --build build\%cmake_preset% --config %target_configuration% -j
if %ERRORLEVEL% NEQ 0 goto :error

rem Build NuGet integration tests through the CMake custom target (msbuild under the hood)
if /i not "%target_platform%"=="arm64" (
    echo.
    echo === Building NuGet integration tests ===
    cmake --build build\%cmake_preset% --config %target_configuration% --target nuget_tests -j
)

rem ARM64 binaries are not runnable on most build hosts; skip execution
if /i "%target_platform%"=="arm64" goto :eof

echo.
echo === Running tests [%cmake_preset% / %target_configuration%] ===
ctest --preset %cmake_preset%-%target_configuration% --output-on-failure
if %ERRORLEVEL% NEQ 0 goto :error

goto :eof

:error
echo.
echo *** Build failed with error %ERRORLEVEL% ***
exit /b %ERRORLEVEL%
