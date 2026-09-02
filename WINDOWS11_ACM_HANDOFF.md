# Windows 11 ACM return handoff

## Start here

Continue in the existing checkout:

```text
D:\Users\Admin\einkwindows
```

- Branch: `sub/hardware-saturation`
- Starting commit: `457fdda` (`v2.3`)
- The Windows implementation and this handoff are **uncommitted working-tree
  changes**. Do not reset, clean, switch branches, or replace the checkout.
- The Windows port is largely untracked in this checkout. Treat every existing
  file under `src/`, `tests/`, `scripts/`, and the Windows documentation as
  user work.
- Use only 64-bit PowerShell 7 at
  `C:\Program Files\PowerShell\7\pwsh.exe`, in UTF-8 mode. Never use Windows
  PowerShell 5.1.
- Read `WINDOWS-TECHNICAL.md` in full before changing the Windows backend.

## What returned from Windows 10

Windows 10 saturation is now physically verified on this test configuration:

- Windows 10 IoT Enterprise LTSC 2021, build 19044.7663;
- AMD Radeon 780M, driver 32.0.21030.2001, WDDM 2.7;
- external `ICNM 8001H0`, 3200x1800 at 46 Hz;
- the user confirmed that changing saturation visibly changed the physical
  panel.

The tested Windows 10 `mscms.dll` does not export
`ColorProfileGetDeviceCapabilities`, so that API cannot certify MHC2 support on
this image. The visible hardware result is the evidence for this exact
configuration. Restoration and other GPU/display combinations still require
separate checks.

## Implemented runtime color routing

There is now one Windows package and one executable. The old compile-time
`EINK_WINDOWS10_MHC2_COMPAT` variant and `-Windows10Compatibility` package were
removed.

Runtime selection is centralized in
`src/platform/windows/WindowsCompatibility.cpp`:

1. Windows 10 builds 19041 through 19045:
   - require the modern `ColorProfile*` list/default/association APIs;
   - if `ColorProfileGetDeviceCapabilities` exists, require MHC2 support for
     the exact adapter LUID and source ID;
   - if the capability API is absent, allow the physically verified MHC2
     profile path and label it hardware/driver-dependent;
   - never query or toggle Windows 11 ACM/WCG state.
2. Windows 11 24H2 and newer, build 26100 or later:
   - require the modern profile APIs;
   - require per-display ACM support from DisplayConfig advanced-color info;
   - use the Windows 11 ACM path and enable/restore ACM transactionally.
3. Windows 11 builds below 26100 and older unsupported systems do not expose
   Saturation/RGB.

Both paths retain the existing generated MHC2 profile transaction, serialized
color worker, crash-recovery journal, original profile/scope restoration, and
generated-profile cleanup.

Important source locations:

- `src/platform/windows/WindowsCompatibility.{h,cpp}` — pure runtime decision;
- `src/platform/windows/WindowsPlatformServices.cpp` — enumeration, MHC2/ACM
  capability checks, apply/restore, and diagnostic;
- `src/core/AppState.h` — `colorAdjustmentSupported` and
  `usesWindows10Mhc2` per-display results;
- `src/ui/DisplayCard.cpp` — Windows 10 MHC2 versus Windows 11 ACM caption;
- `tests/CoreTests.cpp` — build/capability routing matrix;
- `tests/E2ETests.cpp` — Windows 10 MHC2 UI path.

## Windows 11 work to verify next

Use the packaged executable from this handoff first. On the Windows 11 24H2+
system, verify all of the following with this exact build:

1. Confirm the OS build is at least 26100.
2. Confirm the platform diagnostic reports `Windows 11 ACM`.
3. Confirm only displays reporting ACM support expose Saturation/RGB.
4. Run the smallest color self-test and retain its report:

   ```powershell
   ./artifacts/E-Ink-Assistant-Windows/EinkAssistant.exe --color-self-test ./artifacts/windows11-acm-return-test.txt
   ```

5. Confirm the generated profile became the exact display's selected default.
6. Confirm the original profile, association scope, and ACM state were restored.
7. Confirm no `EinkAssistant-*.icm` file or duplicate association remains.
8. Visually test a conservative saturation delta on the intended physical
   panel; screenshots are not proof of a scanout transform.
