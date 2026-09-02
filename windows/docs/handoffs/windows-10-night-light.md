# Windows 10 Night Light handoff

> **Archived validation note.** This document records the pre-Windows-1.0
> cross-machine test session. Paths below reflect the old mixed repository
> layout. Current source paths are under `windows/src/`, tests under
> `windows/tests/`, build scripts under `windows/scripts/`, and authoritative
> documentation at `windows/docs/TECHNICAL.md`.

## Goal

Verify the dedicated Windows 10 Night Light path on Windows 10 2004–22H2.
If it fails, fix only that path while preserving the verified Windows 11 path.

Windows 10 has no 24H2 release. The intended routing is:

| Windows build | Night Light control path |
|---|---|
| Before 19041 (before Windows 10 2004) | Settings fallback only |
| 19041–19045 (Windows 10 2004–22H2) | Original CloudStore registry path |
| 22000 and newer (all Windows 11) | Settings UI Automation path |

Do not route Windows 10 through UI Automation unless the user explicitly
changes this decision. Do not replace the Windows 11 UI Automation path with
registry-only control; live Windows 11 testing proved that the registry blob
can change without changing the real native Night Light state.

## Workspace and package

- Repository on the current Windows 11 installation:
  `C:\Users\Admin\einkwindows`
- Branch: `master`
- Base merge commit: `187c3668b79e8c7d4b6b7adb6e1d842bb6e08c09`
- The routing change is currently an uncommitted working-tree change based on
  that commit.
- Windows 10 test package:
  `C:\Users\Admin\einkwindows\artifacts\E-Ink-Assistant-Windows-tray-v18`
- Zip:
  `C:\Users\Admin\einkwindows\artifacts\E-Ink-Assistant-Windows-tray-v18.zip`
- Do not accidentally test the canonical v17 folder; v18 contains the split
  Windows 10/Windows 11 routing.

v18 hashes:

- `EinkAssistant.exe`:
  `1ACE265B7509364250117410F54FB311393B6D0602671AE3E26DAE3691DD29A2`
- `EinkNightLightControl.exe`:
  `A8F6CA8DC46A652A24B3CFB885FC28E895E1F74F3C9DC8823BCEB13BEA247834`
- Zip:
  `D1D5EB3516B9FF485921C194537B67C2C501641650E1B5DC91DEF9454FC883E5`

When booted into Windows 10, the Windows 11 volume may have another drive
letter. Locate this file read-only if necessary:

```powershell
Get-PSDrive -PSProvider FileSystem | ForEach-Object {
    Join-Path $_.Root 'Users\Admin\einkwindows\WINDOWS10_NIGHT_LIGHT_HANDOFF.md'
} | Where-Object { Test-Path -LiteralPath $_ }
```

Use PowerShell 7 (`pwsh`) for shell commands.

## Implementation details

Routing is defined in:

- `src/platform/windows/WindowsCompatibility.h`
- `src/platform/windows/WindowsCompatibility.cpp`

`chooseNightLightControlPath(build)` returns:

- `Unavailable` below build 19041;
- `Windows10Registry` for builds 19041–21999;
- `Windows11UiAutomation` for build 22000 and newer.

The operational code is in:

- `src/platform/windows/WindowsPlatformServices.cpp`
- `src/platform/windows/NightLightControl.cs`
- `src/app/ApplicationController.cpp`
- `src/ui/MainPanel.cpp`

### Windows 10 path

The app decodes the current Night Light CloudStore `Data` blob, saves the exact
original blob in its recovery journal, changes the enabled field and timestamps,
writes the encoded blob back, and reads it back for verification. Restore and
crash recovery use the same registry path. It does not launch Settings and does
not require `EinkNightLightControl.exe` to expose the direct switch.

The state key is:

```text
HKCU\Software\Microsoft\Windows\CurrentVersion\CloudStore\Store\DefaultAccount\Current\default$windows.data.bluelightreduction.bluelightreductionstate\windows.data.bluelightreduction.bluelightreductionstate
```

Value: `Data`

### Windows 11 path

The elevated app launches `EinkNightLightControl.exe` with the normal Explorer
process as parent. The medium-integrity helper opens `ms-settings:nightlight`,
finds the native action button by UI Automation ID, invokes it, and cleans up
Settings only if the app opened it.

Verified Windows 11 IDs:

- `SystemSettings_Display_BlueLight_ManualToggleOn_Button`
- `SystemSettings_Display_BlueLight_ManualToggleOff_Button`

Do not modify this path merely because Windows 10 uses the registry path.

Both OS paths run on the controller's background system worker. The local
loading spinner replaces the switch until completion. Preserve the existing
focus-loss auto-hide behavior.

