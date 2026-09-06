# E-Ink Assistant for Windows

> **Maintainer and coding-agent reference:** read
> [`docs/TECHNICAL.md`](docs/TECHNICAL.md). It is the authoritative record
> of Windows architecture, feature gates, lifecycle rules, tests, and decisions.

> **Active localization work:** the
> [80-locale Windows handoff](docs/handoffs/multilingual-80.md) describes shared
> translation resources and the native integration and verification still
> required on Windows. Current Windows support remains the four locales below
> until that work passes; shared translation files alone do not expand the app.

**Version status:** Windows 1.2 source baseline, currently unreleased.

This directory contains the native Windows edition of E-Ink Assistant. It uses
Qt 5.15 Widgets rather than a browser runtime, keeps the original high-contrast
panel structure, and isolates Windows APIs behind a platform interface so the
same core and UI can be reused by a later Linux backend.

## Compatibility

- **Windows 7 SP1 through Windows 11:** tray UI, display selection, Text
  Contrast, Video Enhance, Advanced curves and five saved presets, visual
  effects controls, display reconnection handling, four UI languages, and
  launch at login.
- **Tray icon:** Windows decides whether the icon begins in the visible area or
  hidden-icons menu. The app does not modify private Explorer preferences. Its
  compact welcome window shows the exact monochrome book-pages icon sharply
  and explains how to drag it into the visible notification area once for
  easier access.
- **Hardware baseline reminder:** the top of the main panel briefly recommends
  balanced or slightly lower monitor hardware contrast. **Got it** hides it for
  the session; **Never show it again** persists the choice.
- **Windows 10 May 2019 Update or above:** session-only Windows Light Mode. It
  changes Windows mode only, leaves app mode untouched, and restores the
  pre-change value on quit.
- **Windows 10 builds 19041 through 19045:** eligible display paths require the
  profile APIs, an exact GPU/source/target mapping, WDDM 2.6 or later, and a
  positive per-target MatrixDDI result. Saturation and RGB remain off by default.
  First enable shows a centered, dimming five-second warning, applies a small
  test under an independent fifteen-second rollback watchdog, and requires the
  user to confirm that the display remains normal. Timeout or manual rollback
  restores the display and remains retryable; only an actual apply/confirm
  failure rejects that OS/GPU/driver/output/display fingerprint.
- **Windows 11 24H2 or above:** Saturation and RGB use the ACM path through
  per-display ICC profiles and **Automatically manage color for apps**, plus guarded
  system-wide **Disable Night Light** control. The Night Light option appears
  only when the live CloudStore state passes strict format validation.
- **Manual saturation path:** the GPU control-panel guide appears for fully
  unsupported displays and for Windows 10 candidates while experimental color
  controls are off. It disappears while confirmed controls are enabled. The app
  identifies the exact scanout GPU and can open an installed matching NVIDIA,
  Intel, or AMD control panel.
- **Duplicate-display mode:** every active DisplayConfig target is enumerated.
  Windows does not apply per-display ICC/MHC2 color changes while a source is
  duplicated, so Saturation and RGB are unavailable and the UI recommends
  switching to Extend instead of offering a GPU-panel fallback. Other tuning
  warns which cloned display will also be affected. Only one display in that
  clone group can own the tuning controls. Switching between Duplicate and
  Extend aborts an active safety test, restores the old source, refreshes the
  mapping after a short debounce, and reapplies saved e-ink settings to the new
  topology.
- **Reduce Shaking:** not shown on Windows because there is no safe public
  per-display dithering API. The application does not use undocumented
  vendor-registry changes.
- **Night Light fallback:** Windows 10 build 15063 and later can open the native
  settings page when direct control is unavailable. On Windows 11 24H2 or
  above, the inverse switch follows the live system status: switch Off means
  Night Light On. Both directions are temporary, preserve the schedule, and
  restore the pre-change On/Off state with a fresh Windows transition timestamp
  on quit or after an abnormal exit.
- **Night Light focus:** Windows 11 temporarily opens Settings for UI
  Automation. The panel remains visible during that owned transition, returns
  to the foreground when it finishes, and then resumes normal focus-loss hiding.

Qt 5.15.2 is intentionally pinned because it is the last broadly deployable Qt
generation that supports Windows 7. The release uses dynamically linked Qt
libraries, preserving LGPL relinking rights.

The Windows 1.2 build is x64. On the development machine, a packaged tray app
measured **39.58 MiB working set** and **23.03 MiB private memory** after two
seconds idle. Results vary with Windows, the display count, and Qt's shared DLL
pages, but no web engine, QML runtime, service, or independently installed
helper is loaded. The color broker starts only when a color-profile path is used
and closes with the application.

