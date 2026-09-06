# Localization

The `dev/multi-lingual` branch expands the macOS application and product website
to 80 locales and keeps the main README focused on 12. Windows receives the same
shared strings plus a complete catalog of its current localization-layer text;
native Windows integration and acceptance are a separate handoff.

The downloadable macOS 2.6 and Windows 1.2 releases still have their original
four interface languages. This branch does not publish or replace those files.
The website comparison describes the current downloads explicitly.

## Sources and generated files

| Content | Editable source | Generated or translated output |
|---|---|---|
| Locale codes, native names, direction, README membership | `locales.json` | `Resources/locales.json`, `docs/locales.json` |
| Shared app UI | `source/app.en.json`, `translations/<locale>/app.json` | `Resources/<locale>.lproj/Localizable.strings` |
| Windows-specific localization layer | `source/windows.en.json`, `translations/<locale>/windows.json` | `Resources/<locale>.lproj/Windows.strings` |
| Product website, both changelogs, dialogs, metadata, accessibility and runtime labels | `source/site.en.json`, `translations/<locale>/site.json`, `site-templates/` | `docs/index.html`, `docs/changelog.html`, and `docs/<locale>/` |
| README prose | `source/README.en.md` (translation baseline), `README.md`, `docs/i18n/README.<locale>.md` | Language navigation generated from the registry |

Paths in this table are relative to this directory for sources and the repository
root for generated output. English website routes remain at the root. Other
languages have static HTML at their own locale paths, with complete content
available without JavaScript. Existing `?lang=` links are redirected to these
paths. The selector preserves the current page, platform query and fragment.
A direct locale path takes priority over browser/saved preferences.

Each non-English locale has `review.json` recording AI translation/review and
translation-time observations. AI review satisfies the project's linguistic
review requirement; native-speaker review is not required. This policy supersedes
human-review suggestions in the original translator notes. It does not imply
that human review occurred or waive functional, layout, or platform checks.
The four existing app translations were preserved where appropriate. Regional
and script variants are counted separately: Chinese has Simplified/Traditional
resources; Portuguese has Brazil/Portugal resources; Serbian uses Cyrillic,
Punjabi Gurmukhi, Azerbaijani/Uzbek Latin, and Kazakh/Mongolian Cyrillic.

## Build and validate

Run from the repository root with Python 3 and Node.js:

```sh
python3 scripts/build-localizations.py
python3 scripts/build-localized-site.py
python3 scripts/build-readme-navigation.py
python3 scripts/build-localizations.py --check
python3 scripts/build-localized-site.py --check
python3 scripts/build-readme-navigation.py --check
python3 scripts/check-localized-site.py
python3 scripts/check-readmes.py
python3 -m unittest discover -s tests -v
node tests/test_site_locale.cjs
macos/test-macos-localization.sh --resources
```

The generators reject missing keys, empty translations, changed placeholders,
and changed HTML structure/attributes. Duplicate JSON keys are rejected by the
native-catalog validation. A missing translation must be completed; English
fallback is a runtime safeguard and does not satisfy catalog completeness.
Development-only subset flags are available while translations are in progress;
they are not release acceptance checks.

For a new English edit, update the relevant source and each affected translation,
review the meaning and placeholders, regenerate, and run the checks. Do not edit
the generated `.strings` or HTML files. Future README content changes must also
update the English translation baseline and the 11 translated READMEs.

## Verification boundaries

See [the completed verification report](VERIFICATION.md) for exact results,
local preview artifacts, and remaining acceptance work.

- [macOS preview and tests](../macos/LOCALIZATION.md) use production SwiftUI views
  with fictional display data. The preview never initializes real display
  discovery or calls the app's display-control lifecycle.
- Browser checks cover generated routes, metadata, direction, links, dialogs,
  and layout. The retained screenshots in website/README content are illustrative
  or English; their accessible descriptions do not pretend otherwise.
- [Windows handoff](../windows/docs/handoffs/multilingual-80.md) specifies native
  integration, remaining backend/Qt dialog text, font/script/DPI checks, tests,
  packaging and a results template. Windows build, runtime and installer
  acceptance are **NOT RUN on this Mac**. The installer retains four languages
  until its separate Windows work is completed.

Arabic, Hebrew, Persian and Urdu use right-to-left layout. Numeric controls,
RGB values and shell commands retain their intended left-to-right order.
