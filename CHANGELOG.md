# Changelog

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

First release. Per-display tuning for colour e-ink panels on macOS.

**Features**

- **Reduce Shaking** turns off display dithering for a chosen display, which
  e-ink shows as a constant shimmer. Based on
  [Stillcolor](https://github.com/aiaf/Stillcolor) by Abdullah Arif, narrowed
  from all displays to one.
- **Saturation** compensates for the narrow gamut of a colour filter array, via
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
