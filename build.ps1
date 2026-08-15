<#
.SYNOPSIS
    Build script for FS Input Device Emitter solution.

.DESCRIPTION
    Restores required NuGet packages and builds the C++ and C# projects
    using MSBuild.

.PARAMETER Configuration
    Build configuration: 'Release' (default) or 'Debug'.

.PARAMETER Platform
    Target platform: 'x64' (default) or 'x86'.

.PARAMETER Rebuild
    If set, forces a clean rebuild.
#>

[CmdletBinding()]
param (
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',

    [ValidateSet('x64', 'x86')]
    [string]$Platform = 'x64',

    [switch]$Rebuild
)

$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Building FS Input Device Emitter" -ForegroundColor Cyan
Write-Host " Configuration: $Configuration | Platform: $Platform" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# 1. Locate MSBuild
$msbuildPath = $null

$vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswherePath) {
    $msbuildPath = & $vswherePath -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
}

if (-not $msbuildPath -or -not (Test-Path $msbuildPath)) {
    $commonPaths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
    )
    foreach ($p in $commonPaths) {
        if (Test-Path $p) {
            $msbuildPath = $p
            break
        }
    }
}

if (-not $msbuildPath) {
    Write-Error "MSBuild.exe could not be found. Please ensure Visual Studio or Build Tools are installed."
    exit 1
}

Write-Host "[1/3] Using MSBuild: $msbuildPath" -ForegroundColor Green

# 2. Restore NuGet Packages
Write-Host "[2/3] Restoring NuGet Packages..." -ForegroundColor Green
$nugetExe = Join-Path $ScriptDir "nuget.exe"
if (-not (Test-Path $nugetExe)) {
    Write-Host "      Downloading nuget.exe..." -ForegroundColor Yellow
    Invoke-WebRequest -Uri "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" -OutFile $nugetExe -UseBasicParsing
}

& $nugetExe restore (Join-Path $ScriptDir "InputEmitter.sln")
if ($LASTEXITCODE -ne 0) {
    Write-Error "NuGet package restore failed."
    exit $LASTEXITCODE
}

# 3. Build Solution
$target = if ($Rebuild) { "Rebuild" } else { "Build" }
Write-Host "[3/3] Running MSBuild ($target)..." -ForegroundColor Green

$solutionFile = Join-Path $ScriptDir "InputEmitter.sln"
& $msbuildPath $solutionFile "/t:$target" "/p:Configuration=$Configuration" "/p:Platform=$Platform" "/m"

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

Write-Host "=========================================" -ForegroundColor Green
Write-Host " Build succeeded!" -ForegroundColor Green
Write-Host " Output binary directory: $(Join-Path $ScriptDir "x64\$Configuration")" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
