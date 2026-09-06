# Windows 80-locale integration handoff

This is an **active implementation handoff**, prepared on macOS on 2026-09-06.
The shared translation work targets 80 locales for both applications and the
website, with a focused 12-locale main README. Windows native integration and
verification remain to be performed on Windows. This document does not claim
that the current Windows executable supports 80 locales.

- Branch: `dev/multi-lingual`.
- Starting revision: `d2c18455f8e40a35d3d784545c1117fc489eb8fd`.
- Delivery: the user authorized committing and pushing this branch for Windows.
  Use the exact pushed commit reported with the handoff; the starting revision
  above is historical and does not contain the multilingual implementation.
- In a clean Windows checkout, fetch `origin`, switch to `dev/multi-lingual`
  tracking `origin/dev/multi-lingual`, and confirm `git rev-parse HEAD` matches
  the handoff commit. Preserve any pre-existing Windows changes.
- Windows architecture and behavioral requirements remain defined in
  [`../TECHNICAL.md`](../TECHNICAL.md). Current build instructions are in
  [`../../README.md`](../../README.md).
- No Windows C++, Qt resource registration, installer, or hardware behavior was
  changed as part of preparing this handoff.
- **Windows native build: NOT RUN. Windows runtime/GUI: NOT RUN. Windows
  packaged application: NOT RUN. Installer: NOT RUN. Hardware tests: NOT RUN.**
  Shared catalog validation is not evidence of any of these outcomes.

## Source contract

AI translation and review satisfy linguistic acceptance. Native-speaker review
is not required; original translator suggestions do not add a handoff gate.
The native integration and verification requirements below still apply.

Use repository-root paths below. Read the current generator and localization
documentation before integration; do not maintain another hardcoded language
list on Windows.

| Source | Contract |
| --- | --- |
| `localization/locales.json` | JSON array of exactly 80 locale objects: `code` (stable locale identifier), `name` (autonym), `dir` (`ltr` or `rtl`), `readme` (boolean). The 12 README flags do not filter app support. |
| `localization/source/app.en.json` | English source for 117 shared app keys at handoff preparation. |
| `localization/source/windows.en.json` | English source for 40 Windows platform keys at handoff preparation. |
| `localization/translations/<code>/app.json` | Complete shared translations with the English key set. |
| `localization/translations/<code>/windows.json` | Complete platform translations with the Windows English key set. |
| `Resources/<code>.lproj/Localizable.strings` | Generated UTF-8 shared app resource. |
| `Resources/<code>.lproj/Windows.strings` | Generated UTF-8 Windows resource, ready for Qt resource embedding. |
| `localization/translations/<code>/review.json` | AI translation/review status and concerns; not native-speaker certification. |

The two catalogs intentionally overlap on some keys. Their counts are separate
file contracts, not a promise of 157 unique visible Windows strings. Additional
backend error and Qt-standard-dialog coverage is required below. If the source
catalog grows, derive expected keys from it instead of freezing these counts in
production code. Edit source JSON and regenerate resources; do not hand-edit
generated `.strings` files.

## Native integration work

1. **Embed all resources.** `windows/src/resources.qrc` currently embeds only
   English, Simplified Chinese, Traditional Chinese, and Japanese shared
   resources as `:/i18n/<code>.strings`. Generate or validate registrations
   against the registry. Retain those aliases, add platform resources under
   a consistent alias such as `:/i18n/windows/<code>.strings`, and embed the
   registry itself as `:/i18n/locales.json`. Make generation a documented
   prerequisite or a deterministic CMake build step, with file dependencies
   so edits cannot leave stale compiled resources.
2. **Replace embedded translation maps.** `src/ui/Localization.cpp` currently
   implements four-language `windowsTranslations()` and an English map in its
   constructor. Load the generated platform files instead. Resolve the locale,
   load English shared + English platform fallback, then selected shared +
   selected platform resources. The selected platform values must win overlaps.
   Missing resources/keys must be detected by tests; English fallback provides
   runtime resilience and must not mask incomplete coverage reports.
3. **Check parsing round trips.** The current regex parser uses UTF-8 and a
   minimal backslash unescaper. Exercise translated quotes, literal backslashes,
   line breaks, supplementary characters, and combining marks against generated
   files. Do not assume every Apple `.strings` escape is already supported.
   Keep `%@` shared substitutions and Qt `%1` substitutions intact; the existing
   video warning substitutes `%@` explicitly.