9. Confirm a second display remains unchanged.
10. Exercise Factory and confirm it keeps a neutral generated profile active
    without toggling ACM or visibly flashing.

Do not run a live profile mutation until the original per-display default,
scope, and ACM state have been captured. Do not infer ACM support from the OS
build alone.

## Windows 10 tray finding and safe workaround

The native Explorer test found the app in overflow with this state:

```text
items=1 mainToolbars=2 overflowWindows=1
overflow=1 hidden=0 index=6 command=6 owner=0x80944 id=0 rect=0,0,0,0
```

This explains the old false success. `NIS_HIDDEN` was already clear: the icon
was active, but Explorer's separate user preference kept it in overflow.
`Shell_NotifyIcon(NIM_MODIFY)` can successfully clear an already-clear hidden
flag without promoting the icon.

The shipped Windows 10 workaround is deliberately safe:

- recursively find the main and overflow toolbars;
- match only the tray callback window owned by the current process;
- if it is in overflow, request a temporary notification preview;
- tell the user to drag the book icon onto the taskbar once;
- re-detect the exact icon rather than claiming promotion from API success.

Do not revive the old private `ITrayNotify::SetPreference` sample. Its marshaled
structure changed after Windows 10 Fall Creators Update and it crashes on the
tested build 19044 path. Do not rewrite the global `IconStreams` blob or enable
the global “show all icons” setting.

Windows 11 continues to use the isolated exact-path
`HKCU\Control Panel\NotifyIconSettings` `IsPromoted=1` path, with stale
same-filename entries removed. Re-test this on Windows 11 because Explorer's
private layout can change.

Relevant tray source and test:

- `src/platform/windows/WindowsTrayIntegration.{h,cpp}`;
- `tests/E2ETests.cpp::windows10TrayPromotionFindsOwnIcon`.

## Documentation updated

The following now describe the unified runtime package and the verified limits:

- `WINDOWS-TECHNICAL.md` — authoritative architecture, gates, tray diagnosis,
  Windows 10 hardware evidence, tests, and remaining limitations;
- `WINDOWS.md` — user/build compatibility summary;
- `README.md` — one-package Windows summary;
- all four localization resources and built-in localization fallbacks.

## Build and verification already completed

The handoff build used:

```powershell
./scripts/build-windows.ps1 -QtRoot 'D:\Users\Admin\Qt' -BuildDirectory 'build-runtime-routing'
```

Results on Windows 10:

- core: 16 passed, 0 failed;
- offscreen E2E: 20 passed, 0 failed, 6 skipped;
- native Windows tray test: 3 passed, 0 failed;
- `git diff --check`: clean (line-ending conversion warnings only).

Test reports:

- `artifacts/core-routing-tests.txt`;
- `artifacts/e2e-routing-tests.txt`;
- `artifacts/tray-windows10-test.txt`.

## Current artifacts

```text
artifacts\E-Ink-Assistant-Windows\EinkAssistant.exe
SHA256 70106A5539A9FD02B7407370C505C4F31656187069D60D4A34BD00A0F1C44941

artifacts\E-Ink-Assistant-Windows.zip
SHA256 9261B37A526809540416610EBA2EE2B722718BBC07CAFFE93E5580D2612F311B
```

The package contains the same executable for Windows 10 MHC2 and Windows 11
24H2+ ACM. Do not use the older `E-Ink-Assistant-Windows10` artifact for the
return verification.

## Completion criteria for the Windows 11 session

Return with:

- exact Windows 11 build, GPU, WDDM/driver, and display identity;
- platform diagnostic and per-display ACM capability/state;
- original profile/scope/ACM state;
- whether the exact packaged build visibly changed the target display;
- proof that profile/default/ACM restoration completed with no leftovers;
- whether other displays remained unchanged;
- Windows 11 tray promotion result and exact registry entry/path match;
- any API/HRESULT/Win32 errors at the step that produced them;
- updated `WINDOWS-TECHNICAL.md` if the observed behavior changes a recorded
  decision, gate, lifecycle rule, or limitation.
