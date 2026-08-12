// Settings model for the e-ink assistant.
//
// Everything is keyed by the display's UUID rather than its CGDirectDisplayID,
// because the numeric ID is not stable across reboots or reconnections — a
// setting stored against it silently attaches to the wrong panel later.

import Foundation
import CoreGraphics
import ApplicationServices

/// Stable per-display identity, safe to persist.
func displayUUIDString(_ id: CGDirectDisplayID) -> String? {
    guard let uuid = CGDisplayCreateUUIDFromDisplayID(id)?.takeRetainedValue() else {
        return nil
    }
    return CFUUIDCreateString(nil, uuid) as String?
}

/// Shadow-lift strength. The three levels are the ones that survived tuning.
enum EnhanceLevel: String, CaseIterable {
    case off, subtle, medium, strong

    var label: String {
        switch self {
        case .off: return "Off"
        case .subtle: return "Subtle"
        case .medium: return "Medium"
        case .strong: return "Strong"
        }
    }

    var curve: ToneCurve? {
        switch self {
        case .off: return nil
        case .subtle: return ToneCurve(knee: 0.25, gamma: 0.75)
        case .medium: return ToneCurve(knee: 0.35, gamma: 0.60)
        case .strong: return ToneCurve(knee: 0.45, gamma: 0.45)
        }
    }

    /// Measured contrast cost on dark-mode text, used to warn honestly in the UI.
    var textContrastCost: String? {
        switch self {
        case .off: return nil
        case .subtle: return "about 13% less text contrast"
        case .medium: return "about 34% less text contrast"
        case .strong: return "about 56% less text contrast"
        }
    }
}

// MARK: - Persistence

enum EinkSettings {
    private static let defaults = UserDefaults.standard

    static func isEink(_ id: CGDirectDisplayID) -> Bool {
        guard let uuid = displayUUIDString(id) else { return false }
        return defaults.bool(forKey: "eink-\(uuid)")
    }

    static func setEink(_ value: Bool, for id: CGDirectDisplayID) {
        guard let uuid = displayUUIDString(id) else { return }
        defaults.set(value, forKey: "eink-\(uuid)")
    }

    static func enhance(_ id: CGDirectDisplayID) -> EnhanceLevel {
        guard let uuid = displayUUIDString(id),
              let raw = defaults.string(forKey: "enhance-\(uuid)"),
              let level = EnhanceLevel(rawValue: raw)
        else { return .off }
        return level
    }

    static func setEnhance(_ level: EnhanceLevel, for id: CGDirectDisplayID) {
        guard let uuid = displayUUIDString(id) else { return }
        defaults.set(level.rawValue, forKey: "enhance-\(uuid)")
    }
}

/// Pushes a display's stored enhance level to the hardware.
///
/// The gamma table this uses is volatile — macOS resets it on sleep, display
/// reconfiguration and logout — so this has to be re-run on those events. That
/// is why Video Enhance needs the app running, while saturation does not.
func reapplyEnhance(displayID: CGDirectDisplayID) {
    let level = EinkSettings.isEink(displayID) ? EinkSettings.enhance(displayID) : .off
    if let curve = level.curve {
        applyToneCurveLive(curve, displayID: displayID)
    } else {
        clearToneCurveLive(displayID: displayID)
    }
}

/// Clears every display's tone curve, without needing any app state.
///
/// Used on quit. It deliberately does not go through the UI model: that model
/// is only wired up when the menu bar panel is first opened, so relying on it
/// meant quitting without ever opening the panel left the curve applied.
func restoreAllDisplaysToneCurves() {
    var count: UInt32 = 0
    CGGetActiveDisplayList(0, nil, &count)
    var ids = [CGDirectDisplayID](repeating: 0, count: Int(count))
    CGGetActiveDisplayList(count, &ids, &count)
    for id in ids { clearToneCurveLive(displayID: id) }
}

// MARK: - Saturation as app-managed state

extension EinkSettings {
    static func saturation(_ id: CGDirectDisplayID) -> Double {
        guard let uuid = displayUUIDString(id),
              let v = UserDefaults.standard.object(forKey: "saturation-\(uuid)") as? Double
        else { return 1.0 }
        return v
    }

    static func setSaturation(_ value: Double, for id: CGDirectDisplayID) {
        guard let uuid = displayUUIDString(id) else { return }
        UserDefaults.standard.set(value, forKey: "saturation-\(uuid)")
    }
}

/// Removes saturation profiles this app installed, leaving anything else alone.
///
/// Used on quit, so the display goes back to how it was found. Only profiles we
/// recognise as ours are dropped — `installedSaturation` returns nil for a
/// user's own calibration, which must not be disturbed.
func restoreAllDisplaysSaturation() {
    var count: UInt32 = 0
    CGGetActiveDisplayList(0, nil, &count)
    var ids = [CGDirectDisplayID](repeating: 0, count: Int(count))
    CGGetActiveDisplayList(count, &ids, &count)
    for id in ids where installedSaturation(displayID: id) != nil {
        restoreProfile(displayID: id)
    }
}
