#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
test_output="$(mktemp -d "${TMPDIR:-/tmp}/eink-localization-tests.XXXXXX")"
trap 'rm -rf "$test_output"' EXIT
mkdir -p "$test_output/Tests.app/Contents/MacOS" "$test_output/Tests.app/Contents/Resources"
swiftc -parse-as-library -module-cache-path "${TMPDIR:-/tmp}/eink-localization-module-cache" \
  macos/Sources/Shared/Localization.swift macos/test-macos-localization.swift \
  -o "$test_output/Tests.app/Contents/MacOS/Tests"
cp localization/locales.json "$test_output/Tests.app/Contents/Resources/"
cp -R Resources/*.lproj "$test_output/Tests.app/Contents/Resources/"
"$test_output/Tests.app/Contents/MacOS/Tests" "$@"
