[CmdletBinding()]
param(
    [switch]$Install
)

$ErrorActionPreference = "Stop"

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$srcFile = Join-Path $repoRoot "src\ford_fmax_plugin.cpp"
$binDir = Join-Path $repoRoot "bin"
$outDll = Join-Path $binDir "ford_fmax_plugin.dll"

if (-not (Test-Path $binDir)) {
    New-Item -ItemType Directory -Path $binDir -Force | Out-Null
}

Write-Host "Locating Visual Studio MSVC compiler..." -ForegroundColor Cyan

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found at $vswhere"
}

$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    throw "No compatible Visual Studio installation with C++ tools found."
}

$vcvars64 = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars64)) {
    throw "vcvars64.bat not found at $vcvars64"
}

Write-Host "Compiling 64-bit native plugin with MSVC..." -ForegroundColor Cyan

$buildCmd = "`"$vcvars64`" && cl.exe /nologo /O2 /W3 /LD /std:c++17 `"$srcFile`" /link /OUT:`"$outDll`""
cmd.exe /c $buildCmd

if (-not (Test-Path $outDll)) {
    throw "Build failed: output DLL not found at $outDll"
}

$dllSize = (Get-Item $outDll).Length
Write-Host "Successfully compiled ford_fmax_plugin.dll ($dllSize bytes)" -ForegroundColor Green

# Clean up intermediate build files in binDir
Get-ChildItem -Path $repoRoot -Filter "ford_fmax_plugin.*" | Where-Object { $_.Extension -in ".obj", ".exp", ".lib" } | Remove-Item -Force
Get-ChildItem -Path $binDir -Filter "ford_fmax_plugin.*" | Where-Object { $_.Extension -in ".obj", ".exp", ".lib" } | Remove-Item -Force

if ($Install) {
    $ets2PluginDir = "C:\Program Files (x86)\Steam\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins"
    if (-not (Test-Path $ets2PluginDir)) {
        Write-Host "Creating ETS2 plugins directory: $ets2PluginDir" -ForegroundColor Cyan
        New-Item -ItemType Directory -Path $ets2PluginDir -Force | Out-Null
    }
    $targetPath = Join-Path $ets2PluginDir "ford_fmax_plugin.dll"
    Copy-Item -Path $outDll -Destination $targetPath -Force
    Write-Host "Installed ford_fmax_plugin.dll to: $targetPath" -ForegroundColor Magenta
}