## Windows 10 test procedure

First record the exact build:

```powershell
Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion' |
    Select-Object ProductName, DisplayVersion, CurrentBuild, UBR
```

Confirm the v18 hash before launch (adjust the drive letter if needed):

```powershell
$app = 'C:\Users\Admin\einkwindows\artifacts\E-Ink-Assistant-Windows-tray-v18\EinkAssistant.exe'
Get-FileHash -Algorithm SHA256 -LiteralPath $app
```

Before testing, open Windows Night Light settings manually and record the real
native state. Close Settings, then launch v18 and accept UAC:

```powershell
Start-Process -FilePath $app -Verb RunAs
```

Test both directions:

1. Enable **Disable Night Light**.
   - The switch may show a brief loading spinner.
   - Native Night Light must become off.
   - Settings must not open.
2. Disable **Disable Night Light**.
   - Native Night Light must become on.
   - Settings must not open.
3. Repeat both directions once.
4. Quit from the tray normally.
5. Confirm the exact original Night Light state is restored.

Do not infer success only from the app status or registry. Confirm the actual
screen tint and the native Windows Night Light switch.

## Elevated self-test

Run from the complete v18 deployment folder so its Qt DLLs are available:

```powershell
$folder = 'C:\Users\Admin\einkwindows\artifacts\E-Ink-Assistant-Windows-tray-v18'
$report = 'C:\Users\Admin\einkwindows\artifacts\night-light-windows10-self-test.txt'
$process = Start-Process -FilePath (Join-Path $folder 'EinkAssistant.exe') `
    -ArgumentList @('--night-light-self-test', $report) -Verb RunAs -PassThru
$process.WaitForExit()
Get-Content -LiteralPath $report
```

Expected output ends in `PASS`, but visual/native Settings confirmation is still
required.

## If Windows 10 fails

Capture before changing code:

- Exact Windows build and language.
- Original native Night Light state.
- Whether the app shows a direct switch or only the Settings fallback.
- Whether any Settings window appeared (it should not on build 19041+).
- Exact error at the bottom of the app panel.
- Whether the CloudStore `Data` value changed.
- Whether the native state changed immediately, after a short delay, or only
  after restarting Explorer/signing out.
- Whether normal tray exit restored the original state.

Likely failure areas:

1. The Windows 10 build is below 19041 and should be using fallback.
2. The CloudStore blob format differs and `NightLightStateCodec` rejects it.
3. Registry write succeeds but Windows 10 does not react on that build.
4. The change works but state detection or restoration is wrong.

Do not silently fall back to the Windows 11 UI Automation helper. Diagnose the
Windows 10 registry path first because the user has already observed that this
path works on Windows 10.

Relevant tests:

- `tests/CoreTests.cpp::windowsCompatibilityDecisions`
- `tests/CoreTests.cpp::nightLightCodecMatchesReference`
- `tests/CoreTests.cpp::nightLightSessionSyncAndRestore`
- `tests/NightLightSmoke.cpp`
- `tests/E2ETests.cpp::slowSystemTogglesUseLocalSpinner`

## Build and verification

From the repository root:

```powershell
$env:PATH = 'C:\Users\Admin\Qt\Tools\mingw810_64\bin;C:\Users\Admin\Qt\5.15.2\mingw81_64\bin;' + $env:PATH
& 'C:\Users\Admin\Qt\Tools\CMake_64\bin\cmake.exe' --build build-windows11-acm --config Release --parallel
& 'C:\Users\Admin\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build-windows11-acm --output-on-failure
```

If the existing build directory or Qt location is unavailable from Windows 10,
configure a separate build directory rather than overwriting the Windows 11
tree.

Before handoff, core and E2E tests passed 2/2. The Windows 11 UI Automation
self-test also passed three consecutive v18 runs after adding the routing split.

## Safety and workspace rules

- Preserve `$log`, `$out`, `$report`, and `AGENTS.md`.
- Do not kill `EinkAssistant.exe`; quit from the tray so restoration runs.
- Always restore the user's original Night Light state after direct tests.
- Do not modify focus-loss auto-hide.
- Do not commit unrelated Windows 10 saturation-probe files.
- Do not install over the canonical folder while the app is running.
- If packaging a fix, copy both `EinkAssistant.exe` and
  `EinkNightLightControl.exe`, verify hashes, and test both Windows branches.

## Report back

Return:

- Windows 10 build.
- Whether the direct switch was shown.
- Whether Settings stayed closed.
- Real native result for both switch directions.
- Spinner/responsiveness result.
- Exit restoration result.
- Exact errors and whether the CloudStore value changed.
