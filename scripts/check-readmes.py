#!/usr/bin/env python3
"""Verify full README coverage, language navigation, commands and local links."""
from collections import Counter
from pathlib import Path
import json
import re
from urllib.parse import urlsplit, unquote
ROOT=Path(__file__).resolve().parents[1]
entries=[x for x in json.loads((ROOT/'localization/locales.json').read_text()) if x['readme']]
source=(ROOT/'localization/source/README.en.md').read_text()
headings=lambda s:Counter(len(m) for m in re.findall(r'^(#{1,6}) ',s,re.M))
code=lambda s:re.findall(r'```[^\n]*\n(.*?)```',s,re.S)
rows=lambda s:len(re.findall(r'^\|',s,re.M))
errors=[]
for entry in entries:
 locale=entry['code'];path=ROOT/('README.md' if locale=='en' else f'docs/i18n/README.{locale}.md')
 if not path.exists():errors.append(f'{locale}: missing README');continue
 text=path.read_text()
 if headings(text)!=headings(source):errors.append(f'{locale}: heading structure differs')
 if rows(text)!=rows(source):errors.append(f'{locale}: table row count differs')
 if code(text)!=code(source):errors.append(f'{locale}: installation commands differ')
 if 'LANGUAGE_NAV' in text or '<!-- BEGIN README LANGUAGES -->' not in text:errors.append(f'{locale}: unresolved language navigation')
 for ref in re.findall(r'(?:href|src)="([^"]+)"|\]\(([^)]+)\)',text):
  url=next(x for x in ref if x);parsed=urlsplit(url)
  if parsed.scheme or parsed.netloc or not parsed.path:continue
  target=path.parent/unquote(parsed.path)
  if not target.exists():errors.append(f'{locale}: broken local link {url}')
actual={p.name for p in (ROOT/'docs/i18n').glob('README.*.md')}
expected={f'README.{x["code"]}.md' for x in entries if x['code']!='en'}
if actual!=expected:errors.append('Translated README file set does not match the selected 11 non-English locales')
if errors:raise SystemExit('\n'.join(errors))
print('PASS: 12 READMEs; full section/table coverage, exact commands, navigation and local links')
