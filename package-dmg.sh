#!/bin/bash
# Build a standard drag-to-Applications release disk image with create-dmg.
# https://github.com/create-dmg/create-dmg (MIT)
set -euo pipefail
cd "$(dirname "$0")"

APP="E-Ink Assistant.app"
BACKGROUND="Resources/dmg-background.png"

if [ ! -d "$APP" ]; then
  echo "Missing $APP; run ./build.sh first." >&2
  exit 1
fi

CREATE_DMG_TOOL="${CREATE_DMG:-$(command -v create-dmg || true)}"
if [ -z "$CREATE_DMG_TOOL" ] || [ ! -x "$CREATE_DMG_TOOL" ]; then
  echo "create-dmg is required." >&2
  echo "Install it with 'brew install create-dmg', or set CREATE_DMG to its path." >&2
  exit 1
fi

VERSION="$(plutil -extract CFBundleShortVersionString raw "$APP/Contents/Info.plist")"
OUTPUT="E-Ink-Assistant-v${VERSION}.dmg"
WORK="$(mktemp -d /private/tmp/eink-assistant-dmg.XXXXXX)"
STAGE="$WORK/stage"

cleanup() {
  if [[ "$WORK" == /private/tmp/eink-assistant-dmg.* ]]; then
    rm -rf "$WORK"
  fi
}
trap cleanup EXIT

mkdir -p "$STAGE"
cp -R "$APP" "$STAGE/"

"$CREATE_DMG_TOOL" \
  --overwrite \
  --volname "E-Ink Assistant" \
  --volicon Resources/AppIcon.icns \
  --background "$BACKGROUND" \
  --window-pos 120 120 \
  --window-size 660 400 \
  --text-size 13 \
  --icon-size 96 \
  --icon "E-Ink Assistant.app" 170 210 \
  --hide-extension "E-Ink Assistant.app" \
  --app-drop-link 490 210 \
  --no-internet-enable \
  --applescript-sleep-duration 10 \
  "$OUTPUT" "$STAGE"

hdiutil verify "$OUTPUT"
echo "Wrote $OUTPUT"
