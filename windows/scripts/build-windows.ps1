param(
    [ValidateSet('Debug','Release')]
    [string]$Configuration = 'Release',
    [string]$QtRoot = 'C:\Users\Admin\Qt',
    [string]$BuildDirectory = 'build'
)

$ErrorActionPreference = 'Stop'
$windowsRoot = Split-Path -Parent $PSScriptRoot
$workspaceRoot = Split-Path -Parent $windowsRoot
$qtPrefix = Join-Path $QtRoot '5.15.2\mingw81_64'
$toolRoot = Join-Path $QtRoot 'Tools'
$cmake = Join-Path $toolRoot 'CMake_64\bin\cmake.exe'
$ninja = Join-Path $toolRoot 'Ninja\ninja.exe'
$mingwBin = Join-Path $toolRoot 'mingw810_64\bin'
$qtBin = Join-Path $qtPrefix 'bin'
$buildDir = Join-Path $windowsRoot $BuildDirectory
$outputDir = Join-Path $windowsRoot 'artifacts\E-Ink-Assistant-Windows'

foreach ($required in @($cmake, $ninja, (Join-Path $qtBin 'qmake.exe'))) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required Qt build tool not found: $required"
    }
}

$env:PATH = "$mingwBin;$(Split-Path $ninja);$env:PATH"
& $cmake -S $windowsRoot -B $buildDir -G Ninja "-DCMAKE_BUILD_TYPE=$Configuration" "-DCMAKE_PREFIX_PATH=$qtPrefix"
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
& $cmake --build $buildDir --parallel
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }

$env:PATH = "$qtBin;$mingwBin;$env:PATH"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $outputDir 'platforms') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $outputDir 'styles') | Out-Null
Copy-Item -LiteralPath (Join-Path $buildDir 'bin\EinkAssistant.exe') -Destination $outputDir -Force
Copy-Item -LiteralPath (Join-Path $buildDir 'bin\EinkNightLightControl.exe') -Destination $outputDir -Force
foreach ($library in @('Qt5Core.dll', 'Qt5Gui.dll', 'Qt5Widgets.dll')) {
    Copy-Item -LiteralPath (Join-Path $qtBin $library) -Destination $outputDir -Force
}
foreach ($runtime in @('libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll')) {
    Copy-Item -LiteralPath (Join-Path $mingwBin $runtime) -Destination $outputDir -Force
}
Copy-Item -LiteralPath (Join-Path $qtPrefix 'plugins\platforms\qwindows.dll') -Destination (Join-Path $outputDir 'platforms') -Force
Copy-Item -LiteralPath (Join-Path $qtPrefix 'plugins\styles\qwindowsvistastyle.dll') -Destination (Join-Path $outputDir 'styles') -Force
Copy-Item -LiteralPath (Join-Path $workspaceRoot 'LICENSE') -Destination $outputDir -Force
Copy-Item -LiteralPath (Join-Path $windowsRoot 'README.md') -Destination $outputDir -Force
Copy-Item -LiteralPath (Join-Path $windowsRoot 'docs\TECHNICAL.md') -Destination $outputDir -Force
Copy-Item -LiteralPath (Join-Path $windowsRoot 'docs\THIRD-PARTY-NOTICES.md') -Destination $outputDir -Force
Copy-Item -LiteralPath (Join-Path $workspaceRoot 'licenses\LGPL-3.0.txt') -Destination $outputDir -Force

& (Join-Path $buildDir 'bin\eink_core_tests.exe')
if ($LASTEXITCODE -ne 0) { throw 'Core tests failed.' }
& (Join-Path $buildDir 'bin\eink_e2e_tests.exe') -platform offscreen
if ($LASTEXITCODE -ne 0) { throw 'E2E tests failed.' }

$zipPath = Join-Path $windowsRoot 'artifacts\E-Ink-Assistant-Windows.zip'
Compress-Archive -Path (Join-Path $outputDir '*') -DestinationPath $zipPath -Force
$hash = Get-FileHash -Algorithm SHA256 -LiteralPath $zipPath
Write-Host "Built: $zipPath"
Write-Host "SHA256: $($hash.Hash)"
