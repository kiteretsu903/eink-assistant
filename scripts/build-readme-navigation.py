#!/usr/bin/env python3
"""Keep exactly 12 README language links consistent without rewriting prose."""
import argparse
import html
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
START = '<!-- BEGIN README LANGUAGES -->'
END = '<!-- END README LANGUAGES -->'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--allow-incomplete', action='store_true')
    args = parser.parse_args()
    entries = [item for item in json.loads((ROOT/'localization/locales.json').read_text()) if item['readme']]
    assert len(entries) == 12
    errors = []
    for current in entries:
        code = current['code']
        path = ROOT / ('README.md' if code == 'en' else f'docs/i18n/README.{code}.md')
        if not path.exists():
            if not args.allow_incomplete: errors.append(f'Missing {path.relative_to(ROOT)}')
            continue
        parts = []
        for item in entries:
            name = html.escape(item['name'])
            if item['code'] == code:
                parts.append(f'<b lang="{code}" dir="{item["dir"]}">{name}</b>')
            else:
                target = ('../../README.md' if code != 'en' else 'README.md') if item['code'] == 'en' else (('docs/i18n/' if code == 'en' else '') + f'README.{item["code"]}.md')
                parts.append(f'<a href="{target}" lang="{item["code"]}" dir="{item["dir"]}">{name}</a>')
        nav = START+'\n<p align="center">\n  '+' &nbsp;·&nbsp;\n  '.join(parts)+'\n</p>\n'+END
        original = path.read_text()
        if START in original:
            updated = re.sub(re.escape(START)+r'.*?'+re.escape(END), lambda _:nav, original, count=1, flags=re.S)
        elif 'LANGUAGE_NAV' in original:
            updated = re.sub(r'(?:<!--\s*)?LANGUAGE_NAV(?:\s*-->)?', lambda _:nav, original, count=1)
        else:
            match = re.search(r'<p align="center">.*?</p>', original, flags=re.S)
            if not match or ('README.' not in match[0] and '<b>English</b>' not in match[0]):
                errors.append(f'Cannot identify navigation in {path.relative_to(ROOT)}')
                continue
            updated = original[:match.start()] + nav + original[match.end():]
        if args.check:
            if updated != original: errors.append(f'Stale navigation: {path.relative_to(ROOT)}')
        else:
            path.write_text(updated)
    if errors:
        raise SystemExit('\n'.join(errors))
    print('README navigation validated for 12 locales.')

if __name__ == '__main__': main()
