# Changelog

## v2.4 — 2026-09-01

- The control panel now uses the visible height of the screen containing the
  menu-bar item. Short panels keep their natural height; tall panels scroll.
- The panel keeps 40 points between its visible bottom edge and the Dock or
  screen edge, including room for the shadow.

## v2.3 — 2026-09-01

- Increased all four Text Contrast presets. Medium uses the previous Strong
  curve; Strong, Sharp, and Solid are progressively darker.
- Sharp uses gamma 5.00 with a 0.10 black point. Solid uses gamma 6.00 with a
  0.34 black point. These settings darken faint text but remove more gray detail
  and make edges harder.

## v2.2 — 2026-08-14

- Changing the app language now updates the **How this works** heading and its
  **Show more / Show less** button immediately.

## v2.1 — 2026-08-14

- The settings helper can run while the Mac is locked, preventing Shortcuts
  warnings during display disconnect, app quit, and power-off cleanup.
- The helper is named `E-Ink Assistant Settings Helper`. Users upgrading from
  v2.0 must add it once, then may remove the old helper from Shortcuts.

## v2.0 — 2026-08-13

- Added per-display Red, Green, and Blue controls from 0% to 200%.
- Added automatic Reduce Transparency and Reduce Motion control through a
  user-confirmed Shortcuts helper.
- Added per-display Night Shift and True Tone control with reconnect status.
- Added English, Simplified Chinese, Traditional Chinese, and Japanese.
- Updated the panel controls, app icon, documentation, and screenshots.
- Added a drag-to-Applications DMG installer.

## v1.1

- Added Night Shift and True Tone control for one selected display by setting
  its `DisplayIsTV` override. Other display-override keys are preserved.
- This action requires an administrator password and a display reconnect. It
  is not undone when the app quits.

## v1.0

- Initial release for macOS 14 and Apple Silicon.
- Added per-display Reduce Shaking, Saturation, Text Contrast, Video Enhance,
  custom tone curves, five saved curve slots, a live curve plot, Launch at
  Login, and display restoration when the app quits.
- Added English and Simplified Chinese.
- The app is ad-hoc signed, so first launch requires Gatekeeper approval.