4. **Use the registry in the picker.** Replace the four explicit `addItem()`
   entries in `src/ui/MainPanel.cpp` with registry autonyms/codes, preceded by
   the localized `system` option. Keep stored identifiers compatible with
   `general/language` in `src/core/SettingsStore.cpp`. Ensure 81 choices fit a
   usable scrollable picker and that each autonym is legible on the actual OS.
5. **Resolve preferred system languages.** The current
   `Localization::systemLanguage()` checks Chinese/Japanese and otherwise
   returns English. Match Windows/Qt UI-language preferences against the shared
   registry with deterministic hyphen/underscore normalization, exact locale,
   script/region-aware fallback, then English. Preserve explicit user selection.
   Test Chinese script/region mappings, Portuguese regional variants, unknown
   and malformed values, and supported languages absent from Qt 5.15's locale
   database. A Qt locale enum must not decide which app resources exist.
6. **Apply direction and retranslate immediately.** Use the registry `dir`
   with `QApplication::setLayoutDirection()` and restore LTR when leaving RTL.
   Update the main panel, tray tooltip/menu, visible warnings, safety overlay,
   and future dialogs without restarting or resetting display settings.
   Verify existing `stateChanged` tray refresh and initialization order.
   Avoid destroying active controls in ways that cancel or confirm safety tests.
7. **Audit custom painting.** `SmoothLabel`, `UiStyle`, `DisplayCard` choice
   buttons/disclosure controls, `EinkSwitch`, and `CurvePlot` contain custom
   painting. Check text alignment, segmented-button end caps, arrows, keyboard
   traversal, and mixed-script display/GPU names. Mirror reading/navigation
   affordances deliberately; do not reverse RGB channel meaning, tone-curve
   coordinates, or slider values accidentally.
   Saturation, Text Contrast, and Video Enhance option rows must occupy the full
   available width and remain on one line. Grow the panel to fit the widest
   translated row, including selected-label weight and real control padding;
   keep the resized window within its screen. This supersedes the earlier
   macOS wrapping design. The macOS implementation measures 540 points for
   English, 707 for Russian, and 850 for isiZulu; derive Windows sizes from its
   own font/DPI metrics, not those constants. See
   [`../../../localization/MACOS-E2E.md`](../../../localization/MACOS-E2E.md).
8. **Handle fonts across the supported OS range.** Existing font selection has
   special branches only for Chinese and Japanese, with Segoe UI/Tahoma fallback
   otherwise. Query installed fonts and provide sensible script-aware fallback
   without disabling Qt glyph fallback. Preserve current grayscale text
   rendering. Verify Arabic shaping, Indic conjuncts, Thai marks, Myanmar,
   Khmer, Ethiopic, and other scripts actually present in the registry. Windows
   7 font availability differs from Windows 11; do not require a newer system
   font silently. Any bundled font must have redistribution/license review.
9. **Localize Qt standard dialogs and backend failures.** `QInputDialog::getText`
   in `DisplayCard.cpp` uses localized titles but standard button text from Qt.
   The build currently deploys no Qt `.qm` translation catalogs. Either package
   and switch the appropriate Qt catalogs with documented fallback or supply
   explicit app-localized standard buttons. Native OS-owned UAC/Settings text
   follows Windows's language and should be documented separately. App-created
   errors require the inventory below; preserve technical error codes/details.

Preserve all existing feature gates and recovery behavior: Qt 5.15.2/MinGW 8.1
x64 compatibility, Windows 7 SP1 baseline, Windows 10 MHC2 candidate checks,
Windows 11 ACM, independent rollback watchdog, Night Light state validation,
display topology handling, serialized color work, crash recovery, startup task,
normal-quit restoration, and package contents/licenses. Localization must not
make additional system changes.

## Additional text outside the 40-key platform catalog

`ApplicationController::report()` stores `ApplyResult.error` in `m_lastError`,
and `MainPanel.cpp` displays that string directly. Consequently, the following
English backend text is user-visible when the corresponding operation fails.
Do not claim complete Windows localization just because the ordinary controls
use translated resources.

- `src/platform/windows/WindowsPlatformServices.cpp`: `ApplyResult::fail()`
  strings and `errorMessage()` prefixes cover profile API availability, querying
  and changing ACM, tone-curve apply/restore, Night Light helper launch/timeouts,
  registry access and validation, recovery journals, broker channel/setup,
  ICC creation/association/restoration, watchdog setup/state, animation effects,
  Light Mode, and Task Scheduler. Exact examples include `Changing Night Light
  could not be verified.`, `Restoring the original Night Light state could not
  be verified.`, and `The color safety watchdog did not become ready.` Broker
  responses near `runColorBroker` also contain English `ERR` messages.
