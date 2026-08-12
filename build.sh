#!/bin/bash
# Builds E-Ink Assistant.app and the ToneLab tuning tool.
#
# -target is pinned deliberately: without it the SDK emits a deployment target
# newer than the running OS, and LaunchServices refuses to open the bundle with
# a misleading -10825.
set -euo pipefail
cd "$(dirname "$0")"

TARGET="arm64-apple-macos14.0"

bundle() {  # bundle <name> <bundle-id> <extra-plist>
  local name="$1" ident="$2" extra="${3:-}"
  local app="$name.app"
  rm -rf "$app"
  mkdir -p "$app/Contents/MacOS"
  mv "$name.bin" "$app/Contents/MacOS/$name"
  cat > "$app/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key>            <string>$name</string>
  <key>CFBundleDisplayName</key>     <string>$name</string>
  <key>CFBundleIdentifier</key>      <string>$ident</string>
  <key>CFBundleVersion</key>         <string>1.0</string>
  <key>CFBundleShortVersionString</key><string>1.0</string>
  <key>CFBundlePackageType</key>     <string>APPL</string>
  <key>CFBundleExecutable</key>      <string>$name</string>
  <key>CFBundleInfoDictionaryVersion</key><string>6.0</string>
  <key>LSMinimumSystemVersion</key>  <string>14.0</string>
$extra
</dict>
</plist>
PLIST
  # Ad-hoc signature so macOS will launch it locally, and so SMAppService
  # accepts it as a login item.
  codesign --force --sign - "$app" 2>/dev/null || \
    echo "  (codesign unavailable — the app will still run)"
}

echo "building E-Ink Assistant…"
swiftc -O -parse-as-library -target "$TARGET" \
  Sources/Shared/*.swift Sources/EinkAssistant/*.swift -o "E-Ink Assistant.bin"
# LSUIElement: menu bar only, no Dock icon.
bundle "E-Ink Assistant" "local.eink.Assistant" "  <key>LSUIElement</key>         <true/>"

echo "building ToneLab…"
swiftc -O -parse-as-library -target "$TARGET" \
  Sources/Shared/*.swift Sources/ToneLab/*.swift -o "ToneLab.bin"
bundle "ToneLab" "local.eink.ToneLab"

echo "building ReadingLab…"
swiftc -O -parse-as-library -target "$TARGET" \
  Sources/Shared/*.swift Sources/ReadingLab/*.swift -o "ReadingLab.bin"
bundle "ReadingLab" "local.eink.ReadingLab"

echo
echo "done:"
echo "  open 'E-Ink Assistant.app'"
echo "  open ToneLab.app        # shadow-lift (video) tuning"
echo "  open ReadingLab.app     # text-contrast (reading) tuning"
