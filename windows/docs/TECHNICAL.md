# E-Ink Assistant for Windows — Engineering Reference and Decision Record

**Document status:** authoritative for the Windows port
**Application version:** 1.0.x
**Release status:** source baseline prepared; Windows 1.0 is not released
**Last reconciled with the source tree:** 2026-09-01
**Primary audience:** maintainers, release engineers, and later coding agents

This document records the implemented Windows architecture, product decisions,
compatibility policy, operating-system gates, lifecycle rules, test strategy,
and known limitations. It is intentionally more detailed than `../README.md`.
When a future change makes this document inaccurate, update the code and this
document in the same change.

`../../TECHNICAL.md` describes the original macOS implementation. It is useful for
product intent and preset parity, but it is not the source of truth for Windows
APIs or Windows behavior.

## 1. Product contract

The Windows port preserves the macOS application's features and visual model
where Windows exposes a safe implementation. It must remain:

- a small native desktop/tray application rather than a browser application;
- responsive while displays are connected, disconnected, or reconfigured;
- usable on Windows 7 where the operating system provides the required API;
- honest about unavailable features: unsupported controls are absent instead
  of being present but misleadingly disabled; a concise explanatory fallback
  may remain when it gives the user a safe manual alternative;
- structured so a later Linux backend can reuse the state, curve, ICC, and Qt
  UI code;
- reversible: temporary display changes are restored on normal exit, and color
  profile state is recoverable after an abnormal exit.

The application does not install a service, kernel driver, or vendor-specific
display hack. It does not pretend that a screen overlay is equivalent to real
display color calibration.

Windows 1.0 is recorded consistently in CMake project metadata, the executable
manifest and version resource, and the visible panel header. This source-tree
identity does not create a public release, tag, website update, or root-product
changelog entry.

## 2. Major decisions

### 2.1 Qt 5.15 Widgets is the UI framework

The Windows UI uses **Qt 5.15.2 Widgets**, C++17, and the MinGW 8.1 x64 toolchain.

Reasons:

- Qt Widgets has substantially lower idle cost than bundling Chromium or a web
  runtime and does not require QML.
- Qt 5.15 is the last Qt generation that can be deployed broadly to Windows 7.
- the same widgets, core state, and controller boundary can be reused by a
  future Linux backend;
- the framework is mature and predictable for a utility-style panel.

Rejected directions:

- **WinUI / Windows App SDK:** too new for the compatibility goal and not
  reusable on Linux;
- **WPF / WinForms:** Windows-only and less useful for the later Linux port;
- **Electron / WebView:** unnecessary memory and process overhead;
- **Qt Quick/QML:** additional runtime and rendering complexity without a
  product requirement.

The Windows package dynamically links Qt and includes the LGPL license and
relinking notices.

### 2.2 Use public APIs and leave tray placement to Windows

Display gamma, ICC/WCS, Task Scheduler, registry theme values, display
enumeration, accessibility settings, and the notification-area icon use public
Windows and Qt APIs. The application does not write Explorer's private tray
preference database or attempt to force its icon into the visible area. The
first-run window shows the exact icon and gives manual drag-and-pin guidance.

No undocumented GPU dithering registry setting is used. Windows has no safe,
universal, public per-display dithering control, so the Windows UI does not show
the macOS **Reduce Shaking** feature.

### 2.3 Saturation and RGB use Windows Auto Color Management

On Windows 11 24H2 or above (build 26100 or later), saturation and RGB use the
**ACM path**: the app generates a
per-display ICC profile containing an `MHC2` hardware-calibration tag and
associating that profile through Windows color management. Auto Color
Management (ACM) must be supported by the selected display.

The same executable selects a separate **Windows 10 MHC2 path** at runtime on
builds 19041 through 19045. That path reuses the transactional MHC2
profile/association work but never queries or changes Windows 11 ACM/WCG state.
It requires the modern display-profile entry points in `mscms.dll`. If
`ColorProfileGetDeviceCapabilities` is exported, the exact display must report
MHC2 support. The tested Windows 10 build 19044 image does not export that
capability entry point, so availability remains display- and driver-dependent.
Visible saturation changes were physically verified on the test configuration
recorded in section 15.4; this is evidence for that hardware, not a universal
Windows 10 guarantee.

This is the closest universal Windows path to the macOS behavior. A transparent
overlay was rejected because it changes composited pixels rather than the
display color transform, cannot correctly increase saturation, affects screen
capture behavior, and produces incorrect results in many applications.

Saturation and RGB are composed into **one** generated profile. They are not
separate independent transforms.

The **Factory** preset means an identity profile at Saturation 100% and RGB
100/100/100. It does not disable ACM. Keeping the neutral profile active avoids
the visible flicker caused by toggling ACM off and on.

### 2.4 Tone curves use the display gamma ramp

Text Contrast, Video Enhance, and Advanced curves use `SetDeviceGammaRamp` on a
display DC. This API is available on older Windows versions and can be applied
per display, but the graphics driver may clamp, alter, or reject a requested
ramp. The application captures the original ramp before the first change and
restores it when tuning is removed or the app exits.

Text Contrast and Video Enhance are mutually exclusive because both own the
same hardware gamma table. Advanced curve controls also feed the same curve
pipeline.

### 2.5 Manual startup is elevated; logon startup uses Task Scheduler

The executable manifest requests `requireAdministrator`. Manual launch therefore
shows a UAC prompt. This was an explicit product decision to make system color
configuration reliable.

