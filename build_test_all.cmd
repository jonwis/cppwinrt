@echo off

set target_platform=%1
set target_configuration=%2
set target_version=%3
set clean_intermediate_files=%4

if "%target_platform%"=="" set target_platform=x64
if "%target_configuration%"=="" set target_configuration=Release
if "%target_version%"=="" set target_version=999.999.999.999

if not exist ".\.nuget" mkdir ".\.nuget"
if not exist ".\.nuget\nuget.exe" powershell -Command "$ProgressPreference = 'SilentlyContinue' ; Invoke-WebRequest https://dist.nuget.org/win-x86-commandline/latest/nuget.exe -OutFile .\.nuget\nuget.exe"

call .nuget\nuget.exe restore cppwinrt.sln"
call .nuget\nuget.exe restore natvis\cppwinrtvisualizer.sln
call .nuget\nuget.exe restore test\nuget\NugetTest.sln

call msbuild /m /p:Configuration=%target_configuration%,Platform=%target_platform%,CppWinRTBuildVersion=%target_version% cppwinrt.sln /t:fast_fwd /verbosity:Minimal

call msbuild /p:Configuration=%target_configuration%,Platform=%target_platform%,Deployment=Component;CppWinRTBuildVersion=%target_version% natvis\cppwinrtvisualizer.sln /verbosity:Minimal
call msbuild /p:Configuration=%target_configuration%,Platform=%target_platform%,Deployment=Standalone;CppWinRTBuildVersion=%target_version% natvis\cppwinrtvisualizer.sln /verbosity:Minimal

if "%target_platform%"=="arm64" goto :eof

call msbuild /m /p:Configuration=%target_configuration%,Platform=%target_platform%,CppWinRTBuildVersion=%target_version% cppwinrt.sln /verbosity:Minimal /t:cppwinrt

call msbuild /p:Configuration=%target_configuration%,Platform=%target_platform%,CppWinRTBuildVersion=%target_version% test\nuget\NugetTest.sln /verbosity:Minimal

set test_targets=test\test
set test_targets=%test_targets%;test\test_nocoro
set test_targets=%test_targets%;test\test_cpp20
set test_targets=%test_targets%;test\test_cpp20_no_sourcelocation
set test_targets=%test_targets%;test\test_fast
set test_targets=%test_targets%;test\test_slow
set test_targets=%test_targets%;test\test_module_lock_custom
set test_targets=%test_targets%;test\test_module_lock_none
set test_targets=%test_targets%;test\test_module_lock_none
set test_targets=%test_targets%;test\old_tests\test_old
call msbuild /m /p:Configuration=%target_configuration%,Platform=%target_platform%,CppWinRTBuildVersion=%target_version% cppwinrt.sln /verbosity:Minimal "/t:%test_targets%"

call run_tests.cmd %target_platform% %target_configuration%
