# Windows changelog

## 1.2 — Unreleased

- Separates an already-usable ACM pipeline from an ACM toggle that Windows
  actually permits the app to change. A retained `enabled` bit no longer causes
  an unsupported toggle request or a misleading red feature-failure warning.
- Defers app-owned ACM restoration while Duplicate mode temporarily suppresses
  the toggle, keeping the recovery journal until a controllable topology
  returns. An unmodifiable ACM state outside Duplicate mode is treated as an
  externally owned fixed baseline.

## 1.1 — Internal test baseline

- Native Qt Widgets tray application for Windows 7 SP1 through Windows 11.
- Per-display text contrast, video enhancement, advanced tone curves, and
  persistent e-ink display selection.
- Saturation and RGB through the Windows 10 MHC2 path or Windows 11 24H2+
  Auto Color Management. Windows 10 requires WDDM 2.6+, MatrixDDI and exact
  target mapping, defaults the controls off, and protects first enable with a
  centered dimming five-second warning plus a fifteen-second independent
  rollback watchdog. Timeout and voluntary rollback remain retryable; only a
  real test-application failure rejects the fingerprint.
- Exact display-to-GPU mapping with Intel, NVIDIA, and AMD control-panel
  fallbacks when E-Ink Assistant cannot adjust saturation directly.
- Duplicate-display support with per-target enumeration, an explicit warning
  naming the other affected screen, one tuning owner per clone group, and
  transactional handling when switching between Duplicate and Extend. Clone
  detection uses the Windows 7-compatible active-path representation. Because
  Windows defers ICC/MHC2 application in Duplicate mode, Saturation and RGB are
  marked unavailable without a safety test or GPU-panel recommendation; the UI
  recommends Extend instead. Cleanup treats an already-removed inherited
  profile association as a successful rollback.
- Session-safe Windows Light Mode and Night Light controls with restoration,
  crash recovery, local progress feedback, and transition-only focus retention.
- Stable notification-area behavior without private icon-promotion writes,
  plus a compact no-arrow welcome guide showing the exact tray icon and manual
  pinning steps. The guide uses one white card and a high-DPI rendering of the
  same vector tray glyph.
- Main-panel hardware-baseline reminder matching macOS v2.5, with session-only
  dismissal and a separately persisted “never show again” action.
- Welcome completion waits for the closing dialog's focus transition before
  showing the main panel, preventing an immediate flash-and-hide.
- Immediate visual exit: the panel, welcome window, menu, and tray icon hide
  before display-state restoration continues in the background.
- Reproducible multilingual per-machine installer with payload validation,
  uninstall metadata, Start menu integration, and an optional desktop
  shortcut.
- In-place updates request normal application shutdown first, then offer a
  force-close-and-continue fallback when the running app cannot exit.
- Neutral modern installer and uninstaller appearance with native light-theme
  colors, automatic dark mode, product artwork, and simplified page chrome.

These entries record the intended Windows source baselines only. No Windows
release or public website update has been made.
