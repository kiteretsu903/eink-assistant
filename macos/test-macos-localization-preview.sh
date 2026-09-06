#!/bin/bash
# Build an isolated, safe preview, not the production app. No installer/signing.
set -euo pipefail
cd "$(dirname "$0")/.."
output="${TMPDIR:-/tmp}/eink-localization-preview"
mkdir -p "$output/Localization Preview.app/Contents/MacOS" "$output/Localization Preview.app/Contents/Resources"
swiftc -parse-as-library -D LOCALIZATION_PREVIEW -module-cache-path "$output/module-cache" \
  -target arm64-apple-macos14.0 macos/Sources/Shared/*.swift macos/Sources/EinkAssistant/*.swift \
  macos/Sources/LocalizationPreview/*.swift -o "$output/Localization Preview.app/Contents/MacOS/Preview"
cp -R Resources/*.lproj "$output/Localization Preview.app/Contents/Resources/"
cp localization/locales.json "$output/Localization Preview.app/Contents/Resources/"
cat > "$output/Localization Preview.app/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?><!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd"><plist version="1.0"><dict><key>CFBundleIdentifier</key><string>local.eink.localization-preview</string><key>CFBundleExecutable</key><string>Preview</string><key>CFBundleName</key><string>Localization Preview</string><key>CFBundleDevelopmentRegion</key><string>en</string><key>CFBundleShortVersionString</key><string>preview</string></dict></plist>
PLIST
"$output/Localization Preview.app/Contents/MacOS/Preview" "$@"
