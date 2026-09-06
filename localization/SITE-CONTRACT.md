# Website translation contract

Translate `localization/source/site.en.json` into `localization/translations/<code>/site.json`. Each file is a flat JSON object with exactly the same 203 keys and string values. Preserve keys. This catalog covers the homepage, complete current macOS and Windows changelog, download dialogs, metadata, image descriptions, accessible labels, and clipboard feedback.

Values may contain inline HTML (`<strong>`, `<em>`, `<br>`, `<span aria-hidden="true">`, etc.). Preserve tags, attributes, URLs, technical commands, version numbers, percentages, and product names; translate the text around them. Keep valid JSON. Entries with `aria-label`, `alt`, `description`, and runtime keys are plain text. Keep screenshot descriptions honest: existing screenshots show the English app. Do not translate "English" into the name of your target language.

The current downloadable macOS 2.6 and Windows 1.2 builds have four interface languages. The comparison explicitly describes current downloads; do not claim 80 languages for those binaries. Historical release notes retain their historical language lists.

Generator: `python3 scripts/build-localized-site.py`. English stays at the site root. Other locales receive `docs/<code>/index.html` and `changelog.html`. Templates live in `localization/site-templates/`; never edit generated HTML. Locale names/direction come from `localization/locales.json`. No publication is authorized by generating pages.

## Generation and verification

Run `python3 scripts/build-localized-site.py` only after every locale catalog is present. It fails on missing/extra keys, blank values, changed HTML structure or attributes, and unresolved placeholders; it never silently fills incomplete pages with English. `--locales en,ar` permits a development subset. Full builds update `docs/locales.json` and the 160-URL sitemap.

Run `python3 scripts/build-localized-site.py --check` to detect stale generated files, then `python3 scripts/check-localized-site.py` for language directions, complete selectors, canonical/alternate URLs, local assets, anchors, image alternative text, clipboard labels, and sitemap coverage. Serve `docs` locally to inspect actual text and layout. Browser QA should cover home, both changelog tabs, both download dialogs, language switching, clipboard feedback, and long/complex/RTL scripts at desktop and narrow widths.

`docs/locale.js` preserves legacy `?lang=` links, query parameters such as `platform=windows`, and section anchors. It resolves explicit script before region for Chinese, common locale aliases, and unsupported scripts conservatively. Portuguese defaults to the first registry entry (Brazil). Explicit locale paths take precedence; unqualified English root paths use the saved language or the primary browser language. Native language-picker options use each locale's own language and direction. English navigation uses the canonical directory URL, not `index.html`.

`docs/i18n.css` is retained as shared script/RTL/responsive styling. The prior four-language `docs/i18n.js` and `docs/platform-i18n.js` runtime translation files were removed; localized content is now present before JavaScript runs. Existing English screenshots remain accurately labelled as English. Download URLs and historical releases are unchanged.

This branch has not been published. Production HTTP status, public sitemap serving, and search-index submission are deployment-time checks, not inferred from local generation.