**Launch at Login** creates a per-user Task Scheduler task named
`E-Ink Assistant` with:

- a logon trigger;
- `InteractiveToken` and `HighestAvailable`;
- the current executable plus `--background`;
- `MultipleInstances = IgnoreNew`;
- `DisallowStartIfOnBatteries = false`;
- `StopIfGoingOnBatteries = false`;
- `StartWhenAvailable = true`;
- no execution time limit.

This permits elevated logon startup without displaying a UAC prompt at every
login and avoids the common Task Scheduler default that blocks a task on
battery. An old `HKCU\...\Run` entry is migrated away.

When the real `EinkAssistant.exe` starts from a new folder, it compares an
enabled task action with its current executable path and updates a stale task.
This prevents an older login copy from winning the single-instance lock after
an upgrade. Test and diagnostic executables are excluded from this migration.

The color broker/named-pipe path remains in the implementation. It originated
when the main UI ran at medium integrity and now provides process isolation and
recovery structure, but it is no longer the only elevation boundary because
the main executable itself is elevated.

### 2.6 Unsupported controls are hidden; actionable guidance may remain

Capability checks happen both in the UI and the Windows backend. A feature that
is unavailable must not be constructed as a visible adjustment control, and the
backend must still reject an accidental call. This two-layer rule prevents a UI
error from reaching an unsupported API.

Do not replace absence with a permanently disabled control. The saturation
exception is an informational section: it explains why E-Ink Assistant cannot
adjust the selected display, identifies that display's scanout GPU, and offers
one verified vendor-panel launcher when installed. It never presents an
unavailable E-Ink Assistant slider as usable.

## 3. Source layout and ownership

All paths in this document are relative to the repository's `windows/`
directory unless stated otherwise.

| Path | Responsibility |
| --- | --- |
| `src/core/AppState.*` | Cross-platform settings and per-display state |
| `src/core/SettingsStore.*` | INI persistence and compatibility reads |
| `src/core/ToneCurve.*` | Curve generation, presets, and curve math |
| `src/core/IccProfile.*` | ICC profile generation, parsing, and validation |
| `src/app/ApplicationController.*` | Orchestration, state transitions, serialization of color work, and shutdown |
| `src/platform/PlatformServices.h` | Platform contract for Windows and a future Linux backend |
| `src/platform/windows/WindowsPlatformServices.*` | Windows display, gamma, ACM, color-profile, accessibility, theme, and startup integration |
| `src/platform/windows/WindowsNative.h` | Windows types/constants and dynamically resolved color APIs |
| `src/ui/MainPanel.*` | Main panel layout, adaptive size, anchoring, focus behavior, and inline busy state |
| `src/ui/DisplayCard.*` | Per-display tuning controls and feature-gated sections |
| `src/ui/SmoothLabel.*` | Smooth grayscale text rendering used for CJK consistency |
| `src/ui/TrayIcon.*` | Mac-style monochrome tray glyph and theme adaptation |
| `src/ui/WelcomeDialog.*` | First-run tray callout and antialiased arrow/body painting |
| `src/ui/BusyDialog.*` | Startup-only preparation indicator before the panel is ready |
| `src/main.cpp` | Process lifecycle, single instance, tray integration, hotplug event filter, diagnostics, and quit restoration |
| `tests/FakePlatform.*` | Deterministic platform fake for controller/UI tests |
| `tests/CoreTests.cpp` | Math, persistence, color, recovery, and controller tests |
| `tests/E2ETests.cpp` | Full Qt UI journeys and regression tests |
| `tests/HardwareSmoke.cpp` | Opt-in real-system read/write/restore checks |
| `installer/EinkAssistant.iss` | Inno Setup definition for the per-machine Windows installer |
| `scripts/build-installer.ps1` | Release build, payload and pinned translation validation, multilingual installer compilation, hashing, and optional desktop copy |

Dependency direction is:

```text
Qt UI -> ApplicationController -> PlatformServices
                    |                    |
                    v                    v
                  core              Windows backend
```

Core code must not include Windows headers. UI code should ask the controller
for capabilities rather than testing the OS build itself. A Linux port should
implement `PlatformServices` and keep Windows registry, COM, WCS, and display
configuration logic out of the reusable layers.

## 4. Platform compatibility matrix

The project compiles with `_WIN32_WINNT=0x0601` and `WINVER=0x0601`.
Newer entry points that would break process loading on Windows 7 are resolved
dynamically.

| Feature | Minimum gate | Behavior below the gate |
| --- | --- | --- |
| Tray UI, display selection, localization | Windows 7 SP1 | Baseline |
| Text Contrast, Video Enhance, Advanced curves | Windows 7 SP1 | Baseline; driver-dependent |
| Reduce Transparency & Motion | Windows 7 SP1 | Uses legacy-compatible system APIs |
| Launch at Login | Windows 7 SP1 | Uses Task Scheduler COM |
| Night Light settings | build 15063 (Windows 10 1703) | Entire row absent |
| Disable Night Light | build 19041 (Windows 10 2004) plus strict payload validation | Recommendation, Settings path and native settings button |
| Windows Light Mode | build 18362 (Windows 10 1903) | Entire section absent |
| Saturation and RGB, Windows 11 ACM path | build 26100 plus modern profile APIs and per-display ACM support | Adjustment controls absent; selected e-ink display gets driver/manual guidance |
| Saturation and RGB, Windows 10 MHC2 path | build 19041–19045, modern profile APIs, and MHC2 support when queryable | Adjustment controls absent below the gate; hardware/driver-dependent when the capability API is absent; selected e-ink display gets upgrade/manual guidance |
| Tray icon and first-run guidance | Windows 7 SP1 | Windows owns visible/overflow placement; the welcome window provides manual pinning steps |
| True per-monitor DPI | OS support required | Windows 7 uses system-DPI behavior |