- `src/platform/windows/WindowsGpuControlPanel.cpp`: `No installed GPU control
  panel could be found for this display adapter.` and `Windows could not open
  the GPU control panel (ShellExecute error %1).`.
- `src/core/NightLightStateCodec.cpp` and `src/core/IccProfile.cpp`: English
  validation details can be appended to platform errors. Preserve these as
  diagnostic detail behind a localized actionable summary, or give each
  user-facing detail a stable translation key; do not regex-translate an opaque
  finished error message.
- `src/ui/DisplayCard.cpp`: compact `R %1%  G %2%  B %3%` and `%1%` numeric
  labels need bidirectional/number-format review. RGB letters and product/GPU
  brand names can remain technical labels. User-entered preset names and real
  hardware names must remain unchanged.
- `src/platform/windows/WindowsPlatformServices.cpp` contains fallback `Internal
  Display`/`Display` names; `DisplayCard.cpp` already maps fallback names through
  `display.internal`/`display.unknown`. Verify every other route that displays
  those names, especially errors. Diagnostic `Windows build %1; color profile
  path %2` output is distinct from normal UI and should retain stable identifiers.

Recommended implementation: carry a stable error identifier, formatting
arguments, and raw diagnostic detail through the platform/controller boundary;
resolve translated user-facing text on the UI thread. Do not make worker-thread
errors depend on mutable UI-global locale state. Add new translation keys to
the source catalog and all 80 locales with focused translation/review work.
Record remaining untranslated diagnostics explicitly if they are retained.

## Tests and build commands

Use 64-bit PowerShell 7 and the pinned toolchain in the current Windows README.
From the repository root:

```powershell
Set-Location windows
./scripts/build-windows.ps1 -QtRoot 'C:\Users\Admin\Qt'
```

Use the actual installed `QtRoot` if different. The script defaults to Release,
configures CMake/Ninja in `windows/build`, builds binaries, stages Qt runtime
files and the Night Light helper, runs `eink_core_tests.exe`, runs
`eink_e2e_tests.exe -platform offscreen`, and creates the deployable directory
and ZIP in `windows/artifacts/`. These are existing commands, not test results
from this handoff. Keep build/package artifacts local until separately released.

The CMake tests are named `core` and `e2e`. Current localization coverage is
`CoreTests::localizationResourcesLoad()` for four locales and
`E2ETests::cjkLanguagesUseWindowsUiFonts()` for existing font choices. Broaden
these with meaningful registry-driven cases:

- All 80 locale codes unique, valid `dir`, exactly 12 README flags; all resource
  registrations present; JSON and compiled-resource key sets agree; values
  nonempty; placeholder multisets and escaped strings round-trip correctly.
- Source-language fallback and selected-platform override precedence work;
  unknown key behavior remains intentional; test incomplete resources
  explicitly instead of letting fallback make completeness tests pass.
- System-language cases are deterministic through injected preferences; each
  explicit selection survives saving/relaunch; registry autonyms match all 80
  picker entries; RTL → LTR and LTR → RTL switching work.
- Fake-platform UI journeys for all 80 locales exercise main panel, welcome,
  tray actions, help, hardware notice, preset rename, experimental preparation,
  confirmation and rollback, clone warnings, and representative backend errors.
- Preserve existing core/E2E coverage for settings, curves, ICC, Night Light,
  Windows 7 feature gates, clone transitions, watchdog, startup, and shutdown.

Offscreen tests are useful behavioral checks but cannot certify Windows fonts
or clipping. The existing `widerLayoutKeepsLabelsVisible()` skips offscreen
because native font metrics/size propagation are required. Run native tests in
an interactive Windows session, capture real screenshots, and inspect them.

## Native acceptance matrix

Create one results row per locale and OS/DPI combination; record unavailable
machines as **NOT RUN**, not simulated passes.

