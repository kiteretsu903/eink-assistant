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
        case .off: return L("level.off")
        case .subtle: return L("level.subtle")
        case .medium: return L("level.medium")
        case .strong: return L("level.strong")
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
        case .subtle: return L("cost.13")
        case .medium: return L("cost.34")
        case .strong: return L("cost.56")
        }
    }
}

/// Text contrast strength — the mirror of EnhanceLevel. Gamma above 1 darkens
/// the low end while pinning white, pushing text toward the panel's floor.
///
/// The four levels were tuned by eye on a Bigme B251 Pro. `solid` adds a
/// black-point crush that turns antialiased glyph edges into solid black.
enum TextLevel: String, CaseIterable {
    case off, medium, strong, sharp, solid

    var label: String {
        switch self {
        case .off: return L("level.off")
        case .medium: return L("level.medium")
        case .strong: return L("level.strong")
        case .sharp: return L("level.sharp")
        case .solid: return L("level.solid")
        }
    }

    var curve: ToneCurve? {
        switch self {
        case .off:    return nil
        case .medium: return ToneCurve(knee: 0.55, gamma: 1.70)
        case .strong: return ToneCurve(knee: 0.65, gamma: 2.10)
        case .sharp:  return ToneCurve(knee: 0.90, gamma: 3.00)
        case .solid:  return ToneCurve(knee: 0.90, gamma: 3.00, blackPoint: 0.16)
        }
    }