The Windows 11 path wording is **“Windows 11 24H2 or above.”** Do not shorten it
to a single fixed release. The one package performs the build/runtime checks
and selects Windows 10 MHC2 or Windows 11 ACM for each display.

### Windows 7 compatibility policy

Windows 7 is a general code-path target, not a promise that every modern
feature exists. On Windows 7 the intended visible feature set is:

- panel, tray, display selection, languages, saved state;
- Text Contrast, Video Enhance, and Advanced curves;
- Reduce Transparency & Motion through compatibility APIs;
- launch at login through Task Scheduler;
- Qt-painted rounded panels and welcome callout.

The following adjustment controls must be absent:

- Night Light;
- Windows Light Mode;
- Saturation and RGB/ACM;
- Reduce Shaking.

For a selected e-ink display, an unavailable saturation path may still show the
informational GPU fallback and a verified legacy vendor control-panel shortcut.

Qt-only rounded corners are intentional. Do not introduce a hard dependency on
Windows 11 DWM corner attributes. The current code has automated compatibility
coverage but has not been fully boot-tested on every Windows 7 GPU/driver
combination.

## 5. Display identity and hotplug lifecycle

Windows display enumeration combines `EnumDisplayDevicesW` and
`QueryDisplayConfig` data.

- Persistent settings use a stable monitor/device identifier, not the display
  number or transient desktop position.
- The user-visible name prefers the DisplayConfig target-friendly name.
- If Windows reports “Generic PnP Monitor,” the code derives the EDID/model code
  from the device identifier instead of showing the same generic name for every
  display.
- Built-in/external status comes from the output technology where available.
- Each active target's `QueryDisplayConfig` adapter LUID is matched against
  DXGI. This identifies the scanout GPU rather than guessing from the first GPU
  on a hybrid system. The adapter name and PCI vendor are carried with the
  display capability record.

`main.cpp` listens for `WM_DISPLAYCHANGE`, `WM_DEVICECHANGE`, and relevant power
broadcasts. Events are debounced for approximately 800 ms because a physical
connect/disconnect can emit several messages while the topology is incomplete.
Refresh waits for any active serialized color operation, re-enumerates, rebuilds
only what changed, and reapplies saved state to a returning display.

Rules for future changes:

1. Never use a UI index as a persistent display identity.
2. Never reapply a stale profile to a display that no longer resolves to the
   original stable ID.
3. Do not block the GUI event loop while the topology or color system settles.
4. A disappearing display is normal, not an exceptional crash condition.

## 6. Color profile architecture

### 6.1 Availability

On Windows 11, the saturation/RGB UI is available only when all conditions are
true:

1. `windowsBuild() >= 26100`; and
2. the modern display-profile APIs are available; and
3. the selected display reports ACM support through DisplayConfig advanced
   color information.

The backend repeats these checks. Color profile APIs such as
`ColorProfileAddDisplayAssociation`, `ColorProfileRemoveDisplayAssociation`,
`ColorProfileSetDisplayDefaultAssociation`, and related queries are loaded from
`mscms.dll` at runtime to preserve the Windows 7 process-loader path.

Builds 19041 through 19045 use a second runtime path in the same executable:

1. all required modern profile list/default/association entry points must be
   present;
2. `ColorProfileGetDeviceCapabilities`, when present, must return MHC2 support
   for the exact adapter LUID and source ID;
3. when that capability function is absent, the UI identifies the Windows 10
   MHC2 path and warns that support remains hardware/driver-dependent;
4. the generated MHC2 profile is installed and selected transactionally, but
   ACM/WCG is neither queried nor toggled.

API success cannot prove that scanout hardware applied the MHC2 matrix. The
current Windows 10 test configuration has been physically verified, while
other display/GPU combinations still require a visual check. Quit and crash
recovery use the same association/default/profile journal as the Windows 11
path.

When neither color path is available, the saturation and RGB controls remain
absent. The selected display instead shows one of two messages. Before build
26100, Windows 11 24H2 or above is described only as something that *may* add
support. On build 26100 or later, the message recommends updating the current
graphics driver and does not promise that an OS upgrade will help.

The fallback resolves the vendor from the exact display adapter LUID and opens
only an installed matching panel. Discovery prefers registered AppsFolder
entries (including NVIDIA Control Panel, Intel Graphics Command Center, and
AMD Software), then machine-level registered `App Paths`, then known legacy locations for
NVIDIA `nvcplui`, Intel `GfxUIEx`/`GfxUI`/`igfxcpl.cpl`, AMD
Radeon Software/Radeon Settings, and Catalyst `CCC.exe`/`ati2cpl.cpl`.
Unknown vendors and driver-only installations receive no misleading button.
This integration opens the vendor UI; it does not automate a saturation page,
because vendor versions and panel layouts do not expose a stable common route.

### 6.2 Generated profile

`IccProfile::make` produces an ICC profile with standard matrix/TRC tags and an
`MHC2` tag containing the composed saturation/RGB matrix and identity LUTs. The
base display profile is parsed when possible so the generated transform starts
from the original display characterization rather than an arbitrary generic
profile.

