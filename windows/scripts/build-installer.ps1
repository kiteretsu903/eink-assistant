param(
    [ValidateSet('Debug','Release')]
    [string]$Configuration = 'Release',
    [string]$Version = '1.2',
    [string]$QtRoot = 'C:\Users\Admin\Qt',
    [string]$BuildDirectory = 'build-installer',
    [string]$InnoSetupCompiler,
    [switch]$SkipApplicationBuild,
    [switch]$CopyToDesktop
)

$ErrorActionPreference = 'Stop'
$windowsRoot = Split-Path -Parent $PSScriptRoot
$payloadRoot = Join-Path $windowsRoot 'artifacts\E-Ink-Assistant-Windows'
$installerOutput = Join-Path $windowsRoot 'artifacts\installer'
$languageCache = Join-Path $windowsRoot 'artifacts\installer-languages'
$installerDefinition = Join-Path $windowsRoot 'installer\EinkAssistant.iss'
$outputBaseName = "E-Ink-Assistant-Windows-$Version-Setup"
$innoSourceCommit = '1ae7bf81dc0d2013235dfe4bb0b6f4e4a0b6b25c'

if (-not $SkipApplicationBuild) {
    & (Join-Path $PSScriptRoot 'build-windows.ps1') `
        -Configuration $Configuration `
        -QtRoot $QtRoot `
        -BuildDirectory $BuildDirectory
    if ($LASTEXITCODE -ne 0) { throw 'Application build failed.' }
}

$requiredPayload = @(
    'EinkAssistant.exe',
    'EinkNightLightControl.exe',
    'Qt5Core.dll',
    'Qt5Gui.dll',
    'Qt5Widgets.dll',
    'platforms\qwindows.dll',
    'styles\qwindowsvistastyle.dll',
    'LICENSE',
    'LGPL-3.0.txt',
    'README.md',
    'TECHNICAL.md',
    'THIRD-PARTY-NOTICES.md'
)
foreach ($relativePath in $requiredPayload) {
    $fullPath = Join-Path $payloadRoot $relativePath
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        throw "Installer payload is incomplete: $fullPath"
    }
}

if (-not $InnoSetupCompiler) {
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) {
        $InnoSetupCompiler = $command.Source
    } else {
        $knownLocations = @(
            (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'),
            'C:\Program Files (x86)\Inno Setup 6\ISCC.exe',
            'C:\Program Files\Inno Setup 6\ISCC.exe'
        )
        $InnoSetupCompiler = $knownLocations |
            Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
            Select-Object -First 1
    }
}
if (-not $InnoSetupCompiler -or -not (Test-Path -LiteralPath $InnoSetupCompiler -PathType Leaf)) {
    throw 'Inno Setup 6 compiler was not found. Install JRSoftware.InnoSetup with WinGet, or pass -InnoSetupCompiler.'
}
$translations = @(
    [pscustomobject]@{
        Name = 'ChineseSimplified.isl'
        SHA256 = 'E0B0B350E2245F3C5E65586DFE43D574F6E7F06F2261149ABA284954B3FC9A8D'
    },
    [pscustomobject]@{
        Name = 'ChineseTraditional.isl'
        SHA256 = '031684FC769259291FD563338B5ABE20B7753C88AB5A2976B83A80788DEB8455'
    }
)
New-Item -ItemType Directory -Force -Path $installerOutput, $languageCache | Out-Null
foreach ($translation in $translations) {
    $translationPath = Join-Path $languageCache $translation.Name
    $validCache = (Test-Path -LiteralPath $translationPath -PathType Leaf) -and
        ((Get-FileHash -Algorithm SHA256 -LiteralPath $translationPath).Hash -eq $translation.SHA256)
    if (-not $validCache) {
        $translationUrl = "https://raw.githubusercontent.com/jrsoftware/issrc/$innoSourceCommit/Files/Languages/$($translation.Name)"
        Invoke-WebRequest -Uri $translationUrl -OutFile $translationPath -UseBasicParsing
    }
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $translationPath).Hash
    if ($actualHash -ne $translation.SHA256) {
        throw "Pinned installer translation failed SHA-256 validation: $translationPath"
    }
}

$simplifiedIsl = Join-Path $languageCache 'ChineseSimplified.isl'
$traditionalIsl = Join-Path $languageCache 'ChineseTraditional.isl'
& $InnoSetupCompiler `
    "/DAppVersion=$Version" `
    "/DSourceDir=$payloadRoot" `
    "/DOutputDir=$installerOutput" `
    "/DOutputBaseFilename=$outputBaseName" `
    "/DChineseSimplifiedIsl=$simplifiedIsl" `
    "/DChineseTraditionalIsl=$traditionalIsl" `
    $installerDefinition
if ($LASTEXITCODE -ne 0) { throw 'Installer compilation failed.' }

$installerPath = Join-Path $installerOutput "$outputBaseName.exe"
if (-not (Test-Path -LiteralPath $installerPath -PathType Leaf)) {
    throw "Installer compiler did not create the expected file: $installerPath"
}

$desktopPath = $null
if ($CopyToDesktop) {
    $desktopDirectory = [Environment]::GetFolderPath([Environment+SpecialFolder]::DesktopDirectory)
    if (-not $desktopDirectory) { throw 'The current user desktop directory could not be resolved.' }
    $desktopPath = Join-Path $desktopDirectory (Split-Path -Leaf $installerPath)
    Copy-Item -LiteralPath $installerPath -Destination $desktopPath -Force
}

$hash = Get-FileHash -Algorithm SHA256 -LiteralPath $installerPath
Write-Host "Installer: $installerPath"
Write-Host "SHA256: $($hash.Hash)"
if ($desktopPath) { Write-Host "Desktop copy: $desktopPath" }
