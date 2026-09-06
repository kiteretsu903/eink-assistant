# macOS localization

The app loads `locales.json` from its bundle. The source of truth is
`localization/locales.json`; `macos/build.sh` copies it and generates
`CFBundleLocalizations` from its locale codes. Each code maps directly to a
`Resources/<code>.lproj/Localizable.strings` directory. The existing saved values
`system`, `en`, `zh-Hans`, `zh-Hant`, and `ja` retain their meaning.

System mode uses the primary preferred language only. Exact locale and script
matches take precedence, supported base languages handle regional variants,
Chinese region/script variants resolve to Hans or Hant, and unsupported languages
or unsupported explicit scripts fall back to English. If only regional locales
exist, registry order supplies the default (Portuguese resolves to pt-BR).
Norwegian `no`, Tagalog `tl`, and legacy Hebrew/Indonesian codes resolve to their
supported equivalents. Every missing translation key falls back to English.

Switching languages refreshes the model, welcome view, tray accessibility label,
error messages, locale environment and right-to-left layout immediately.
Autonyms remain recognizable in the language menu. Numeric sliders and the tone
curve stay left-to-right deliberately, while surrounding labels and controls
follow the selected locale. Preset options stay on one row and fill the available width. The panel grows
from its 540-point baseline to fit the widest translated option row, including
bold labels and button padding; switching languages also repositions the bubble
within the screen. Prose grows vertically and the main panel scrolls.

## Validation

[Live macOS E2E results](../localization/MACOS-E2E.md) cover actual native language
switching across all 80 locales, representative controls, restart persistence,
and restoration of the original preferences using Computer Use.

Run `macos/test-macos-localization.sh` to check the 80 registry entries, preserved
IDs, region/script matching, RTL, missing-key fallback and preference isolation.
Add `--resources` after generating all translations to assert all resource
bundles are present. The cross-platform generator performs full string/key and
placeholder validation.

Run `macos/test-macos-localization-fonts.sh /tmp/eink-localization-fonts.json`
to shape every app string and locale autonym through CoreText's regular system
UI font cascade. It reports missing glyphs and LastResort font runs, ignoring
legitimate whitespace, controls, joiners and variation selectors. The JSON also
lists the actual fallback fonts used. This checks the current host's font
coverage, not every supported macOS version or every font weight.

Production compile check (does not launch or package the app):

```sh
swiftc -typecheck -parse-as-library -module-cache-path /tmp/eink-multilingual-swift-cache -target arm64-apple-macos14.0 macos/Sources/Shared/*.swift macos/Sources/EinkAssistant/*.swift
```

## Safe visual preview

`macos/test-macos-localization-preview.sh --locale ar` builds a separate app under
TMPDIR, with a separate bundle ID and an in-memory language preference. It uses a
fictional display model, skips display discovery, notification registration,
helper migration, login-item registration, gamma/profile changes and quit cleanup.
The real AssistantDelegate and app entry point are never instantiated. All
production controls are disabled; only the preview language selector is active.
The installed running app is untouched. Close the preview process when finished.

For offscreen screenshots of the production control view, welcome view and
accessibility installation guide:

```sh
macos/test-macos-localization-preview.sh --locale ar --snapshot /tmp/eink-localization-screenshots
macos/test-macos-localization-preview.sh --all --snapshot /tmp/eink-localization-screenshots
macos/test-macos-localization-preview.sh --locale ar --advanced --snapshot /tmp/eink-localization-advanced
```

Snapshots use a white background and light appearance for reproducible layout
inspection. `--advanced` captures only the app view with curve controls expanded;
the standard run already captures the unchanged welcome and helper views.
The renderer settles the SwiftUI layout before caching its bitmap so first-frame
font/layout readiness does not appear as missing outlines or overflow.
macOS window-server access is required even for offscreen rendering.
If restricted shell execution aborts in `_RegisterApplication`, rerun the same
isolated preview with appropriate window-service permissions; do not launch the
production app as a workaround. These mock screenshots verify rendering, not
real display-control behavior or native-speaker translation quality.

## Verification on 2026-09-06

The strict resource/resolution test passed with all 80 resource bundles. CoreText
checked 9,440 strings (117 per locale plus 80 autonyms), used 26 resolved fonts,
and reported zero missing-glyph or LastResort issues on this host.

The final isolated preview runs exited successfully and produced 240 standard
images in `/tmp/eink-localization-all` and 80 advanced app images in
`/tmp/eink-localization-all-advanced-final`. These counts are rendering coverage,
not a claim that every screenshot received manual visual inspection.

Representative visual checks covered standard app views in English, Arabic,
German, Hindi, Tamil, Malayalam, Khmer, Burmese, Persian, Urdu, Hebrew, Lao,
Amharic and Armenian; welcome/helper views included Tamil and Urdu, plus the
tallest Armenian app, Kazakh welcome and Russian helper examples. Final advanced
views were inspected for Tamil, Malayalam, Khmer, Lao, Burmese, Amharic, Urdu and
Armenian. Reviewed views showed wrapped controls and prose, RTL placement, and
left-to-right numeric sliders/curves without visible clipping. This inspection
found and corrected the Tamil hardware reminder's two-line truncation and the
Malayalam advanced note's compressed height, then verified both final renders.

The previews use disabled controls and a fictional display. They do not exercise
live display adjustments, every menu or popup interaction, other OS versions, or
native-speaker linguistic review (which is not required for acceptance).
No connected display settings or production
preferences were changed by this verification.