Ranges:

- Saturation: 0–300%;
- Red, Green, Blue: 0–200% each;
- Factory identity: 100%, 100%, 100%, 100%.

Generated files follow the shape:

```text
EinkAssistant-<display-hash>-<session-id>-<sequence>.icm
```

Descriptions include the display identity and active values. Names are unique
per session so an interrupted write cannot silently overwrite another active
profile.

### 6.3 Transaction and recovery rules

Color profile changes are transactional at the application level:

1. Resolve the current display and capture the original profile, association
   scope, and ACM state.
2. Write a recovery record to the app configuration area before mutating the
   system (`color-recovery.ini`).
3. Generate and validate the profile.
4. Install it, associate it with the exact display, make it default, and ensure
   the required ACM state.
5. Record enough information to undo each completed step.

On normal shutdown, restore the original default association and ACM state,
remove the app association, and uninstall only profiles generated by this app.
On the next startup after an abnormal exit, recovery runs before applying new
state. It restores the recorded original and removes stale generated
associations/files.

Startup cleanup also deduplicates old `EinkAssistant-*` profiles for the same
display. This prevents the Windows Color Management page from accumulating
`(1)`, `(2)`, and later duplicates after a crash.

Never delete a profile that cannot be positively identified as generated by
E-Ink Assistant. Never assume the original profile is system-wide; preserve its
scope.

### 6.4 Factory behavior

Factory is an active neutral profile. It must not:

- turn ACM off;
- detach the profile merely because every slider equals 100%;
- briefly restore the original and then attach another generated profile.

Those transitions cause an avoidable visible flash. The identity profile is a
deliberate steady state.

## 7. Tone-curve architecture

Tone curves are produced in `src/core/ToneCurve.*` and applied through the
Windows gamma ramp. The curve uses a smooth transition around the knee and
black/white point remapping while remaining monotonic and preserving valid
endpoints.

Current Windows 1.0 presets are:

| Group | Preset | Black | Gamma | Knee | White |
| --- | --- | ---: | ---: | ---: | ---: |
| Text Contrast | Medium | 0.65 | 2.10 | 0.00 | 1.00 |
| Text Contrast | Strong | 0.80 | 2.70 | 0.00 | 1.00 |
| Text Contrast | Sharp | 1.00 | 5.00 | 0.10 | 1.00 |
| Text Contrast | Solid | 1.00 | 6.00 | 0.34 | 1.00 |
| Video Enhance | Subtle | 0.25 | 0.75 | 0.00 | 1.00 |
| Video Enhance | Medium | 0.35 | 0.60 | 0.00 | 1.00 |
| Video Enhance | Strong | 0.45 | 0.45 | 0.00 | 1.00 |

These values were synchronized from the macOS v2.3 presets.
Do not casually “improve” them independently on Windows; preset parity is a
product decision. If upstream changes, update the constants, tests, and this
table together.

The UI includes explanatory/warning text, matching the macOS intent, that these
effects alter display tone response and that Video Enhance brightens dark
areas. A successful API return is not proof that every GPU rendered the exact
requested curve, so hardware tests must include visual or sampled verification
where practical.

## 8. System visual and theme controls

### 8.1 Reduce Transparency & Motion

This is in the **Follow e-ink displays** group. It controls both animation and
transparency, not only the Windows transparency toggle.

- On Windows 10/11 it combines system animation settings with the modern
  transparency registry value.
- On Windows 7/8 it uses the compatible SystemParametersInfo/DWM path.
- The Windows panel intentionally has no separate **Open Settings** shortcut for
  this option; the app already controls the complete supported setting group.
- When automatic follow is enabled, the state follows whether any selected
  e-ink display is active.

Current shutdown behavior calls the platform to return visual effects to the
non-reduced state; it does not yet retain a complete bit-for-bit snapshot of
every pre-launch animation preference. This is known technical debt. Do not
claim exact restoration until a captured-state implementation and test exist.

### 8.2 Windows Light Mode

Windows Light Mode is a separate session-only section, available on build 18362
or later. It writes **only** `SystemUsesLightTheme`. It intentionally does not
write `AppsUseLightTheme`.

It does not follow e-ink connection state. Opening the app never changes the
setting. The first in-app change captures the current Windows value, and normal
quit restores that exact value.

### 8.3 Night Light

Night Light is system-wide on Windows, not per display. Microsoft publishes
`ms-settings:nightlight` for opening its page, but no public API for changing
its state. The MHC display pipeline is composed with OS-owned features such as
Night Light, so an app ICC profile cannot cancel it.

