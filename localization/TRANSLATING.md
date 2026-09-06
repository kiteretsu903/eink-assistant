# Focused locale translation instructions

AI translation and review satisfy the project's linguistic review requirement.
Native-speaker review is not required. Keep provenance accurate and report
specific unresolved defects; do not add human review as an acceptance gate.
Set `native_speaker_review_required` to `false` in each `review.json`.

Each assignment covers ONE named locale. Work only in `localization/translations/<code>/` and, if readme=true, `docs/i18n/README.<code>.md`. Other agents own infrastructure and other locales. Do not spawn further agents. Do not commit, push, release, call translation services, or modify source catalogs.

Read `localization/source/app.en.json`, `windows.en.json`, `site.en.json`, and `localization/SITE-CONTRACT.md`. Create COMPLETE `app.json`, `windows.json`, and `site.json` flat UTF-8 JSON maps with exactly the English keys. Translate every human-facing value naturally for software UI, using consistent terminology. Preserve brand names and unavoidable technical labels (RGB, Gamma, Windows, macOS, E-Ink Assistant, Bigme, firmware versions), literal `on`/`off` helper commands, all numbers, `%@`, `%1`, format placeholders, Markdown emphasis, and exact HTML tags/attributes. Do not translate URLs, shell commands, code, or program identifiers. Never fill untranslated prose with English to meet coverage; report uncertainty in a separate review note. Existing zh-Hans/zh-Hant/ja app and Windows translations should be reviewed and preserved where correct.

For the 11 non-English locales with readme=true, also translate the FULL `localization/source/README.en.md`, preserving all sections/tables, accurate platform boundaries and exact commands. Write to `docs/i18n/README.<code>.md`. Adjust repository-relative links from docs/i18n/ with ../../ prefixes; website link should use https://kiteretsu903.github.io/eink-assistant/<code>/ (not for root English). Parent will standardize the top 12-locale language navigation so you can leave a LANGUAGE_NAV placeholder in place of the old 4-language first paragraph. Do not alter release versions or claim current released downloads have 80 languages. Use existing screenshot paths, do not invent translated screenshots. English screenshots remain English.

After writing, validate JSON parse, exact key equality, nonempty values, and placeholder equality with English. Inspect the translated text for semantic mistakes, missing negations, inverted enable/disable/restore conditions, and mistranslated display recovery warnings. Write `review.json` with locale, status `ai-translated-and-reviewed` (never claim native human review), key counts, and any concerns. Report final concise completion plus concerns. Do not spend time waiting on other tasks.