    var detail: String? {
        switch self {
        case .off: return nil
        case .medium: return L("text.detail.medium")
        case .strong: return L("text.detail.strong")
        case .sharp: return L("text.detail.sharp")
        case .solid: return L("text.detail.solid")
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

    static func textLevel(_ id: CGDirectDisplayID) -> TextLevel {
        guard let uuid = displayUUIDString(id),
              let raw = defaults.string(forKey: "text-\(uuid)"),
              let level = TextLevel(rawValue: raw)
        else { return .off }
        return level
    }

    static func setTextLevel(_ level: TextLevel, for id: CGDirectDisplayID) {
        guard let uuid = displayUUIDString(id) else { return }
        defaults.set(level.rawValue, forKey: "text-\(uuid)")
    }
}

/// Pushes a display's stored enhance level to the hardware.
///
/// The gamma table this uses is volatile — macOS resets it on sleep, display
/// reconfiguration and logout — so this has to be re-run on those events. That
/// is why Video Enhance needs the app running, while saturation does not.
/// The curve a display should currently be showing.
///
/// Text and Video Enhance both drive the one gamma table and pull in opposite
/// directions, so they are mutually exclusive; Text wins if both are somehow set.
func effectiveCurve(displayID: CGDirectDisplayID) -> ToneCurve? {
    guard EinkSettings.isEink(displayID) else { return nil }
    // A custom curve takes precedence over the presets it replaces.
    if EinkSettings.advanced(displayID) {
        let custom = EinkSettings.customCurve(displayID)
        return custom.isIdentity ? nil : custom
    }
    if let text = EinkSettings.textLevel(displayID).curve { return text }
    return EinkSettings.enhance(displayID).curve
}

func reapplyEnhance(displayID: CGDirectDisplayID) {
    if let curve = effectiveCurve(displayID: displayID) {
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

// MARK: - Color profile adjustments as app-managed state

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

    static func rgbBalance(_ id: CGDirectDisplayID) -> RGBBalance {
        guard let uuid = displayUUIDString(id),
              let raw = defaults.array(forKey: "rgb-\(uuid)") as? [Double],
              raw.count == 3
        else { return .identity }
        return RGBBalance(red: raw[0], green: raw[1], blue: raw[2]).clamped
    }

    static func setRGBBalance(_ balance: RGBBalance, for id: CGDirectDisplayID) {
        guard let uuid = displayUUIDString(id) else { return }
        let value = balance.clamped
        defaults.set([value.red, value.green, value.blue], forKey: "rgb-\(uuid)")
    }

    /// The selected preset is stored separately from the numeric saturation.
    /// A manual slider adjustment writes -1 so landing on a preset's exact
    /// value does not incorrectly make that preset look selected again.
    static func saturationPreset(_ id: CGDirectDisplayID) -> Int? {
        guard let uuid = displayUUIDString(id),
              UserDefaults.standard.object(forKey: "saturation-preset-\(uuid)") != nil
        else { return nil }
        let index = UserDefaults.standard.integer(forKey: "saturation-preset-\(uuid)")
        return index >= 0 ? index : nil
    }

    static func setSaturationPreset(_ index: Int?, for id: CGDirectDisplayID) {
        guard let uuid = displayUUIDString(id) else { return }
        UserDefaults.standard.set(index ?? -1,
                                  forKey: "saturation-preset-\(uuid)")
    }
}

/// Removes color profiles this app installed, leaving anything else alone.
///
/// Used on quit, so the display goes back to how it was found. Only profiles we
/// recognize as ours are dropped — `installedSaturation` returns nil for a
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

extension EinkSettings {
    static func reduceShaking(_ id: CGDirectDisplayID) -> Bool {
        guard let uuid = displayUUIDString(id) else { return false }
        return defaults.bool(forKey: "shaking-\(uuid)")
    }

    static func setReduceShaking(_ value: Bool, for id: CGDirectDisplayID) {
        guard let uuid = displayUUIDString(id) else { return }
        defaults.set(value, forKey: "shaking-\(uuid)")
    }
}

// MARK: - Advanced (fully custom) curve

extension EinkSettings {
    static func advanced(_ id: CGDirectDisplayID) -> Bool {
        guard let uuid = displayUUIDString(id) else { return false }
        return defaults.bool(forKey: "advanced-\(uuid)")
    }

    static func setAdvanced(_ value: Bool, for id: CGDirectDisplayID) {
        guard let uuid = displayUUIDString(id) else { return }
        defaults.set(value, forKey: "advanced-\(uuid)")
    }

    static func customCurve(_ id: CGDirectDisplayID) -> ToneCurve {
        guard let uuid = displayUUIDString(id),
              let raw = defaults.array(forKey: "curve-\(uuid)") as? [Double],
              raw.count == 4
        else { return ToneCurve(knee: 0.90, gamma: 3.00) }
        return ToneCurve(knee: raw[0], gamma: raw[1],
                         blackPoint: raw[2], whitePoint: raw[3])
    }

    static func setCustomCurve(_ c: ToneCurve, for id: CGDirectDisplayID) {
        guard let uuid = displayUUIDString(id) else { return }
        defaults.set([c.knee, c.gamma, c.blackPoint, c.whitePoint], forKey: "curve-\(uuid)")
    }
}

// MARK: - Saved custom curves

/// Up to five curves the user can store and re-apply. Deliberately global
/// rather than per display, so a curve tuned on one panel can be applied to
/// another.
enum CurvePresets {
    static let slotCount = 5

    private static func key(_ slot: Int) -> String { "preset-slot-\(slot)" }
    private static func nameKey(_ slot: Int) -> String { "preset-name-\(slot)" }

    static func name(slot: Int) -> String? {
        guard (0..<slotCount).contains(slot),
              let n = UserDefaults.standard.string(forKey: nameKey(slot)),
              !n.trimmingCharacters(in: .whitespaces).isEmpty
        else { return nil }
        return n
    }

    static func setName(_ name: String, slot: Int) {
        guard (0..<slotCount).contains(slot) else { return }
        let trimmed = name.trimmingCharacters(in: .whitespaces)
        if trimmed.isEmpty {
            UserDefaults.standard.removeObject(forKey: nameKey(slot))
        } else {
            UserDefaults.standard.set(trimmed, forKey: nameKey(slot))
        }
    }

    /// The label to show on a slot: its name, or its number if unnamed.
    static func label(slot: Int) -> String { name(slot: slot) ?? "\(slot + 1)" }

    static func curve(slot: Int) -> ToneCurve? {
        guard (0..<slotCount).contains(slot),
              let raw = UserDefaults.standard.array(forKey: key(slot)) as? [Double],
              raw.count == 4
        else { return nil }
        return ToneCurve(knee: raw[0], gamma: raw[1],
                         blackPoint: raw[2], whitePoint: raw[3])
    }

    static func all() -> [ToneCurve?] {
        (0..<slotCount).map { curve(slot: $0) }
    }

    static func save(_ c: ToneCurve, slot: Int) {
        guard (0..<slotCount).contains(slot) else { return }
        UserDefaults.standard.set([c.knee, c.gamma, c.blackPoint, c.whitePoint],
                                  forKey: key(slot))
    }

    static func clear(slot: Int) {
        guard (0..<slotCount).contains(slot) else { return }
        UserDefaults.standard.removeObject(forKey: key(slot))
        UserDefaults.standard.removeObject(forKey: nameKey(slot))
    }

    /// A compact description for tooltips, so a slot is identifiable without
    /// applying it.
    static func summary(_ c: ToneCurve) -> String {
        String(format: "knee %.2f  γ %.2f  black %.2f  white %.2f",
               c.knee, c.gamma, c.blackPoint, c.whitePoint)
    }
}
