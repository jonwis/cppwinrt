@echo off

setlocal ENABLEDELAYEDEXPANSION

set target_platform=%1
set target_configuration=%2
set target_version=%3
if "%target_platform%"=="" set target_platform=x64
if "%target_version%"=="" set target_version=999.999.999.999

if /I "%target_platform%" equ "all" (
  if "%target_configuration%"=="" (
    set target_configuration=all
  )
  call %0 x86 !target_configuration!
  call %0 x64 !target_configuration!
  call %0 arm64 !target_configuration!
  goto :eof
)

if /I "%target_configuration%" equ "all" (
  call %0 %target_platform% Debug
  call %0 %target_platform% Release
  goto :eof
)

if "%target_configuration%"=="" (
 set target_configuration=Debug
)

set repo_dir=%~dp0
set cppwinrt_exe=%repo_dir%build\msvc-x64\Release\cppwinrt.exe

if not exist "%cppwinrt_exe%" (
 echo Configuring and building cppwinrt tool via CMake...
 cmake --preset msvc-x64 -DCPPWINRT_BUILD_VERSION=%target_version%
 if %ERRORLEVEL% NEQ 0 goto :error
 cmake --build build\msvc-x64 --config Release --target cppwinrt -j
 if %ERRORLEVEL% NEQ 0 goto :error
)

set projection_output=%repo_dir%build\projection\%target_platform%\%target_configuration%
if not exist "%projection_output%" mkdir "%projection_output%"

echo Building projection into %projection_output%
"%cppwinrt_exe%" -in local -out "%projection_output%" -verbose
if %ERRORLEVEL% NEQ 0 goto :error
echo.

goto :eof

:error
echo.
echo *** build_projection.cmd failed with error %ERRORLEVEL% ***
exit /b %ERRORLEVEL%
