# E-Ink Assistant

<p align="center">
  <b>English</b> &nbsp;·&nbsp;
  <a href="docs/i18n/README.zh-Hans.md">简体中文</a> &nbsp;·&nbsp;
  <a href="docs/i18n/README.zh-Hant.md">繁體中文</a> &nbsp;·&nbsp;
  <a href="docs/i18n/README.ja.md">日本語</a>
</p>

<p align="center">
  <img src="Resources/AppIcon.png" alt="E-Ink Assistant app icon" width="128">
</p>

**Tune black-and-white and color e-ink displays on macOS and Windows.**

[Visit the product website](https://kiteretsu903.github.io/eink-assistant/)

E-Ink Assistant adjusts text contrast, shadow detail, and color on the e-ink
displays you choose. Other displays stay unchanged. The macOS edition runs in
the menu bar; the Windows edition runs in the system tray.

[Download macOS 2.6](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-v2.6.dmg) ·
[Download Windows 1.2](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-Windows-1.2-Setup.exe) ·
[View all releases](https://github.com/kiteretsu903/eink-assistant/releases)

Free, open source, and MIT licensed.

## Features and system requirements

| Feature | macOS | Windows |
|---|---|---|
| Supported systems | **macOS 14 or later**<br>Apple silicon only | **Windows 7 SP1 through Windows 11**<br>x64 computers |
| Where the app runs | Menu bar | System tray |
| Choose specific e-ink displays | Yes. Other displays stay unchanged. | Same as macOS |
| Text Contrast | Four levels: Medium, Strong, Sharp, Solid | Same as macOS |
| Video Enhance | Three levels: Subtle, Medium, Strong | Same as macOS |
| Advanced curve and presets | Live curve editor and five named presets | Same as macOS |
| Saturation and RGB | Per-display color profile; 0%–300% saturation and 0%–200% RGB | Available on applicable Windows 10 2004 and Windows 11 21H2+ systems; the available method depends on the system and hardware |
| Reduce Shaking | Available on supported external displays; turns on when a display is marked as e-ink | Not available. Windows has no unified public per-display dithering control, and most Windows systems probably do not need it. |
| Reduce Transparency & Motion | Available through a one-time, user-confirmed helper | Available from Windows 7 SP1 through compatible system APIs |
| System light mode | Not changed | Session-only Windows Light Mode on Windows 10 1903+ |
| Night Shift / Night Light | Per-display Night Shift and True Tone exclusion; requires administrator approval and reconnect | Night Light settings from Windows 10 1703+; direct Disable Night Light control on Windows 11 24H2+ |
| Mirrored / duplicated displays | Mirrored physical displays remain individually selectable | Tone curves affect the shared source; Saturation and RGB require Extend mode |
| Restore changes | Temporary curves, color profiles, and dithering restore on quit; the Night Shift / True Tone exclusion is persistent | Temporary gamma, color, visual, and Night Light changes restore on quit; color and Night Light also recover after an abnormal exit |
| Launch at login | Supported | Supported |
| Interface languages | English, Simplified Chinese, Traditional Chinese, Japanese | Same as macOS |
| Administrator access | Only for the optional Night Shift / True Tone exclusion | Required by the installer and app |

[macOS details](macos/README.md) ·
[Windows compatibility and setup](WINDOWS.md) ·
[macOS changelog](CHANGELOG.md) ·
[Windows changelog](windows/CHANGELOG.md)

<p align="center">
  <img src="docs/en/app-main-v2-1.png" alt="E-Ink Assistant v2.1 in English" width="440">
</p>

## What you can do

- **Reading:** darken faint body and secondary text with Text Contrast.
- **Photos and video:** lift dark image detail with Video Enhance.
- **Color e-ink:** adjust saturation and RGB when the platform and display path
  support it.
- **Per-display settings:** mark only the e-ink panels you want the app to
  change; save separate settings for each one.
- **Advanced tuning:** adjust the complete tone curve and save five named
  presets.

### Color e-ink

- Everything listed for B&W e-ink below.
- **Saturation compensation** to strengthen a narrow color gamut, with six
  presets and a 0%–300% slider.
- **Direct RGB correction** from 0%–200% per channel to remove red, green, or
  blue color casts. RGB values are saved separately for each display.
- **Per-display Night Shift and True Tone control** so macOS color-temperature
  processing does not fight deliberate color tuning.

### Black-and-white e-ink

- **Reduce Shaking** to stop visible macOS dithering shimmer on supported
  displays.
- **Text Contrast** to make faint and secondary text darker and easier to read.
- **Video Enhance** to recover shadow detail in videos and photos.
- **Reduce Transparency & Motion** to simplify system visuals and avoid
  slow-refresh animation.
- **Advanced tone-curve tuning** for the panel's specific contrast response.
- **Optional Night Shift and True Tone control** when macOS tone shifting
  visibly affects grayscale output.

---

## Feature details

### Reduce Shaking: B&W and color

macOS dithers display output to smooth gradients. An LCD refreshes the pattern
away; e-ink holds every pixel, so the pattern can become a constant shimmer.
Disabling it makes the picture sit still. This turns on automatically when you
mark a supported display as e-ink.

### Text Contrast: B&W and color

Darkens text so it separates from the page on a low-contrast panel. Faint and
secondary text gains the most: at the strongest level its signal contrast is
roughly quadrupled.

![Illustrative text contrast before and after](docs/en/text-contrast-editorial.png)

Four levels: **Medium, Strong, Sharp, Solid**. Medium now equals the former
Strong setting; each level after it is more aggressive. Solid heavily crushes
the black point for maximum separation, which looks crisper but loses more gray
detail and soft edges.

### Video Enhance: B&W and color

Brightens only the darkest tones while leaving mid-tones and highlights alone,
so shadow detail that e-ink normally crushes becomes visible again.

![Illustrative video enhancement before and after](docs/en/video-enhance-editorial.png)

Three levels: **Subtle, Medium, Strong**. It cannot distinguish dark imagery
from dark text, so it lightens text too. **Use it for video and photos, and turn
it off for reading.** Video Enhance and Text Contrast are mutually exclusive.

### Saturation: color e-ink

Color e-ink places a color filter over a monochrome panel, which costs much of
the gamut. Saturation compensation restores stronger color signals. B&W panels
can simply leave this at 100%.

![Illustrative saturation before and after](docs/en/saturation-editorial.png)

Presets **B&W / Faded / Factory / Enhanced / Vivid / Anime**, or a slider from
0% to 300%.

### RGB: color e-ink

Adjust **Red, Green, and Blue directly from 0% to 200%** to correct a panel's
color cast. The controls are collapsed by default; select **RGB** to reveal the
three sliders. The compact row always shows the current values. Each display
keeps its own RGB values. **Reset RGB** returns all three channels to the
neutral 100% setting.

RGB and Saturation share one per-display color profile, so they work together
without interfering with Text Contrast or Video Enhance.

### Disable Night Shift & True Tone: primarily color e-ink

Both features shift color and tone, which can fight deliberate display tuning.
E-Ink Assistant can mark only the selected display as a television so macOS
withholds them from that panel. This may also help a B&W panel affected by tone
shifting.

Other screens keep Night Shift and True Tone. This requires an administrator
password and **disconnecting and reconnecting that display**, and is the one setting not undone on quit. Existing
overrides, including custom scaled resolutions, are preserved.

### Reduce Transparency & Motion: B&W and color

These system accessibility settings improve legibility and avoid animation that
slow-refresh panels cannot display well. A one-time, user-confirmed Shortcuts
helper gives the app a safe On/Off control and optional connection automation.

### Advanced: B&W and color

Full control of knee, gamma, black point, and white point, with a live plot and
five saveable, renameable slots.

<p align="center">
  <img src="docs/en/app-advanced-v2-1.png" alt="Advanced mode in E-Ink Assistant v2.1" width="440">
</p>

> The comparison images are illustrative feature previews. Actual results depend
> on the panel and source material.

---

## Install

### macOS

Requires **macOS 14 or later** on **Apple silicon**.

1. [Download the macOS 2.6 DMG](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-v2.6.dmg).
2. Open it and drag **E-Ink Assistant** into **Applications**.
3. Try to open the app once. If macOS blocks it, open **System Settings →
   Privacy & Security** and select **Open Anyway**.

This is independently developed software and is not currently on the App Store.
macOS will show an “unable to verify” warning the first time you open it. The
code is fully open source, so you can review it before deciding whether to use
it.

If **Open Anyway** does not appear after moving the app to Applications, open
Terminal and run:

```
xattr -dr com.apple.quarantine "/Applications/E-Ink Assistant.app"
```

### Windows

Requires **Windows 7 SP1 through Windows 11** on an **x64 computer**.

1. [Download the Windows 1.2 installer](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-Windows-1.2-Setup.exe).
2. Run Setup and approve the administrator prompt.
3. Open E-Ink Assistant from the Start menu or system tray.

See [WINDOWS.md](WINDOWS.md) for exact feature availability by Windows version,
GPU, driver, and display connection.

---

## Using it

<p align="center">
  <img src="docs/en/app-displays-v2-1.png" alt="Marking displays in E-Ink Assistant v2.1" width="440">
</p>

1. Open the app from the macOS menu bar or Windows system tray.
2. Mark each B&W or color e-ink display you want to tune.
3. Set a balanced hardware contrast in the monitor's own menu first.
4. Choose Text Contrast for reading or Video Enhance for media, not both.
5. On color e-ink, adjust Saturation and RGB when the platform supports them.

On macOS, the system-wide **Reduce Transparency & Motion** row has a one-time **Install &
Enable** setup. Confirm **Add Shortcut** in Apple's Shortcuts app. The bundled
helper accepts Text only, recognizes exact `on` and `off` commands, ignores
everything else, and ends with `Nothing` so it produces no output. It is not
exposed in Share Sheet, Spotlight, Quick Actions, or the lock-screen interface.
It can run in the background while the Mac is locked so automatic display
following and quit cleanup remain reliable. The app does not list or inspect
your other shortcuts.

Automatic mode turns both settings on when a marked e-ink display connects and
off only after the last marked e-ink display disconnects. Quitting the app also
turns them off.

**Display adjustments restore when you quit** and reapply when you launch. Turn
on **Launch at Login** for automatic startup.

---

## Display scope and presets

The core tools, including Reduce Shaking, Text Contrast, Video Enhance,
accessibility controls, and Advanced curves, can benefit **both B&W and color
e-ink**.
Saturation and RGB correction are specifically for color panels.

Set the monitor hardware first. The bundled presets were tuned by eye on a
**Bigme B251 Pro** (R2 FW V2.0) using **Web Mode, Hardware Gamma Level 3,
Contrast 50, Color Restore Mode off**. A B&W panel or another color model will
need its own values; Advanced mode exposes the complete curve for that purpose.
Use custom tuning at your own risk.

Reduce Shaking is Apple Silicon only and hides itself where unsupported.

---

## More

- [CHANGELOG.md](CHANGELOG.md): what changed in each version
- [TECHNICAL.md](TECHNICAL.md): implementation, measurements, and approaches
  that do *not* work on modern macOS
- [mac-saturation](https://github.com/kiteretsu903/mac-saturation): the color
  mechanism investigation and a profile-export CLI

## License and credits

MIT, see [LICENSE](LICENSE).

**Reduce Shaking is based on [Stillcolor](https://github.com/aiaf/Stillcolor) by
Abdullah Arif** (MIT). Stillcolor discovered that display dithering can be
disabled through the `enableDither` I/O Registry property. This project
reimplements that idea per display; credit for the discovery belongs to
Stillcolor. Thank you.

Full notices in [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