## Architecture

- `src/core`: cross-platform settings, tone curves, ICC generation, and state.
- `src/ui`: reusable Qt Widgets panel, controls, localization, and welcome UI.
- `src/platform`: the platform contract.
- `src/platform/windows`: display enumeration, ACM, ICC association, gamma,
  accessibility, startup, and broker code.

A Linux port can keep the core and UI and implement the same platform contract.
Shared localization and product artwork live in the repository-level
`../Resources/` directory. The macOS source is isolated under `../macos/`.

Documentation is indexed in [`docs/README.md`](docs/README.md), and the
unreleased 1.2 summary is in [`CHANGELOG.md`](CHANGELOG.md).

## Build

Install Qt 5.15.2 `mingw81_64`, MinGW 8.1, CMake and Ninja. From the repository
root, enter the Windows project and run the build in PowerShell 7:

```powershell
Set-Location windows
./scripts/build-windows.ps1
```

The deployable directory and ZIP under `windows/artifacts/` support both the
Windows 10 MHC2 and Windows 11 24H2+ ACM runtime paths.

### Installer

The repository includes an Inno Setup 6.7+ definition and repeatable installer
build script. After installing Inno Setup 6.7 or later, run from `windows/`:

```powershell
./scripts/build-installer.ps1
```

The script first performs the normal Release build and full automated test
baseline, validates every required deployed file, and then writes
`artifacts/installer/E-Ink-Assistant-Windows-1.2-Setup.exe`. Pass
`-CopyToDesktop` to also place a copy on the current user's desktop, or
`-SkipApplicationBuild` to package an already-validated payload.

The installer UI supports English, Simplified Chinese, Traditional Chinese,
and Japanese. It uses a neutral modern style with native light-theme controls,
a borderless page treatment, automatic dark appearance, and the product icon
instead of Inno Setup's blue package artwork, while retaining the Windows
7-compatible installer engine. The two Chinese Inno Setup
translations are downloaded from a pinned upstream source revision on first
use, verified against committed SHA-256 values, and cached under
`artifacts/installer-languages/`.

The installer is per-machine and requires administrator approval. It installs
under Program Files, creates an uninstall entry and Start menu shortcut, and
offers an optional desktop shortcut. During an update, Setup first asks version
1.1 or later to exit normally and waits up to eight seconds for display-state
restoration. If the app cannot close, the Preparing to Install page offers to
force-close it and continue the update; use that fallback only when graceful
shutdown fails. When the completion-page launch option is selected, the program
inherits Setup's administrative token because its executable manifest requires
elevation.

## Tests

The build runs:

- mathematical and persistence tests for tone curves, ICC profiles, state and
  localization, including reference-vector tests for the Night Light codec;
- a complete UI journey against a fake display backend;
- duplicate-to-extend and extend-to-duplicate transitions, including safety
  rollback, single-owner enforcement, and the clone-impact warning;
- a simulated Windows 7 capability path that verifies unsupported color rows
  are absent while tone controls remain usable.

The Night Light test harness also verifies live disable/restore and a forced
process-termination recovery cycle without requiring the tray app's startup
UAC prompt:

```powershell
./build/bin/eink_night_light_smoke.exe normal artifacts/night-light-test.txt
./build/bin/eink_night_light_smoke.exe crash artifacts/night-light-crash.ini
./build/bin/eink_night_light_smoke.exe recover artifacts/night-light-crash.ini
```

For a read/write/restore smoke test against the connected displays:

```powershell
./build/bin/eink_hardware_smoke.exe
```

That test captures each current gamma ramp, writes an identity ramp, and
immediately restores the captured table. It only queries ACM state and does not
install an ICC profile.

The Windows 11 hardware verification additionally passed an ACM enable/disable
round trip and a real medium-integrity UI to elevated-broker color test at 101%
saturation. The latter verified the generated ICC profile became the selected
per-display default, then confirmed the original empty profile, ACM-off state,
and color-profile directory were restored with no generated file left behind.
The system-theme smoke test also verifies Windows Light Mode, confirms app mode
is untouched, and restores the machine's original Windows color mode afterward.

The application currently requests administrator privileges at startup. Launch
at Login uses a highest-available per-user Task Scheduler logon task so it can
start without an interactive UAC prompt. Color-profile work remains serialized
and recoverable: on quit the app removes its association, restores the previous
profile and ACM state, and uninstalls only its own generated profiles.
