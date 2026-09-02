# Changelog

## v2.3 — 2026-09-01

- **More aggressive Text Contrast presets.** Medium now uses the previous
  Strong curve. Strong and Sharp darken secondary and faint text more heavily,
  with a steeper progression between the four levels.

- **A substantially stronger Solid level.** Solid now combines a full-range
  gamma curve with a much heavier black-point crush. In the signal model used
  by ReadingLab, secondary-text contrast rises from 15.5:1 to 19.3:1 and
  tertiary-text contrast from 6.2:1 to 11.0:1. The tradeoff is deliberately
  harder edges and greater loss of gray detail.

## v2.2 — 2026-08-14

- **Fixed live language switching.** Changing the in-app language now refreshes
  the self-contained **How this works** section and its **Show more / Show
  less** control immediately. A section rendered in Japanese, Chinese, or
  English can no longer remain in that language after the rest of the panel
  switches.

## v2.1 — 2026-08-14

- **Fixed locked-screen automation.** The settings helper can now run in the
  background while the Mac is locked, so display-disconnection automation,
  app-exit cleanup, and power-off cleanup no longer produce a Shortcuts warning.
  It remains unavailable from Share Sheet, Spotlight, Quick Actions, and the
  lock-screen interface; it still accepts only exact `on` and `off` commands
  and provides no output.

- **Reliable upgrade path.** The corrected workflow uses the new fixed name
  `E-Ink Assistant Settings Helper`, so v2.1 never mistakes the older v2.0
  helper for the updated one. Existing users confirm **Add Shortcut** once after
  upgrading. The unused `E-Ink Assistant Accessibility Helper` can then be
  deleted from Shortcuts.

## v2.0 — 2026-08-13

- **Built for both B&W and color e-ink.** The app and documentation now make
  clear which shared tools help both panel types and which color controls are
  specific to color e-ink.

- **Direct per-display RGB control** from 0% to 200% for each Red, Green, and
  Blue channel. RGB is composed with Saturation in one display profile, saved
  per display, and collapsed by default behind a compact live-value summary.

- **Safe Reduce Transparency & Motion control.** A bundled, user-confirmed
  Shortcuts helper provides explicit On and Off commands without inspecting
  the user's other shortcuts or producing output. The app can automatically
  turn both settings on when a marked e-ink display connects, off only after
  the last marked display disconnects, and off whenever the app quits.

- **E-ink-first interface redesign.** Larger black text, hard section and
  control outlines, more visible sliders, aligned controls, bold selected
  states, compact loading feedback, and clearer per-display grouping improve
  readability without relying on subtle gray backgrounds.

- **More accurate per-display system controls.** Night Shift and True Tone are
  explicitly disabled only for the named display, with reconnect guidance and
  reconnect-state tracking instead of a misleading restart message.

- **Four complete languages:** American English, Simplified Chinese,
  Traditional Chinese, and Japanese. Unsupported system languages fall back to
  English.

- **New native macOS app icon and refreshed documentation,** including
  full-resolution screenshots and separate benefit summaries for B&W and color
  e-ink displays.

- **Standard drag-to-Applications DMG installer** with a custom background,
  clear install arrow, Applications alias, and reusable release packaging.

## v1.1

- **Disable Night Shift & True Tone** per display. macOS withholds both from
  displays it classifies as televisions, so this writes a `DisplayIsTV` display
  override for the chosen display. Existing override keys, such as custom
  scaled resolutions, are preserved.

  It affects that display alone: other screens keep Night Shift and True Tone,
  since this is a per-display override rather than the system-wide switch.

  Unlike everything else in the app this needs an administrator password, takes
  effect only after a restart, and is **not** undone when you quit. The app
  shows a reminder while a restart is still pending.

  Same mechanism as
  [macos-display-role-switcher](https://github.com/kiteretsu903/macos-display-role-switcher).

## v1.0

First release. Per-display tuning for color e-ink panels on macOS.

**Features**

- **Reduce Shaking** turns off display dithering for a chosen display, which
  e-ink shows as a constant shimmer. Based on
  [Stillcolor](https://github.com/aiaf/Stillcolor) by Abdullah Arif, narrowed
  from all displays to one.
- **Saturation** compensates for the narrow gamut of a color filter array, via
  a synthesized ICC display profile. Presets 原厂 / 增强 / 艳丽 / 动漫, or a
  slider from 60% to 300%.
- **Text Contrast** darkens text so it reads on a low-contrast panel. Four
  levels: Medium, Strong, Sharp, Solid.
- **Video Enhance** brightens only the darkest tones, leaving mid-tones and
  highlights untouched. Three levels.
- **Advanced** exposes the curve directly (knee, gamma, black point, white
  point) with five saveable, renameable slots.
- Live plot of the curve currently applied.
- English and 简体中文, following the system language, with an in-app picker
  that switches immediately.
- Launch at Login. Quitting restores every display to its original state.

**Known limitations**

- Presets were tuned on one panel in one configuration; see the README.
- Reduce Shaking is Apple Silicon only, and hides itself where unsupported.
- Requires macOS 14 or later, Apple Silicon.
- Ad-hoc signed, so the first launch needs Gatekeeper approval.
