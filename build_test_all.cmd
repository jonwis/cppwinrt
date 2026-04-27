@echo off
setlocal

rem build_test_all.cmd [platform] [config] [version] [clean_intermediate_files]
rem
rem  platform : x64 | x86 | arm64  (default: x64)
rem  config   : debug | release     (default: Release)
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

if /i "%target_configuration%"=="debug" (
    set target_configuration=Debug
    set ctest_configuration=debug
) else (
    set target_configuration=Release
    set ctest_configuration=release
)

set "CPPWINRT_VS_ENV_CONTINUE="
call "%~dp0ensure_vs_env.cmd" "%target_platform%" "%~f0" %*
if not defined CPPWINRT_VS_ENV_CONTINUE exit /b %ERRORLEVEL%
set "CPPWINRT_VS_ENV_CONTINUE="

if /i "%clean_intermediate_files%"=="clean" (
    echo Cleaning intermediate files...
    git clean -dfx _build\ >nul
)

set cmake_arch=%target_platform%
if /i "%target_platform%"=="win32" set cmake_arch=x86

set cmake_preset=msvc-%cmake_arch%
set "prebuild_override="
set "host_tool_override="

if /i "%target_platform%"=="arm64" (
    cmake --preset msvc-x64 -DCPPWINRT_BUILD_VERSION=%target_version% -DCPPWINRT_BUILD_HOST_TOOLS=ON
    cmake --build _build\msvc-x64 --config Release --target cppwinrt-prebuild cppwinrt -j
)

echo.
echo === Configuring [%cmake_preset%] version=%target_version% ===
cmake --preset %cmake_preset% -DCPPWINRT_BUILD_VERSION=%target_version% %prebuild_override% %host_tool_override%
if %ERRORLEVEL% NEQ 0 goto :error

echo.
echo === Building [%cmake_preset% / %target_configuration%] ===
cmake --build _build\%cmake_preset% --config %target_configuration% -j
if %ERRORLEVEL% NEQ 0 goto :error

rem Build NuGet integration tests through the CMake custom target (msbuild under the hood)
if /i not "%target_platform%"=="arm64" (
    echo.
    echo === Building NuGet integration tests ===
    cmake --build _build\%cmake_preset% --config %target_configuration% --target nuget_tests -j
)

rem ARM64 binaries are not runnable on most build hosts; skip execution
if /i "%target_platform%"=="arm64" goto :eof

echo.
echo === Running tests [%cmake_preset% / %target_configuration%] ===
ctest --preset %cmake_preset%-%ctest_configuration% --output-on-failure -j %NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% NEQ 0 goto :error

goto :eof

:error
echo.
echo *** Build failed with error %ERRORLEVEL% ***
exit /b %ERRORLEVEL%

:ensure_host_prebuild
if exist "%~dp0_build\x64\Release\cppwinrt-prebuild.exe" if exist "%~dp0_build\x64\Release\cppwinrt.exe" exit /b 0
call "%CPPWINRT_VSDEVCMD%" -no_logo -host_arch=x64 -arch=x64
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
cmake --preset msvc-x64 -DCPPWINRT_BUILD_VERSION=%target_version%
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
cmake --build _build\msvc-x64 --config Release --target cppwinrt cppwinrt-prebuild -j
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
call "%CPPWINRT_VSDEVCMD%" -no_logo -host_arch=x64 -arch=arm64
exit /b %ERRORLEVEL%
