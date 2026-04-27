@echo off
setlocal EnableExtensions

if "%~2"=="" (
    echo Usage: ensure_vs_env.cmd ^<target-arch^> ^<script^> [args...] 1>&2
    exit /b 2
)

set "target_arch=%~1"
set "script_to_run=%~f2"
shift
shift

call :normalize_arch "%target_arch%"
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
set "target_arch=%normalized_arch%"

set "current_arch="
if defined VSCMD_ARG_TGT_ARCH (
    call :normalize_arch "%VSCMD_ARG_TGT_ARCH%"
    if %ERRORLEVEL% EQU 0 set "current_arch=%normalized_arch%"
)

set "vsdevcmd=%CPPWINRT_VSDEVCMD%"
if not defined vsdevcmd if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find Common7\Tools\VsDevCmd.bat`) do (
        if not defined vsdevcmd set "vsdevcmd=%%~fI"
    )
)
if not defined vsdevcmd (
    for /f "usebackq delims=" %%I in (`where VsDevCmd.bat 2^>nul`) do (
        if not defined vsdevcmd set "vsdevcmd=%%~fI"
    )
)

if defined VSCMD_VER if /i "%current_arch%"=="%target_arch%" (
    endlocal & set "CPPWINRT_VS_ENV_CONTINUE=1" & set "CPPWINRT_VSDEVCMD=%vsdevcmd%" & exit /b 0
)

if /i "%CPPWINRT_VS_ENV_READY%"=="1" (
    echo Failed to initialize a matching Visual Studio tools environment for %target_arch%. 1>&2
    exit /b 1
)

if not defined vsdevcmd (
    echo Could not find VsDevCmd.bat. Install Visual Studio Build Tools or run from a Developer Command Prompt. 1>&2
    exit /b 1
)

set "relaunch_args="
:collect_args
if "%~1"=="" goto :relaunch
set "relaunch_args=%relaunch_args% %~1"
shift
goto :collect_args

:relaunch
echo Initializing Visual Studio tools for %target_arch%...
endlocal & cmd /d /s /c ""%vsdevcmd%" -no_logo -host_arch=x64 -arch=%target_arch% && set CPPWINRT_VS_ENV_READY=1 && set CPPWINRT_VSDEVCMD=%vsdevcmd% && call "%script_to_run%"%relaunch_args%"
exit /b %ERRORLEVEL%

:normalize_arch
set "normalized_arch="
if /i "%~1"=="x86" set "normalized_arch=x86" & exit /b 0
if /i "%~1"=="win32" set "normalized_arch=x86" & exit /b 0
if /i "%~1"=="i386" set "normalized_arch=x86" & exit /b 0
if /i "%~1"=="i686" set "normalized_arch=x86" & exit /b 0
if /i "%~1"=="x64" set "normalized_arch=x64" & exit /b 0
if /i "%~1"=="amd64" set "normalized_arch=x64" & exit /b 0
if /i "%~1"=="x86_64" set "normalized_arch=x64" & exit /b 0
if /i "%~1"=="arm64" set "normalized_arch=arm64" & exit /b 0
if /i "%~1"=="arm64ec" set "normalized_arch=arm64" & exit /b 0
echo Unsupported target architecture "%~1". 1>&2
exit /b 1