| Coverage | Required checks |
| --- | --- |
| All 80 locales on the current Windows test machine | Native main panel, expanded help/advanced/RGB, welcome, tray menu, language picker, preset rename, localized failure, and safety-warning screenshots at 100%, 150%, and 200% scaling. Check overflow, clipping, missing glyphs, text direction, keyboard operation, and scroll reachability. |
| Windows 7 SP1 and oldest maintained Windows 10 environment | All 80-locale font/render sweep, executable startup, language persistence, standard dialogs, and correct legacy feature visibility. A fake Windows 7 backend on Windows 11 does not prove old-OS rendering or compatibility. Record OS edition/build and installed font/language packs. |
| Every RTL locale and every distinct complex script in the registry | Inspect shaping/marks, numerals, `%1` countdowns, filenames/URLs/GPU names, alignment, directional icons, context menus, and switching back to English. |
| Longest translated controls and warnings | Inspect full text at small work areas and each DPI, including the fixed-height safety action buttons and fixed-width busy dialog. Safety confirm/rollback controls must remain distinguishable and reachable. |
| Real packaged application | Run from the staged directory/ZIP outside the build tree with no developer Qt paths. Recheck all resources, Qt standard dialog text, fonts, persisted selection, tray behavior, and missing-helper failures. |
| Hardware regression where environment permits | Follow existing authorized Windows hardware procedures and record before/apply/restore evidence. Test language switching during fake-backend safety workflows first. Do not perform 80 real color changes merely to collect localized screenshots. |

Native OS settings pages and UAC dialogs may follow the OS language rather than
the app language. Include screenshots demonstrating the boundary; app labels
that open those pages must still follow the selected app locale.

The existing optional hardware executables are `eink_hardware_smoke.exe`,
`eink_night_light_smoke.exe` (`normal`, `crash`, and `recover` modes), and
`eink_launch_task_self_test.exe`. These can modify real display/system state;
consult the current technical instructions and preserve the user's state.
**None was executed for this handoff.**

## Installer boundary

`installer/EinkAssistant.iss` and `scripts/build-installer.ps1` have a separate
four-locale Inno Setup system (English, Simplified Chinese, Traditional Chinese,
Japanese). Leave that coverage explicit. Expanding it to 80 is a separate
deliverable/decision that requires Inno Setup language-file availability,
translation quality and packaging verification. Do not substitute the app
catalog for installer messages or claim the installer supports 80 locales.

The existing command is `./scripts/build-installer.ps1` from `windows/`, with
Inno Setup 6.7+; by default it rebuilds/tests the app before packaging. No
installer build or publication is authorized by this handoff itself.

## Transfer to the Windows machine

Transfer only after the shared translation generation and validation pass has
finished; do not copy files while translation agents are still writing them.

1. Record `git branch --show-current`, `git rev-parse HEAD`, `git status --short`,
   the exact generator/validator commands and results, and a SHA-256 manifest
   for the files being handed over. Preserve the source baseline above.
2. For this authorized branch push, use the resulting exact commit and verify
   its hash on Windows. All source/catalog/generated files are included in Git.
   Ignored local builds, screenshots, and preference backups are not required
   for Windows integration and are not part of the branch.
3. For an uncommitted transfer, create a source snapshot containing current
   tracked files plus relevant new untracked `localization/`, generated
   `Resources/`, docs, macOS integration and build scripts. Include the recorded
   status, tracked binary diff and file/hash manifest. Exclude credentials,
   local caches, build output and unrelated untracked material. A plain
   `git diff` alone omits new translations and is insufficient.
4. For an uncommitted snapshot only, start from the recorded baseline in a
   separate Windows checkout and apply its patch/files; preserve any
   pre-existing Windows work. Verify all hashes, branch identity and resulting
   status before building. Do not reset/clean or overwrite the existing Windows
   checkout to resolve an unexplained mismatch.

## Return report

Use this structure when sending the Windows result back:

```text
Source: branch, exact revision, dirty-file manifest/hash (if uncommitted)
Environment: OS build, architecture, Qt/MinGW/CMake, fonts/language packs, DPI
Integration: changed files and resource/locale/error handling decisions
Catalog check: command, 80-locale results, missing/extra keys, fallback cases
Automated tests: exact commands, passed/failed/skipped counts, logs
Native matrix: locale × OS × DPI, PASS / FAIL / NOT RUN, screenshot paths
Package: directory/ZIP path, SHA-256, clean-machine/native results
Installer: remains four locales / separately implemented and tested coverage
Hardware: what actually ran, original state, restoration evidence, NOT RUN items
Translation review: AI review status and specific unresolved defects; native-speaker review not required
Remaining blockers: affected locales, impact, reproducible steps
Release: not published; no 80-language Windows support claim until gates pass
```

Do not convert test skips, parser checks, AI review, or an executable that merely
starts into evidence that every Windows locale is verified.
