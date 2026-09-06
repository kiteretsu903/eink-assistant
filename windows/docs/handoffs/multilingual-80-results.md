# Windows multilingual integration results

Date: 2026-09-06. Development integration is implemented and the current-machine
automated checks pass. **This is not full release acceptance for every locale
on every supported Windows version. No release or push was made.**

## Source and environment

- Branch: dev/multi-lingual, fetched and switched from master.
- Base revision: da6c6a0bf2974f19487c25d466324898fadd808b.
- The source SHA-256 inventory captured before committing is
  [source-manifest.json](../../artifacts/localization/source-manifest.json).
- Pre-existing untracked files ($log, $out, $report and AGENTS.md) were preserved.
- Host: Windows 11 IoT Enterprise LTSC, build 26300, x64.
- Qt 5.15.2, MinGW 8.1.0 x64, CMake 3.30.5, Ninja.
- CMake selected Python 3.12.7 from D:/ProgramData/anaconda3. Generation also
  passed with the Codex bundled Python. The Windows Store python alias was
  unavailable; no Python runtime is shipped with the application.
- Native Qt Windows runs used QT_SCALE_FACTOR=1, 1.5 and 2. Windows display
  settings were not modified. These are Qt scale-factor tests on this host,
  not separate physical mixed-DPI or legacy-OS machines.
- Installed Windows UI languages: en-US, ja-JP, zh-CN. Font-file inventory:
  [installed-font-files.csv](../../artifacts/localization/installed-font-files.csv).

## Implementation

- All 80 registry locales, autonyms and directions are embedded. The shared
  generator owns resources.qrc; CMake rejects stale catalogs before AUTORCC.
- Runtime loads English shared/platform fallbacks, then selected shared/platform
  strings, with platform overrides last. JSON string-literal decoding preserves
  escaped backslashes, quotes, tabs, line breaks and Unicode.
- The 81-entry language picker uses installed script-aware fonts and a scrollable
  popup. Explicit settings remain compatible; preferred-language matching handles
  exact tags, Chinese scripts/regions, Portuguese variants and unknown locales.
- RTL switching mirrors navigation and custom control outlines while preserving
  slider direction, RGB channel meaning and tone-curve coordinates.
- Rows measure polished, bold font metrics and grow the panel within the screen;
  very narrow work areas retain scrolling. Long safety action labels wrap and
  both buttons stay inside the overlay when the locale changes.
- Tray labels, welcome content, warnings and the active safety dialog retranslate.
  A modal preset rename keeps its parent alive and preserves entered text while
  its app-localized Rename/Not Now buttons update.
- Every failed ApplyResult receives an AI-translated generic failure summary in
  all 80 locales. Original English backend prose, numeric codes and hardware
  details remain selectable, expandable diagnostics. Worker threads do not read
  mutable UI localization state.
- Display capability gates, recovery, color serialization and the production
  watchdog are preserved. Only the existing test timing hook gained the ability
  to update its active timer for fake-platform locale sweeps.

## Validation

The shared generator check passed: 80 locales, 117 shared app keys and 41 Windows
keys per locale; no missing/extra keys or placeholder-validation errors.
The existing Python translation-contract suite passed all 10 tests.

Run from the repository root (use a working Python 3 executable):

    python scripts/build-localizations.py --check
    python -X utf8 -m unittest discover -s tests -v
    ./windows/scripts/build-windows.ps1
    ./windows/build/bin/eink_core_tests.exe -o windows/build/core-results.txt,txt
    ./windows/build/bin/eink_e2e_tests.exe -platform offscreen -o windows/build/e2e-results.txt,txt

Full core: Totals: 25 passed, 0 failed, 1 skipped, 0 blacklisted, 1930ms
Full offscreen E2E: Totals: 30 passed, 0 failed, 5 skipped, 0 blacklisted, 32229ms

The core skip is the opt-in GPU panel launch test. The five offscreen skips are
native font/placement/layout/focus checks; they are not counted as passes.

Native Windows commands, repeated at each scale with EINK_LOCALE_CAPTURES set
to the corresponding output directory:

    ./windows/build/bin/eink_e2e_tests.exe allLocalesNativeJourney languageSwitchDuringSafetyTest cjkLanguagesUseWindowsUiFonts widerLayoutKeepsLabelsVisible modeButtonsAreContinuousAndPanelUsesScreenHeight -platform windows
    ./windows/build/bin/eink_e2e_tests.exe allLocalesExtendedControls -platform windows

| Scale | Main/welcome/safety and native layout group | Expanded controls and rename group |
| --- | --- | --- |
| 100% | Totals: 7 passed, 0 failed, 0 skipped, 0 blacklisted, 42720ms | Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 34535ms |
| 150% | Totals: 7 passed, 0 failed, 0 skipped, 0 blacklisted, 57162ms | Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 39987ms |
| 200% | Totals: 7 passed, 0 failed, 0 skipped, 0 blacklisted, 70818ms | Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 46159ms |

Each locale loop covers all 80 codes. Init/cleanup cases are included in Qt's
reported totals. There are **1,200 generated screenshots**: main, welcome,
safety, expanded RGB/advanced/help, and preset rename for every locale at three
scales. Expanded screenshots capture the full scrollable content widget;
other captures show the window client area.

The [native matrix](../../artifacts/localization/native-matrix.csv) lists each
locale/OS/scale, evidence files and the representative views visually inspected.
Complete logs are retained beside it. Screenshots were generated for all entries;
only representative views were visually reviewed. Generation is not exhaustive
glyph/shaping certification.

## Package

Local staged directory: windows/artifacts/E-Ink-Assistant-Windows/.
[Local ZIP](../../artifacts/E-Ink-Assistant-Windows.zip), 13,507,804 bytes.

SHA-256: 04C558CB833046768B5E3A3A384E4906F7A922C0E176A2CF8BC75A5E351386DF

The staged executable hash matches the final build executable. Packaging ran
the existing core and offscreen E2E commands successfully. Full final tests were
also recorded explicitly after adding the extended-dialog regression.

The installer remains **four locales** and was not built or published.
The real staged app's hardware-affecting startup, clean-machine deployment and
actual Explorer tray interaction were **NOT RUN** in this work.

## Review and remaining acceptance

The 80-locale generic error summary was AI-translated and reviewed; its provenance
is in [error-summary-review.json](error-summary-review.json). Existing translations
retain their original AI review records. No native-speaker review is required.

- Windows 7 and Windows 10 native startup/font/render sweeps: **NOT RUN**.
- Qt emitted fallback OpenType warnings on this machine. Representative Arabic,
  Indic, Myanmar, Khmer and Ethiopic images were inspected, but a full
  per-string glyph/shaping audit remains open.
- Actual tray popup/overflow screenshots, OS-owned UAC/Settings language-boundary
  captures, keyboard traversal across every locale, and physical mixed-DPI
  monitor transitions are not fully covered by these snapshots.
- Clean-machine/real packaged application lifecycle and installer: **NOT RUN**.
- No hardware-changing smoke commands were executed. Existing automated core
  tests include a read-only live display/GPU mapping probe; fake platforms were
  used for color, Night Light and safety workflows. Physical display/color
  restoration acceptance remains **NOT RUN**.
- Existing published downloads/site release claims remain unchanged. Complete
  the outstanding acceptance before claiming an 80-locale Windows release.
