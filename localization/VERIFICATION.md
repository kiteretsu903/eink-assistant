# Multilingual verification — 2026-09-06

Verified locally on `dev/multi-lingual` before the user-authorized branch commit
and push for Windows handoff. No download, release, or website deployment was
published by this work.

## Completed checks

Subsequent [live macOS E2E testing](MACOS-E2E.md) passed all 80 native language
switches, representative interactions, restart persistence, and restoration of
the original app preferences. This extends the preview-only checks below.

| Area | Result |
|---|---|
| Catalogs | 80/80 locales; 117 shared app, 40 Windows, and 203 website keys per locale. Exact keys, placeholders, HTML structure/links, and numeric values validated. |
| Generated output | Native resources, static website, and README navigation pass deterministic regeneration checks. All 160 Apple `.strings` files pass `plutil`. |
| Website | 160 static pages pass route, local link/anchor, metadata, direction, accessibility-label, language-selector, and sitemap validation. |
| Browser layout | All 160 pages checked at measured CSS viewport widths 358, 389, and 1280 pixels. No page overflow or checked text clipping; locale routing correct. Mobile checks also verified image loading and text direction. |
| Browser interactions | Arabic macOS/Windows download dialogs open and close, focus returns to the trigger, and the expanded shell command remains exact and LTR. Language switching preserves changelog platform/query/fragment; platform tabs update. |
| README | English plus 11 complete translations; section/table coverage, installation commands, navigation, and local links pass. |
| Regression tests | 10 Python contract tests; JavaScript locale resolution/navigation and blocked-storage preference regressions pass. |
| macOS runtime contract | 80-locale registry/resources, legacy saved IDs, region/script matching, RTL, English fallback, and isolated preview preferences pass. |
| macOS build | `./build.sh` passes for E-Ink Assistant, ToneLab, and ReadingLab. Each bundle contains 80 locale directories and 80 `CFBundleLocalizations` entries; local ad-hoc signatures pass strict verification. |
| macOS font coverage | 9,440 shared-app strings/autonyms checked across all 80 locales; 26 resolved fonts; no missing glyph or LastResort fallback issues on this Mac. |
| macOS rendering | 240 standard snapshots (control, welcome, helper views for 80 locales) and 80 advanced-control snapshots generated from production views using fictional display data. Representative RTL, long-text, and complex-script images visually inspected. |

Native visual review corrected a two-line hardware reminder limit and a truncated
advanced-mode explanation. Browser review corrected long Tamil/Malayalam text
overflow. CSS/JS URLs now include content hashes to avoid stale cached assets.

The complete native previews, font report, and results with source hashes are
retained locally in ignored `artifacts/localization-qa/`. These artifacts are not
included by Git unless explicitly added. Preview commands are documented in
[macOS localization](../macos/LOCALIZATION.md).

## Remaining acceptance boundaries

- Windows native resource registration, integration, build, GUI, fonts/DPI,
  hardware behavior, packaging, and installer tests are **NOT RUN**. Continue
  with the [Windows handoff](../windows/docs/handoffs/multilingual-80.md).
- Native previews use fictional data and disabled controls. They do not establish
  real hardware behavior or interactive acceptance for every locale. Font results
  describe this Mac, not every supported operating-system version.
- Translation subagents worked on one locale per assignment. All 79 non-English
  locales include AI review notes. AI review satisfies the linguistic acceptance
  requirement; native-speaker review is not required, per the user's decision.
- Production Pages deployment, crawler/indexing behavior, signing/notarization,
  and release installation were not performed. Current downloadable releases
  retain their original four interface languages.
