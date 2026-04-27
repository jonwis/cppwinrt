@echo off
setlocal

set target_configuration=%1
set target_version=%2
set target_deployment=%3

if "%target_configuration%"=="" set target_configuration=Release
if "%target_version%"=="" set target_version=999.999.999.999
if "%target_deployment%"=="" set target_deployment=Standalone

set repo_dir=%~dp0
set package_output=%repo_dir%build\packages
set MSBUILD_EXE=

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
	for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find MSBuild\**\Bin\MSBuild.exe`) do (
		set "MSBUILD_EXE=%%I"
	)
)

if "%MSBUILD_EXE%"=="" (
	for /f "usebackq delims=" %%I in (`where msbuild 2^>nul`) do (
		if "%MSBUILD_EXE%"=="" set "MSBUILD_EXE=%%I"
	)
)

if "%MSBUILD_EXE%"=="" (
	echo.
	echo MSBuild.exe was not found. Install Visual Studio Build Tools or run from a Developer Command Prompt.
	exit /b 1
)

if not exist ".\.nuget" mkdir ".\.nuget"
if not exist ".\.nuget\nuget.exe" powershell -Command "$ProgressPreference = 'SilentlyContinue' ; Invoke-WebRequest https://dist.nuget.org/win-x86-commandline/latest/nuget.exe -OutFile .\.nuget\nuget.exe"

call .nuget\nuget.exe restore natvis\cppwinrtvisualizer.sln
if %ERRORLEVEL% NEQ 0 goto :error
call .nuget\nuget.exe restore test\nuget\NuGetTest.sln
if %ERRORLEVEL% NEQ 0 goto :error

echo.
echo === Configuring CMake presets ===
cmake --preset msvc-x86 -DCPPWINRT_BUILD_VERSION=%target_version%
if %ERRORLEVEL% NEQ 0 goto :error
cmake --preset msvc-x64 -DCPPWINRT_BUILD_VERSION=%target_version%
if %ERRORLEVEL% NEQ 0 goto :error
cmake --preset msvc-arm64 -DCPPWINRT_BUILD_VERSION=%target_version%
if %ERRORLEVEL% NEQ 0 goto :error

echo.
echo === Building CMake artifacts (cppwinrt + fast forwarder libs) ===
cmake --build build\msvc-x86 --config %target_configuration% --target cppwinrt cppwinrt_fast_forwarder -j
if %ERRORLEVEL% NEQ 0 goto :error
cmake --build build\msvc-x64 --config %target_configuration% --target cppwinrt_fast_forwarder -j
if %ERRORLEVEL% NEQ 0 goto :error
cmake --build build\msvc-arm64 --config %target_configuration% --target cppwinrt_fast_forwarder -j
if %ERRORLEVEL% NEQ 0 goto :error

if not exist "%package_output%" mkdir "%package_output%"

rem Build cppwinrt visualizer dll for x86, x64, and arm64
call "%MSBUILD_EXE%" /p:Configuration=%target_configuration%,Platform=x64,Deployment=%target_deployment%,CppWinRTBuildVersion=%target_version% natvis\cppwinrtvisualizer.sln
if %ERRORLEVEL% NEQ 0 goto :error
call "%MSBUILD_EXE%" /p:Configuration=%target_configuration%,Platform=x86,Deployment=%target_deployment%,CppWinRTBuildVersion=%target_version% natvis\cppwinrtvisualizer.sln
if %ERRORLEVEL% NEQ 0 goto :error
call "%MSBUILD_EXE%" /p:Configuration=%target_configuration%,Platform=arm64,Deployment=%target_deployment%,CppWinRTBuildVersion=%target_version% natvis\cppwinrtvisualizer.sln
if %ERRORLEVEL% NEQ 0 goto :error

echo.
echo === Packing NuGet package ===
.\.nuget\nuget.exe pack nuget\Microsoft.Windows.CppWinRT.nuspec -NonInteractive -OutputDirectory "%package_output%" -Properties Configuration=%target_configuration%;target_version=%target_version%;cppwinrt_exe=%repo_dir%build\msvc-x86\%target_configuration%\cppwinrt.exe;cppwinrt_fast_fwd_x86=%repo_dir%build\msvc-x86\%target_configuration%\cppwinrt_fast_forwarder.lib;cppwinrt_fast_fwd_x64=%repo_dir%build\msvc-x64\%target_configuration%\cppwinrt_fast_forwarder.lib;cppwinrt_fast_fwd_arm64=%repo_dir%build\msvc-arm64\%target_configuration%\cppwinrt_fast_forwarder.lib -version %target_version% -Verbosity Detailed
if %ERRORLEVEL% NEQ 0 goto :error

rem Build vsix
call "%MSBUILD_EXE%" /restore /p:Configuration=%target_configuration%,Platform="Any CPU",Deployment=%target_deployment%,CppWinRTVersion=%target_version%,NugetPackageVersion=%target_version%,NatvisDirx86=%repo_dir%natvis\x86\%target_configuration%\%target_deployment%,NatvisDirx64=%repo_dir%natvis\x64\%target_configuration%\%target_deployment%,NatvisDirarm64=%repo_dir%natvis\arm64\%target_configuration%\%target_deployment%,NupkgDir=%package_output% vsix\vsix.sln
if %ERRORLEVEL% NEQ 0 goto :error

goto :eof

:error
echo.
echo *** build_packages.cmd failed with error %ERRORLEVEL% ***
exit /b %ERRORLEVEL%
