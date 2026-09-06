#!/usr/bin/env python3
"""Check generated locale routes, links, metadata, accessibility labels, and sitemap."""
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urlsplit, unquote
import argparse
import json
import xml.etree.ElementTree as ET
ROOT = Path(__file__).resolve().parent.parent
BASE = 'https://kiteretsu903.github.io/eink-assistant/'
class Page(HTMLParser):
    def __init__(self, text):
        super().__init__(); self.nodes=[]; self.feed(text)
    def handle_starttag(self, tag, attrs):
        self.nodes.append((tag,dict(attrs)))

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--locales', help='Comma-separated subset for development')
    args=ap.parse_args()
    locales=json.loads((ROOT/'localization/locales.json').read_text())
    selected=set(args.locales.split(',')) if args.locales else {x['code'] for x in locales}
    expected={x['code'] for x in locales} | {'x-default'}
    errors=[]; urls=[]
    for locale in locales:
        code=locale['code']
        if code not in selected: continue
        for filename in ('index.html','changelog.html'):
            path=ROOT/'docs'/('' if code=='en' else code)/filename
            if not path.exists(): errors.append(f'{path}: missing'); continue
            raw=path.read_text(); p=Page(raw)
            label=f'{code}/{filename}'
            def check(condition, message):
                if not condition: errors.append(f'{label}: {message}')
            root=next(a for t,a in p.nodes if t=='html')
            check(root.get('lang')==code and root.get('dir')==locale['dir'],'wrong language/direction')
            canonical=BASE+('' if code=='en' else code+'/')+('' if filename=='index.html' else filename)
            urls.append(canonical)
            check([a.get('href') for t,a in p.nodes if t=='link' and a.get('rel')=='canonical']==[canonical], 'wrong canonical')
            alts=[a for t,a in p.nodes if t=='link' and a.get('rel')=='alternate']
            check({a.get('hreflang') for a in alts}==expected and len(alts)==len(expected),'incomplete alternates')
            for alternate in alts:
                target_code = alternate.get('hreflang')
                expected_url = BASE + ('' if target_code in {'en','x-default'} else target_code + '/') + ('' if filename == 'index.html' else filename)
                check(alternate.get('href') == expected_url, 'incorrect alternate URL')
            options=[a for t,a in p.nodes if t=='option']
            check({a.get('value') for a in options}==expected-{'x-default'},'language picker incomplete')
            check([a.get('value') for a in options if 'selected' in a]==[code], 'wrong selected language')
            check('noindex' not in raw.lower() and '{{' not in raw, 'noindex or unresolved marker')
            for tag,attrs in p.nodes:
                if tag=='img':check('alt' in attrs, 'image has no alt attribute')
                if tag=='script': check(not any(x in attrs.get('src','') for x in ('i18n.js','platform-i18n.js')), 'legacy translation script remains')
                for attr in ('href','src'):
                    ref=attrs.get(attr,''); parsed=urlsplit(ref)
                    if not ref or parsed.scheme or parsed.netloc:continue
                    target=(path.parent/unquote(parsed.path)).resolve() if parsed.path else path
                    if target.is_dir(): target = target / 'index.html'
                    if parsed.path and not target.exists():errors.append(f'{label}: broken local {attr} {ref}')
                    if parsed.fragment and target.suffix=='.html' and target.exists():
                        ids={a.get('id') for _,a in Page(target.read_text()).nodes}
                        check(parsed.fragment in ids,f'missing anchor {ref}')
            if filename=='index.html':
                copy=next(a for _,a in p.nodes if 'data-copy-command' in a)
                check(all(copy.get(k) for k in ('data-copy-label','data-copied-label','data-copy-failed-label')),'missing clipboard messages')
    if not args.locales:
        tree=ET.parse(ROOT/'docs/sitemap.xml')
        actual=[n.text for n in tree.findall('.//{http://www.sitemaps.org/schemas/sitemap/0.9}loc')]
        if sorted(actual)!=sorted(urls):errors.append('Sitemap does not exactly cover all generated pages')
    if errors:raise SystemExit('\n'.join(errors))
    print(f'PASS: {len(urls)} pages; local links, anchors, language selectors, directions, metadata, accessibility labels'+(' and sitemap' if not args.locales else ''))
if __name__=='__main__':main()
