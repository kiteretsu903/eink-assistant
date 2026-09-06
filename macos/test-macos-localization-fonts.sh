#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
font_test_output="$(mktemp -d "${TMPDIR:-/tmp}/eink-localization-fonts.XXXXXX")"
trap 'rm -rf "$font_test_output"' EXIT
swiftc -parse-as-library -module-cache-path "${TMPDIR:-/tmp}/eink-localization-module-cache" \
  macos/test-macos-localization-fonts.swift -o "$font_test_output/Fonts"
"$font_test_output/Fonts" "$PWD" "${1:-/tmp/eink-localization-fonts.json}"
