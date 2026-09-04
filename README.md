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

## Controls

| Control | Use it for | What it does |
|---|---|---|
| Text Contrast | Reading | Darkens faint text with Medium, Strong, Sharp, and Solid levels. Stronger levels trade gray detail for harder edges. |
| Video Enhance | Photos and video | Reveals shadow detail with Subtle, Medium, and Strong levels. Turn it off for reading because it also lightens dark text. |
| Saturation and RGB | Color e-ink | Provides six saturation presets, a 0%–300% saturation slider, and 0%–200% RGB correction when the platform supports it. |
| Reduce Shaking | Supported macOS displays | Stops visible dithering shimmer and turns on automatically for displays marked as e-ink. |
| Night Shift and True Tone | Displays affected by color-temperature changes | Excludes the selected macOS display. This requires administrator approval and reconnecting the display, and the setting remains after quit. |
| Reduce Transparency & Motion | Slow-refresh panels | Simplifies system visuals. macOS uses a one-time, user-confirmed helper. |
| Advanced curves | Panel-specific tuning | Adjusts knee, gamma, black point, and white point with a live plot and five named presets. |

<p align="center">
  <img src="docs/en/text-contrast-editorial.png" alt="Illustrative text contrast before and after" width="31%">
  <img src="docs/en/video-enhance-editorial.png" alt="Illustrative video enhancement before and after" width="31%">
  <img src="docs/en/saturation-editorial.png" alt="Illustrative saturation before and after" width="31%">
</p>

> These images illustrate the controls. Results depend on the panel and source
> material.

## Install

### macOS 14+, Apple silicon

1. Download the macOS 2.6 DMG using the link above.
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

### Windows 7 SP1 to Windows 11, x64

1. Download the Windows 1.2 installer using the link above.
2. Run Setup and approve the administrator prompt.
3. Open E-Ink Assistant from the Start menu or system tray.

See [WINDOWS.md](WINDOWS.md) for exact feature availability by Windows version,
GPU, driver, and display connection.

## Use it

<p align="center">
  <img src="docs/en/app-displays-v2-1.png" alt="Marking displays in E-Ink Assistant v2.1" width="440">
</p>

1. Open the app from the macOS menu bar or Windows system tray.
2. Mark each B&W or color e-ink display you want to tune.
3. Set a balanced hardware contrast in the monitor's own menu first.
4. Choose Text Contrast for reading or Video Enhance for media, not both.
5. On color e-ink, adjust Saturation and RGB when the platform supports them.

**Display adjustments restore when you quit** and reapply when you launch. Turn
on **Launch at Login** for automatic startup.

## Display setup

Set a balanced contrast in the monitor's own menu before adjusting the app. The
bundled presets were tuned by eye on a
**Bigme B251 Pro** (R2 FW V2.0) using **Web Mode, Hardware Gamma Level 3,
Contrast 50, Color Restore Mode off**. A B&W panel or another color model will
need its own values. Advanced mode exposes the complete curve, and settings are
saved separately for each display.

Reduce Shaking is Apple Silicon only and hides itself where unsupported.

<details>
<summary>macOS Reduce Transparency & Motion helper</summary>

The first use asks you to confirm **Add Shortcut** in Apple's Shortcuts app. The
bundled helper accepts only the exact Text commands `on` and `off`, produces no
output, and is not exposed in Share Sheet, Spotlight, Quick Actions, or the
lock-screen interface. It can run while the Mac is locked. The app does not
list or inspect your other shortcuts.

Automatic mode turns both settings on when a marked e-ink display connects and
off after the last marked display disconnects. Quitting the app also turns them
off.

</details>

## Project documentation

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
