param(
    [Parameter(Position = 0)]
    [string]$Command,

    [ValidateSet('x86', 'x64', 'arm', 'arm64')]
    [string]$Arch = 'x64',

    [ValidateSet('x86', 'x64', 'arm', 'arm64')]
    [string]$HostArch = 'x64',

    [switch]$NoLogo,
    [switch]$PassThruExitCode
)

$ErrorActionPreference = 'Stop'

$vsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vsWhere)) {
    throw "vswhere.exe was not found at '$vsWhere'."
}

$vsInstallPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsInstallPath) {
    throw 'No Visual Studio installation with C++ tools was found.'
}

$vsDevCmd = Join-Path $vsInstallPath 'Common7\Tools\VsDevCmd.bat'
if (-not (Test-Path $vsDevCmd)) {
    throw "VsDevCmd.bat was not found at '$vsDevCmd'."
}

$cmdParts = @()
$cmdParts += '"' + $vsDevCmd + '"'
$cmdParts += "-arch=$Arch"
$cmdParts += "-host_arch=$HostArch"
if ($NoLogo) {
    $cmdParts += '-no_logo'
}

$bootstrap = ($cmdParts -join ' ')
if ([string]::IsNullOrWhiteSpace($Command)) {
    $fullCommand = "$bootstrap && set"
}
else {
    $fullCommand = "$bootstrap && $Command"
}

& cmd.exe /d /s /c $fullCommand
$exitCode = $LASTEXITCODE

if ($PassThruExitCode) {
    exit $exitCode
}

if ($exitCode -ne 0) {
    throw "Command failed with exit code $exitCode."
}