A viable private workaround exists on compatible Windows 10 and Windows 11 builds: the MIT-licensed
[`win-nightlight-cli`](https://github.com/kvnxiao/win-nightlight-cli) project
parses the authoritative `DefaultAccount\Current` CloudStore record as a CloudStore
envelope containing Microsoft Bond CompactBinary v1. Presence of inner field 0
represents force-enabled state, while fields 10 and 20 carry initialization and
transition metadata. On the tested build 26100 machine, the authoritative
`Current` disabled-state record is 41 bytes and matches that documented shape;
the separate short `DefaultAccount\Cloud` record is only a mirror and must not be
used as the state source.

The app now implements this workaround as **Disable Night Light**, above
Windows Light Mode. It is visible only when the build is at least 19041
(Windows 10 2004) and the live payload passes a strict complete-format parser.
This is a capability gate, not a claim that Night Light technically began with
that release. On Windows 10 1703 through 1909, or on any newer build whose
payload fails validation, the global switch is replaced by one recommendation
to disable Night Light, the path **Settings > System > Display > Night light**,
and a button that opens the native page. The fallback is global rather than
duplicated inside every display card. On Windows 10 before 1703 and on Windows
7/8, where Night Light is not present, the entire Night Light section—including
the recommendation and Settings shortcut—is absent.

The switch is an inverse live view of the Windows state: **On** means Night
Light is disabled and **Off** means Night Light is enabled. A two-second
low-cost status check updates the control if Windows or another application
changes Night Light; it does not fight or overwrite that external change.
Rapid clicks are coalesced for 220 ms and only the final requested state is
written. The status poll yields while a choice is pending, preventing it from
snapping the switch back to a stale system value.

Before the first in-app change, the exact original state blob is saved through
an atomic crash-recovery journal. Both switch directions update the canonical
state and both timestamps, then read, parse, and verify the result. Turning the
switch Off does **not** discard the snapshot or make the newly enabled state
permanent. It never writes the schedule record, so the user's schedule is
preserved. Normal quit and the next launch after an abnormal exit restore the
original On/Off state with fresh transition timestamps, then verify the result
before deleting the journal. Reusing the old saved timestamps is forbidden:
Windows can persist that backward-dated blob without applying it to the live
color pipeline, causing the next launch to read a state that disagrees with the
screen. No Rust runtime or helper process remains resident.

Do not replace this with the reference CLI's high-level `off` operation: that
operation also disables the user's schedule. Format attribution and the MIT
license are included in `docs/THIRD-PARTY-NOTICES.md`.

## 9. UI, rendering, and interaction decisions

### 9.1 Visual structure

The Windows panel follows the macOS high-contrast hierarchy while using Qt
Widgets. Its nominal width is 680 logical pixels. Its height is content-driven:
compact content stays compact, while long content grows only to the target
screen's available height minus a 16-pixel top and bottom gap. It is bottom
anchored.

Text Contrast and Video Enhance use the same continuous segmented-control shape
as the macOS app: rounded outer corners, straight internal separators, and no
gaps between segments. A selected choice remains white and is indicated only by
heavy (`QFont::Black`) text plus a thick straight bottom underline; the rounded side border does
not become thicker. Saturation presets use the same underline-only selection
language while remaining separate rounded buttons. Preset buttons reserve
enough checked-state width that **Factory** and localized text cannot be clipped.

The panel includes a visible **−** control that hides it and a **Quit** control
that performs restoration and exits. Losing focus hides the panel; it does not
terminate the tray process. The panel must not hide while its own popup or a
configuration transition is active.

Mouse-wheel input over a slider scrolls the containing panel vertically and
does not change the slider value. This avoids accidental tuning while navigating
a long display card.

### 9.2 Focus indication

Mouse clicks must not leave Qt's dotted focus rectangle on option buttons,
switches, or display selectors. These widgets use a focus policy that keeps
keyboard Tab focus but avoids mouse-acquired focus decoration. Do not remove
keyboard accessibility merely to hide the mouse rectangle.

The normal panel contract remains “hide on focus loss.” The one temporary
exception is a direct Night Light toggle on Windows 11: its UI Automation path
must open Settings, so the panel retains itself while that operation owns the
focus transition. Completion clears the guard and reactivates the panel.
Subsequent ordinary focus loss hides it normally; minimizing during the
operation is still respected and does not reopen the panel.

### 9.3 Icons

- Display selectors use vector-drawn monitor/laptop geometry and a real
  checkmark, not font glyphs such as `□`, `▪`, or language-dependent symbols.
- The notification-area icon is a code-drawn monochrome equivalent of the
  macOS `book.pages` tray icon. It adapts using `SystemUsesLightTheme`.
- The main executable/window retains the colored application icon.

### 9.4 CJK fonts and smooth text

Language font families are selected explicitly:

| Locale | Preferred families |
| --- | --- |
| English | Segoe UI |
| Simplified Chinese | Microsoft YaHei UI, Microsoft YaHei |
| Traditional Chinese | Microsoft JhengHei UI, Microsoft JhengHei |
| Japanese | Yu Gothic UI, Meiryo UI, Meiryo |

Fonts request antialiasing and no hinting. `SmoothLabel`/`SmoothProxyStyle`
composite text through a transparent image to obtain consistent grayscale
antialiasing closer to modern browser rendering. Do not depend on a single
pan-CJK fallback font; it produces inconsistent glyph style and weight across
languages.

Supported UI locales are English, Simplified Chinese, Traditional Chinese, and
Japanese. Every new user-facing string must be added to all four localization
resources and exercised by localization tests.

### 9.5 Rounded windows

Rounded corners are painted by Qt. Both the main panel and first-run welcome
window use a translucent background and antialiased rounded-body painting. The
welcome window is a plain rounded card without a directional arrow.

Do not use:

- Windows 11 DWM corner attributes;
- a rectangular native white backing window;
- `QRegion` polygon/rounded masks, which create visibly jagged edges.

The main panel radius is approximately 10 logical pixels, matching the Windows
11 visual scale while remaining compatible with Windows 7.

## 10. DPI and multi-monitor placement

High-DPI application attributes are set before `QApplication` is created:

- `EnableHighDpiScaling`;
- `UseHighDpiPixmaps`;
- pass-through scale-factor rounding.

The manifest declares PerMonitorV2/PerMonitor awareness for supported systems.

When a tray icon on another monitor opens the panel for the first time, Windows
may otherwise calculate size using the previous monitor's DPI and place the
window incorrectly. `MainPanel::showPanel(QScreen*)` therefore:

1. resolves the intended target screen;
2. ensures the native window exists while hidden;
3. binds `QWindow::setScreen` before calculating geometry;
4. sizes and anchors immediately;
5. repeats anchoring at 0 ms and about 80 ms after Windows completes the DPI
   transition;
6. uses a generation token so an older delayed placement cannot move a newer
   show request.

The welcome window binds to the primary screen and positions inside its
available bottom-right area, with a delayed reposition after its final size is
known. It no longer depends on private tray-icon geometry.

## 11. Responsiveness and busy feedback

The GUI thread must remain available to paint and handle window messages during
display configuration.

`ApplicationController` owns a `QThreadPool` with a maximum thread count of one
for color work. Saturation/RGB profile operations are serialized because the
Windows color store and per-display recovery records are transactional shared
state. Hotplug reapply also enters this queue when invoked from the controller
thread.

Reduce Transparency & Motion and Windows Light Mode use a separate serialized
system-settings pool. Their switches disappear immediately after a click and a
small monochrome transparent spinner occupies the same 58-by-32 logical slot
until Windows confirms the resulting state. These two operations deliberately
do not trigger the header-level configuring indicator. Completion restores the
switch and synchronizes both its position and adjacent On/Off text to the
queried system value. Shutdown waits for this pool before restoring state.

Disable Night Light uses the same local spinner pattern. While its Windows 11
UI Automation helper temporarily transfers focus to Settings, a dedicated
focus-retention flag prevents the tray panel's general focus-loss handler from
hiding it. The controller emits an unconditional completion signal so the flag
cannot remain stuck after either success or failure.

Fast operations do not display a busy indicator. The inline status begins only
after approximately 250 ms, avoiding a distracting flash for an option that
completes immediately. While the panel exists, feedback is embedded in the
panel header; a separate modal configuration window is not shown. The spinner
is monochrome with a transparent background, and the application does not set a
global busy cursor.

Startup may use `BusyDialog` before the panel is constructed. Quit immediately
hides the panel, welcome window, context menu, and tray icon, then runs
controller restoration on a worker. A short event-loop poll ends the process
only after restoration finishes; delayed activation cannot reopen the panel.

Operation counters/signals must stay balanced. Do not emit a finished event for
work that never emitted a start, and do not allow a short operation to cancel a
long operation's visible status.

## 12. Tray, single-instance, and first-run behavior

The process is a tray application. Hiding the panel leaves it running.

- A named local mutex enforces one instance on Windows; the reusable non-Windows
  path retains a temporary `QLockFile` fallback.
- A second launch locates/foregrounds the existing panel instead of starting a
  second controller.
- Left-clicking the tray icon toggles the panel.
- Quit hides the tray icon and windows immediately, then restores temporary
  display state before the process ends.

### First-run manual tray guidance

Windows owns whether a new notification-area icon starts in the visible area
or the hidden-icons menu. E-Ink Assistant deliberately performs no private COM,
`IconStreams`, `NotifyIconSettings`, toolbar-inspection, or shell-restart work
to override that choice.

The first-run welcome window places a dedicated tray section at the top. It
renders the same monochrome book-pages glyph used by the live tray icon and
asks the user to:

1. open hidden icons (`^`) at the right side of the taskbar;
2. find the shown book-pages icon;
3. drag it into the visible notification area for easier future access.

The welcome window has no directional arrow and makes no claim that it points
to the icon. Its position therefore remains stable across Windows 7 through
Windows 11, replacement taskbars, multi-monitor layouts, and Explorer format
changes. Moving or rebuilding the executable does not trigger any tray
preference mutation.

An enabled Launch-at-Login task is automatically reconciled to the current real
application path on the next successful launch.

## 13. Persistence

General and per-display settings are written through `QSettings` to the user's
application configuration area (`settings.ini`). The organization/application
identity is `EinkAssistant` / `E-Ink Assistant`.

Persisted general state includes:

- language;
- launch at login;
- Reduce Transparency & Motion and automatic-follow state;
- first-run welcome preference; legacy tray-discovery values remain readable
  only for settings-file compatibility and do not trigger shell integration.

Persisted per-display state includes:

- stable ID and e-ink selection;
- saturation preset/value and RGB values;
- Text Contrast and Video Enhance selection;
- Advanced curve values;
- five saved custom curves.

The legacy `reduceShaking` key is still read/written for settings compatibility
and possible other platform backends. It must not cause the Windows UI to
reintroduce Reduce Shaking.

Migration rules:

- tolerate missing keys and older files;
- prefer a stable default rather than discarding the whole file;
- do not key display settings by translated display name;
- update persistence tests whenever a key is renamed or its semantics change.

## 14. Build and package

Required toolchain:

- Qt 5.15.2 `mingw81_64`;
- MinGW 8.1 x64;
- CMake;
- Ninja;
- PowerShell 7.

From the repository's `windows/` directory:

```powershell
./scripts/build-windows.ps1
```

Optional parameters:

```powershell
./scripts/build-windows.ps1 -Configuration Debug -QtRoot 'D:\Qt' -BuildDirectory 'build-debug'
```

The script configures `build/`, runs the build, copies the executable, required
Qt/MinGW DLLs and plugins, licenses, `README.md`, and this engineering reference
into `artifacts/E-Ink-Assistant-Windows/`, runs automated core/E2E tests, and
creates `artifacts/E-Ink-Assistant-Windows.zip` with a SHA-256 hash.

The shipped runtime is x64. Do not run `windeployqt` blindly and accept its
entire output: the current explicit dependency list is intentional for package
size and auditability. If a new Qt module is linked, update both CMake and the
package list.

If the canonical ZIP is open in Explorer or another program,
`Compress-Archive -Force` can fail. Close the handle or use a uniquely named
verification artifact; do not delete unrelated artifacts to work around a lock.

## 15. Automated and hardware testing

### 15.1 Required release baseline

Every release change must, at minimum, pass:

```powershell
./build/bin/eink_core_tests.exe
./build/bin/eink_e2e_tests.exe -platform offscreen
```

The packaging script runs both automatically. Offscreen execution proves state
and most widget behavior but cannot prove native font rendering, Explorer tray
geometry, native focus transitions, real mixed-DPI placement, or actual GPU
color behavior.

Run targeted native Qt tests with the Windows platform plugin when changing
those areas, for example:

```powershell
./build/bin/eink_e2e_tests.exe firstShowBindsToRequestedScreen -platform windows
```

Use the exact test function name listed by the current test binary; names can
change as tests are refined.

### 15.2 Core coverage

Core tests cover, among other cases:

- tone-curve endpoints, monotonicity, and upstream preset constants;
- ICC structural validity and saturation/RGB round trips;
- settings persistence and saved curves;
- Text/Video mutual exclusion;
- all localization catalogs;
- serialized color work occurring off the UI thread;
- Factory remaining an active identity profile;
- abnormal-exit recovery ordering and cleanup.

### 15.3 UI/E2E coverage

E2E tests cover, among other cases:

- a complete user journey with the fake platform;
- the Windows 7 capability path and absent unsupported controls;
- the Windows 10 MHC2 UI path without pretending ACM is enabled;
- OS-aware unsupported-saturation copy, exact display-to-GPU attribution, and
  Intel/NVIDIA/AMD panel-button routing without exposing color sliders;
- first-run choice persistence;
- no unnecessary display-card rebuild;
- wheel scrolling over sliders;
- minimize/focus-loss behavior;
- CJK font selection;
- delayed inline busy feedback and no flash for fast operations;
- hotplug stability and reapply;
- tray `book.pages` glyph rendering;
- Windows 10/default color-pipeline decisions and the legacy tray strategy;
- vector selector/check icons;
- first show binding to the requested screen;
- wide layout, Factory clipping, continuous segmented mode rows, selected
  underlines, and adaptive height;
- absence of dotted focus frames after mouse clicks;
- Night-Light-only focus retention while Settings is active, followed by normal
  focus-loss hiding after completion;
- Qt-painted rounded main window.

### 15.4 Real-system diagnostics

`eink_hardware_smoke.exe` performs a gamma-ramp identity write and immediate
restore. Additional diagnostic entry points in the application include:

```text
--color-self-test <report>
--color-cleanup <report>
--color-visual-test <report> <saturation> <hold-seconds>
--color-crash-seed <state>
--color-recovery-test <state>
--tray-anchor-test <output>
--launch-at-login-self-test <report>
--color-broker <pipe-name>
```

GPU-panel launch is covered by an opt-in core-test case so routine automation
does not open an external application. For example, on an AMD display path:

```powershell
$env:EINK_GPU_PANEL_LAUNCH_TEST = 'amd'
./build/bin/eink_core_tests.exe liveGpuPanelLaunchSmoke
Remove-Item Env:EINK_GPU_PANEL_LAUNCH_TEST
```

These are engineering interfaces, not end-user commands. Several intentionally
mutate live display, profile, registry, or scheduled-task state. Before running
one on a user's computer:

1. capture the original state;
2. use the smallest test delta;
3. verify restoration after the test, not merely a zero exit code;
4. inspect Windows Color Management for leftover generated profiles;
5. retain the report when investigating a failure.

For color changes, success requires all of the following: the correct display
changed, the requested default association is active, the visual result is
plausible, the original state returns after cleanup, and no generated duplicate
remains.

Windows 10 MHC2 hardware verification (2026-09-01): Windows 10 IoT Enterprise
LTSC 2021, build 19044.7663; AMD Radeon 780M driver 32.0.21030.2001, WDDM 2.7;
external `ICNM 8001H0`, 3200x1800 at 46 Hz. The user confirmed that saturation
changes were visible on the physical panel. This verifies the transform on
that configuration only; profile restoration and additional GPU/display
combinations remain release-check items.

## 16. Resource and performance policy

The low-memory strategy is architectural:

- no WebEngine, WebView, QML, service, or resident helper;
- one Qt GUI process and one serialized worker pool;
- the color broker starts only when an MHC2/ACM profile path is used and remains scoped to
  that application session;
- display cards are updated in place where possible;
- reconnection events are debounced;
- fast operations avoid unnecessary animation and modal windows.

Do not publish a fixed memory number as a permanent guarantee. Working set and
private bytes vary with Qt DLL sharing, DPI, language fonts, display count, and
Windows version. For a release claim, measure the packaged Release executable
after idle stabilization and record both working set and private bytes plus the
test environment.

## 17. Security and safety boundaries

- The main process is elevated. Treat every path, registry key, pipe message,
  and profile filename as untrusted input even when it originates from saved
  settings.
- The broker pipe protocol must validate command shape and restrict operations
  to expected color-profile actions.
- Task Scheduler registration must target the exact current executable and the
  current interactive user.
- Cleanup may remove only positively identified app-generated color artifacts.
- Private Explorer reads/writes are per-user and must not scan or mutate
  unrelated notification entries.
- Never add a service, driver, broad firewall rule, or machine-wide autostart as
  an incidental implementation detail.

## 18. Known limitations and technical debt

1. **Manual UAC is broad.** Every manual launch prompts because the entire
   executable requires elevation, even when the user only opens the panel.
2. **Gamma behavior is driver-dependent.** `SetDeviceGammaRamp` does not promise
   identical hardware response across GPUs and HDR modes.
3. **Windows 10 MHC2 remains hardware-dependent.** Build 19044 lacks the
   capability entry point on the tested image. The test configuration produced
   a visible saturation change, but other hardware still requires a physical
   check and exact restoration verification.
4. **Tray placement is user-controlled.** Windows may initially place the icon
   in hidden icons. The welcome guide shows the exact glyph and manual drag
   path; the app does not force promotion.
5. **Visual-effects restoration is not a complete original-state snapshot.**
   It currently returns to the non-reduced state.
6. **An already-running old build must be quit once during an upgrade.** Its
   single-instance lock prevents the new executable from initializing and
   reconciling the scheduled-task path until the older process exits.
7. **Windows 7 is code-path tested, not exhaustively hardware certified.** Qt,
   driver, TLS/certificate, and GPU differences still require a real VM or
   machine release check.
8. **Startup feedback is still a separate dialog before panel construction.**
   Runtime configuration and quit feedback are inline by design.
9. **Old `reduceShaking` state remains in persistence/controller interfaces.**
   It exists for compatibility but is not a Windows feature.
10. **Main-process elevation makes the broker partly redundant.** Removing or
    repurposing it requires a deliberate migration and recovery audit.
11. **GPU control-panel launch targets are best-effort.** Registered shell apps
    and known Intel/NVIDIA/AMD locations cover current and legacy packages, but
    OEM repackaging can omit or rename the panel. The app hides the button when
    no installed target is verified and never claims to adjust saturation
    through a vendor API.

## 19. Rules for future coding agents

Before changing behavior:

1. Read this document, `../README.md`, `src/platform/PlatformServices.h`, the relevant Windows
   backend method, and its tests.
2. Identify the oldest Windows version on which the changed code can load. Use
   runtime lookup for APIs that are not present on Windows 7.
3. Keep OS/build gates in the platform capability result and backend guard;
   avoid scattered UI-only version comparisons.
4. Preserve stable display identity and per-display recovery state.
5. Put potentially slow profile/topology work off the GUI thread and preserve
   serialization.
6. Prefer updating existing widgets/state over rebuilding the panel.
7. Add every string to all four locales.
8. Add a regression test for the reported failure, then run the full core and
   E2E baseline.
9. For native/DPI/tray/font/color changes, perform a real Windows verification;
   offscreen Qt is insufficient.
10. Update this document when a decision, gate, preset, lifecycle, test command,
    or known limitation changes.

Do not:

- enable ACM based only on an OS version;
- disable ACM for Factory;
- key settings by friendly display name;
- perform profile installation synchronously on the GUI thread;
- show unsupported rows on Windows 7;
- reintroduce Reduce Shaking on Windows without a safe public per-display API;
- replace Qt corners with a Windows 11-only native effect;
- use a font glyph as a checkbox/monitor icon;
- treat API success as proof of visible hardware success;
- remove a profile or scheduled task that the app cannot positively identify as
  its own.

## 20. Release checklist

- [ ] Version and preset tables match code and upstream product intent.
- [ ] Core tests pass.
- [ ] Offscreen E2E tests pass.
- [ ] Native Windows UI tests pass for changed UI/DPI/focus behavior.
- [ ] Connect/disconnect/reconnect the e-ink display while the app is running.
- [ ] Open the panel from tray icons on monitors with different DPI values.
- [ ] Verify unsupported adjustment controls are absent on simulated/real older
      Windows and that any GPU-panel button matches the selected display adapter.
- [ ] Verify Factory causes no ACM disable flicker.
- [ ] Force-kill during a generated profile, restart, and verify recovery leaves
      no duplicate profile.
- [ ] Verify Reset RGB returns all channels to 100% and the live profile follows.
- [ ] Verify normal Quit restores gamma/profile/ACM temporary state.
- [ ] Verify Launch at Login task XML has no AC-power restrictions and starts
      elevated without an interactive UAC prompt.
- [ ] Verify the first-run window has no arrow, shows the exact book-pages tray
      glyph, and gives clear manual hidden-icons pinning steps in all locales.
- [ ] Inspect English, Simplified Chinese, Traditional Chinese, and Japanese at
      100%, 125%, 150%, and mixed-monitor DPI where available.
- [ ] Package contains executable, minimal DLL/plugin set, licenses,
      `README.md`, `TECHNICAL.md`, and `THIRD-PARTY-NOTICES.md`.
- [ ] Installer build validates that package, installs under Program Files,
      registers uninstall metadata and shortcuts, and blocks replacement while
      the running tray process owns its single-instance mutex.
- [ ] Completion-page launch inherits Setup's administrative token; do not run
      the `requireAdministrator` executable with the original unelevated token.
- [ ] Record ZIP SHA-256 and real-system verification notes.
