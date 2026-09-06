#!/usr/bin/env python3
"""Generate crawlable localized pages with no external Python dependencies."""
import argparse
import html
import hashlib
from html.parser import HTMLParser
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parent.parent
BASE = 'https://kiteretsu903.github.io/eink-assistant/'
TOKEN = re.compile(r'\{\{([AH]):([^}]+)\}\}')

class Markup(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.tags = []
    def handle_starttag(self, tag, attrs):
        self.tags.append((tag, tuple(sorted(attrs))))
    def handle_startendtag(self, tag, attrs):
        self.handle_starttag(tag, attrs)
    def handle_endtag(self, tag):
        if tag != 'br':
            self.tags.append(('/' + tag, ()))

def tags(value):
    parser = Markup()
    parser.feed(value)
    return parser.tags

def catalog(locale, source):
    if locale == 'en':
        return source
    path = ROOT / 'localization/translations' / locale / 'site.json'
    if not path.exists():
        raise ValueError(f'{locale}: missing {path.relative_to(ROOT)}')
    localized = json.loads(path.read_text())
    missing = source.keys() - localized.keys()
    extra = localized.keys() - source.keys()
    if missing or extra:
        raise ValueError(f'{locale}: missing keys {sorted(missing)}, unexpected keys {sorted(extra)}')
    for key, original in source.items():
        translated = localized[key]
        if not isinstance(translated, str) or not translated.strip():
            raise ValueError(f'{locale}/{key}: empty or non-string translation')
        if tags(original) != tags(translated):
            raise ValueError(f'{locale}/{key}: HTML tags or attributes changed')
        if '{{' in translated or '}}' in translated:
            raise ValueError(f'{locale}/{key}: unexpected template syntax')
    return localized

def page_url(code, page):
    return BASE + ('' if code == 'en' else code + '/') + ('' if page == 'index' else 'changelog.html')

def render(template, page, locale, locales, strings):
    code = locale['code']
    prefix = '' if code == 'en' else '../'
    result = TOKEN.sub(lambda match: html.escape(html.unescape(strings[match[2]]), quote=True) if match[1] == 'A' else strings[match[2]], template)
    options = '\n'.join(f'<option value="{item["code"]}" lang="{item["code"]}" dir="{item["dir"]}"' + (' selected' if item['code'] == code else '') + f'>{html.escape(item["name"])}</option>' for item in locales)
    seo = f'<link rel="canonical" href="{page_url(code, page)}">\n'
    seo += '\n'.join(f'<link rel="alternate" hreflang="{item["code"]}" href="{page_url(item["code"], page)}">' for item in locales)
    seo += f'\n<link rel="alternate" hreflang="x-default" href="{page_url("en", page)}">'
    for token, value in {'LOCALE':code, 'DIRECTION':locale['dir'], 'ASSET_PREFIX':prefix, 'LANGUAGE_OPTIONS':options, 'SEO_LINKS':seo}.items():
        result = result.replace('{{' + token + '}}', value)
    # Content hashes prevent stale CSS/JS after a locale-layout update.
    def version_asset(match):
        attribute, ref = match[1], match[2]
        path = ref.split('?')[0]
        if ':' not in path and path.endswith(('.css', '.js')):
            asset = ROOT / 'docs' / path
            if asset.exists():
                ref = path + '?v=' + hashlib.sha256(asset.read_bytes()).hexdigest()[:12]
        return attribute + ref + '"'
    result = re.sub(r'(\b(?:src|href)=")([^"]+)"', version_asset, result)
    # Local page links stay in the same locale. Shared static assets live at root.
    result = re.sub(r'(\bsrc=")([^":]+)(")', lambda m:m[1] + prefix + m[2] + m[3], result)
    result = re.sub(r'(\bhref=")([^":]+\.css(?:\?[^" ]*)?)(")', lambda m:m[1] + prefix + m[2] + m[3], result)
    result = re.sub(r'href="index\.html(?=["#])', 'href="./', result)
    labels = ' '.join(f'data-{attr}="{html.escape(strings[key], quote=True)}"' for attr, key in [('copy-label','runtime.copy'),('copied-label','runtime.copied'),('copy-failed-label','runtime.copyFailed')])
    result = result.replace('data-copy-command>', 'data-copy-command ' + labels + '>')
    if '{{' in result:
        raise ValueError(f'{code}/{page}: unresolved template token')
    return '\n'.join(line.rstrip() for line in result.splitlines()).rstrip() + '\n'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Validate catalogs and verify generated files are current without writing')
    parser.add_argument('--locales', help='Comma-separated subset for development; default is every locale')
    args = parser.parse_args()
    locales = json.loads((ROOT / 'localization/locales.json').read_text())
    source = json.loads((ROOT / 'localization/source/site.en.json').read_text())
    selected = set(args.locales.split(',')) if args.locales else {item['code'] for item in locales}
    unknown = selected - {item['code'] for item in locales}
    if unknown:
        raise ValueError(f'Unknown locales: {sorted(unknown)}')
    outputs = {}
    for locale in locales:
        if locale['code'] not in selected:
            continue
        strings = catalog(locale['code'], source)
        for page in ('index', 'changelog'):
            template = (ROOT / 'localization/site-templates' / f'{page}.html').read_text()
            directory = ROOT / 'docs' / ('' if locale['code'] == 'en' else locale['code'])
            outputs[directory / f'{page}.html'] = render(template, page, locale, locales, strings)
    # Generate registry and sitemap only on a full build, or if absent.
    if not args.locales:
        outputs[ROOT / 'docs/locales.json'] = json.dumps(locales, ensure_ascii=False, indent=2) + '\n'
        urls = [page_url(item['code'], page) for item in locales for page in ('index','changelog')]
        outputs[ROOT / 'docs/sitemap.xml'] = '<?xml version="1.0" encoding="UTF-8"?>\n<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n' + '\n'.join('  <url><loc>' + url + '</loc></url>' for url in urls) + '\n</urlset>\n'
    stale = []
    for path, value in outputs.items():
        if args.check:
            if not path.exists() or path.read_text() != value:
                stale.append(str(path.relative_to(ROOT)))
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(value)
    if stale:
        raise ValueError('Generated files differ: ' + ', '.join(stale))
    print(f'{"Verified" if args.check else "Generated"} {len(selected)} locales, {len(selected) * 2} pages; {len(source)} catalog keys per locale.')

if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, json.JSONDecodeError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
