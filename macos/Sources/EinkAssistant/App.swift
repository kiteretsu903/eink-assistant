// E-Ink Assistant — display tuning for B&W and color e-ink panels.
//
// Adjustments use two pipelines, deliberately different in how they persist:
//
//   Saturation/RGB written into the display's ICC profile.
//   Video Enhance  driven through the display's gamma table, which macOS resets
//                  on sleep and display changes. That needs the app running to
//                  be re-applied, which is why it is framed as a mode you turn
//                  on and off rather than a setting.

import SwiftUI
import AppKit
import CoreGraphics
import CoreFoundation
import ServiceManagement


// MARK: - Model

struct PanelState: Identifiable, Equatable {
    let id: CGDirectDisplayID
    let name: String
    let isBuiltin: Bool
    var isEink: Bool
    var saturation: Double
    var saturationPreset: Int?
    var rgbBalance: RGBBalance
    var enhance: EnhanceLevel
    var textLevel: TextLevel
    var advanced: Bool
    var custom: ToneCurve
    var reduceShaking: Bool
    var shakingSupported: Bool
    var isTelevision: Bool
    var roleNeedsReconnect: Bool
}

@MainActor
final class AssistantModel: ObservableObject {
    static let shared = AssistantModel()
    private static let hardwareNoticeSuppressKey = "hide-hardware-setup-notice"

    @Published var panels: [PanelState] = []
    @Published var launchAtLogin = false
    @Published var language: AppLanguage = Localization.current
    @Published var presets: [ToneCurve?] = CurvePresets.all()
    /// Bumped on rename so rows redraw; names live in CurvePresets.
    @Published var presetNamesVersion = 0
    /// Read directly from NSWorkspace. Changing them goes through the signed,
    /// user-confirmed Shortcuts helper because direct defaults writes fail.
    @Published var reduceTransparency = false
    @Published var reduceMotion = false
    @Published var helperReady = Shortcuts.wasInstalled
    @Published var helperInstalling = false
    @Published var helperRunning = false
    @Published var helperFailed = false
    /// Holds the requested On/Off state while macOS publishes the new values.
    /// Without this, SwiftUI immediately redraws from NSWorkspace's stale cache
    /// and makes a successful Off command appear to spring back to On.
    @Published var pendingAccessibilityState: Bool?
    @Published var autoAccessibility = UserDefaults.standard.bool(
        forKey: "accessibility-auto-follow")
    /// "Got it" hides the hardware reminder for this app session. The second
    /// action stores an explicit opt-out so it stays hidden after relaunch.
    @Published var showsHardwareSetupNotice = !UserDefaults.standard.bool(
        forKey: AssistantModel.hardwareNoticeSuppressKey)
    @Published var lastError: String?

    // Writing a gamma table or a color profile makes macOS post a storm of
    // didChangeScreenParameters notifications — measured at 13 and 15 for a
    // single write. Re-applying on each one feeds back into itself and the
    // display visibly flickers, so our own changes are ignored for a moment
    // and external ones are coalesced.
    private var selfChangeUntil = Date.distantPast
    private var reapplyTask: Task<Void, Never>?
    private var settledReapplyTask: Task<Void, Never>?
    private var forcedReapplyPending = false
    private var helperTask: Task<Void, Never>?
    private var accessibilityPresenceTask: Task<Void, Never>?
    private var connectedEinkUUIDs: Set<String> = []
    private var hasObservedEinkPresence = false

    private func markSelfChange() {
        selfChangeUntil = Date().addingTimeInterval(1.5)
    }

    /// `force` skips the self-change guard and re-applies everything
    /// unconditionally. It is reserved for wake and mirror/unmirror events,
    /// which cannot be caused by our color-profile or gamma-table writes.
    /// Keeping the force request sticky prevents a later AppKit notification
    /// for the same transition from replacing it with a guarded refresh.
    func scheduleReapply(force: Bool = false) {
        forcedReapplyPending = forcedReapplyPending || force
        if force { settledReapplyTask?.cancel() }
        reapplyTask?.cancel()
        let delay: UInt64 = forcedReapplyPending ? 200_000_000 : 700_000_000
        reapplyTask = Task { @MainActor [weak self] in
            try? await Task.sleep(nanoseconds: delay)
            guard !Task.isCancelled, let self else { return }
            let mustReapply = self.forcedReapplyPending
            self.forcedReapplyPending = false
            self.refresh()
            // A change we caused ourselves needs no response.
            guard mustReapply || Date() >= self.selfChangeUntil else { return }
            self.reapplyAll()

            // Displays can come back progressively after wake and mirror-mode
            // changes. Use a separate task so the notifications caused by our
            // own first write cannot cancel the settled second pass.
            guard mustReapply else { return }
            self.settledReapplyTask?.cancel()
            self.settledReapplyTask = Task { @MainActor [weak self] in
                try? await Task.sleep(nanoseconds: 2_000_000_000)
                guard !Task.isCancelled, let self else { return }
                self.refresh()
                self.reapplyAll()
            }
        }
    }

    init() {
        Localization.refresh()
        refresh()
        launchAtLogin = SMAppService.mainApp.status == .enabled

        // The gamma table is volatile: macOS clears it on wake and on any
        // display reconfiguration, so Video Enhance has to be re-asserted.
        NotificationCenter.default.addObserver(
            forName: NSApplication.didChangeScreenParametersNotification,
            object: nil, queue: .main
        ) { [weak self] _ in
            Task { @MainActor in self?.scheduleReapply() }
        }
        NSWorkspace.shared.notificationCenter.addObserver(
            forName: NSWorkspace.didWakeNotification,
            object: nil, queue: .main
        ) { [weak self] _ in
            Task { @MainActor in self?.scheduleReapply(force: true) }
        }
        // Display-only sleep, the common case, posts screensDidWake rather than
        // didWake, which covers system sleep. Both are needed.
        NSWorkspace.shared.notificationCenter.addObserver(
            forName: NSWorkspace.screensDidWakeNotification,
            object: nil, queue: .main
        ) { [weak self] _ in
            Task { @MainActor in self?.scheduleReapply(force: true) }
        }
        NSWorkspace.shared.notificationCenter.addObserver(
            forName: NSWorkspace.accessibilityDisplayOptionsDidChangeNotification,
            object: nil, queue: .main
        ) { [weak self] _ in
            Task { @MainActor in
                self?.refreshAccessibility()
                self?.reconcilePendingAccessibilityState()
            }
        }
        registerDisplayReconfigurationCallback()

        reapplyAll()
        verifyAccessibilityHelperOnLaunch()
    }

    func refresh() {
        let discoveredDisplays = controllableDisplays()
        // Built-in panels are rarely the e-ink target. Keep the system's
        // discovery order within each group, but list external displays first
        // so the most likely choices are immediately visible.
        let displays = discoveredDisplays.filter { !$0.isBuiltin }
            + discoveredDisplays.filter { $0.isBuiltin }
        let currentEinkUUIDs = Set(displays.compactMap { display -> String? in
            guard EinkSettings.isEink(display.id) else { return nil }
            return displayUUIDString(display.id)
        })
        let previousHadEink = !connectedEinkUUIDs.isEmpty
        let currentlyHasEink = !currentEinkUUIDs.isEmpty

        panels = displays.map { d in
            PanelState(
                id: d.id,
                name: d.name,
                isBuiltin: d.isBuiltin,
                isEink: EinkSettings.isEink(d.id),
                // Stored value is authoritative now: the profile is removed on
                // quit, so it is absent on a fresh launch.
                saturation: EinkSettings.saturation(d.id),
                saturationPreset: EinkSettings.saturationPreset(d.id),
                rgbBalance: EinkSettings.rgbBalance(d.id),
                enhance: EinkSettings.enhance(d.id),
                textLevel: EinkSettings.textLevel(d.id),
                advanced: EinkSettings.advanced(d.id),
                custom: EinkSettings.customCurve(d.id),
                reduceShaking: EinkSettings.reduceShaking(d.id),
                shakingSupported: Dither.isSupported(displayID: d.id),
                isTelevision: DisplayRole.isTelevision(displayID: d.id),
                roleNeedsReconnect: DisplayRole.needsReconnect(displayID: d.id)
            )
        }

        connectedEinkUUIDs = currentEinkUUIDs
        if hasObservedEinkPresence {
            handleEinkPresenceTransition(from: previousHadEink, to: currentlyHasEink)
        } else {
            hasObservedEinkPresence = true
        }
    }

    /// Auto mode is binary across all marked e-ink displays. It turns on at
    /// the empty -> non-empty edge, and turns off only at non-empty -> empty;
    /// swapping or disconnecting one of several panels therefore does nothing.
    private func handleEinkPresenceTransition(from hadEink: Bool, to hasEink: Bool) {
        guard autoAccessibility, helperReady, hadEink != hasEink else { return }
        accessibilityPresenceTask?.cancel()
        if hasEink {
            setAccessibilityEnabled(true)
            return
        }

        // A multi-display reconfiguration can briefly enumerate no external
        // displays while the remaining panel is coming back. Confirm the empty
        // state after it settles so unplugging one of several e-ink panels can
        // never issue a spurious Off.
        accessibilityPresenceTask = Task { @MainActor [weak self] in
            try? await Task.sleep(nanoseconds: 2_000_000_000)
            guard !Task.isCancelled, let self,
                  self.autoAccessibility, self.helperReady else { return }
            let stillHasEink = controllableDisplays().contains {
                EinkSettings.isEink($0.id)
            }
            if !stillHasEink { self.setAccessibilityEnabled(false) }
        }
    }

    /// Re-asserts everything this app owns. Both adjustments are now
    /// app-managed: quitting returns displays to their original state, so
    /// launching has to put the stored settings back.
    func reapplyAll() {
        markSelfChange()
        // Color profile first, then curves. Installing a color profile clears the
        // display's gamma table, so a curve applied before it is silently wiped
        // — which is why settings appeared not to reload at launch.
        for panel in panels where panel.isEink {
            let stored = EinkSettings.saturation(panel.id)
            let rgb = EinkSettings.rgbBalance(panel.id)
            if abs(stored - 1.0) > 0.001 || !rgb.isIdentity {
                _ = try? applySaturation(stored, rgbBalance: rgb,
                                         displayID: panel.id,
                                         displayName: panel.name)
            }
        }
        for panel in panels where panel.isEink {
            reapplyEnhance(displayID: panel.id)
        }
        // Dithering is hardware state that survives across processes and is
        // reset by display reconfiguration, so it is re-asserted here too.
        for panel in panels where panel.isEink {
            Dither.setDisabled(panel.reduceShaking, displayID: panel.id)
        }
        reassertCurvesSoon()
    }

    /// AppKit's didChangeScreenParameters does not fire for every display
    /// reconfiguration. Stillcolor uses the CoreGraphics callback for exactly
    /// this reason, so it is registered here as well.
    private func registerDisplayReconfigurationCallback() {
        let context = Unmanaged.passUnretained(self).toOpaque()
        CGDisplayRegisterReconfigurationCallback({ displayID, flags, userInfo in
            guard let userInfo else { return }
            let relevant: CGDisplayChangeSummaryFlags =
                [.addFlag, .removeFlag, .enabledFlag, .disabledFlag, .setModeFlag,
                 .mirrorFlag, .unMirrorFlag, .desktopShapeChangedFlag]
            guard !flags.intersection(relevant).isEmpty else { return }
            let mirrorChanged = flags.contains(.mirrorFlag)
                || flags.contains(.unMirrorFlag)
            let model = Unmanaged<AssistantModel>.fromOpaque(userInfo).takeUnretainedValue()
            Task { @MainActor in
                // `addFlag` is the one trustworthy signal macOS exposes for
                // this workflow. Clear the instruction after the target
                // display has genuinely disconnected and returned.
                if flags.contains(.addFlag) {
                    DisplayRole.clearReconnectNeeded(displayID: displayID)
                }
                model.scheduleReapply(force: mirrorChanged)
            }
        }, context)
    }

    /// The gamma-table wipe caused by a profile write can land slightly after
    /// the call returns, so the curve is asserted again a moment later rather
    /// than relying on ordering alone.
    private func reassertCurvesSoon() {
        Task { @MainActor [weak self] in
            try? await Task.sleep(nanoseconds: 1_200_000_000)
            guard let self else { return }
            self.markSelfChange()
            for panel in self.panels where panel.isEink {
                reapplyEnhance(displayID: panel.id)
            }
        }
    }

    private func index(of id: CGDirectDisplayID) -> Int? {
        panels.firstIndex { $0.id == id }
    }

    func setEink(_ value: Bool, for id: CGDirectDisplayID) {
        guard let i = index(of: id) else { return }
        // Only act on a real transition. SwiftUI invokes a Binding's setter
        // during the first render, and without this guard that phantom write
        // ran the un-mark cleanup path and wiped the user's saturation.
        guard panels[i].isEink != value else { return }

        let previouslyHadEink = panels.contains { $0.isEink }
        markSelfChange()
        panels[i].isEink = value
        EinkSettings.setEink(value, for: id)
        connectedEinkUUIDs = Set(panels.compactMap { panel in
            guard panel.isEink else { return nil }
            return displayUUIDString(panel.id)
        })
        handleEinkPresenceTransition(from: previouslyHadEink,
                                     to: !connectedEinkUUIDs.isEmpty)

        // Dithering shimmer is the first thing people notice on e-ink, so
        // marking a display turns this on rather than making them find it.
        if value {
            panels[i].reduceShaking = true
            EinkSettings.setReduceShaking(true, for: id)
            Dither.setDisabled(true, displayID: id)
        }

        // Un-marking a display restores it completely: no tone curve, and the
        // factory color profile back in place. Safe to do unconditionally
        // because of the transition guard above — an accidental write can no
        // longer reach this path.
        if !value {
            panels[i].enhance = .off
            panels[i].textLevel = .off
            panels[i].advanced = false
            panels[i].reduceShaking = false
            EinkSettings.setReduceShaking(false, for: id)
            Dither.setDisabled(false, displayID: id)
            EinkSettings.setEnhance(.off, for: id)
            EinkSettings.setTextLevel(.off, for: id)
            EinkSettings.setAdvanced(false, for: id)
            clearToneCurveLive(displayID: id)
            panels[i].saturation = 1.0
            panels[i].saturationPreset = nil
            panels[i].rgbBalance = .identity
            EinkSettings.setSaturation(1.0, for: id)
            EinkSettings.setSaturationPreset(nil, for: id)
            EinkSettings.setRGBBalance(.identity, for: id)
            applyColorProfile(for: i)
        }
    }

    /// Saturation and RGB balance share one ICC profile. Keeping the write in
    /// one path prevents one control from wiping out the other.
    private func applyColorProfile(for index: Int) {
        let panel = panels[index]
        markSelfChange()
        do {
            try applySaturation(panel.saturation,
                                rgbBalance: panel.rgbBalance,
                                displayID: panel.id,
                                displayName: panel.name)
            lastError = nil
        } catch {
            lastError = String(format: L("error.color"), panel.name)
        }
        // A profile write clears the hardware gamma table. Restore any active
        // Text Contrast, Video Enhance, or custom curve afterward.
        reapplyEnhance(displayID: panel.id)
        reassertCurvesSoon()
    }

    func setSaturation(_ amount: Double, presetIndex: Int? = nil,
                       for id: CGDirectDisplayID) {
        guard let i = index(of: id) else { return }
        panels[i].saturation = amount
        panels[i].saturationPreset = presetIndex
        EinkSettings.setSaturation(amount, for: id)
        EinkSettings.setSaturationPreset(presetIndex, for: id)
        applyColorProfile(for: i)
    }

    func setRGBBalance(_ balance: RGBBalance, for id: CGDirectDisplayID) {
        guard let i = index(of: id) else { return }
        panels[i].rgbBalance = balance.clamped
        EinkSettings.setRGBBalance(panels[i].rgbBalance, for: id)
        applyColorProfile(for: i)
    }

    func resetRGBBalance(for id: CGDirectDisplayID) {
        setRGBBalance(.identity, for: id)
    }

    func setEnhance(_ level: EnhanceLevel, for id: CGDirectDisplayID) {
        guard let i = index(of: id) else { return }
        guard panels[i].enhance != level else { return }   // ignore phantom writes
        markSelfChange()
        panels[i].enhance = level
        EinkSettings.setEnhance(level, for: id)
        // One gamma table, and these two pull in opposite directions.
        if level != .off {
            panels[i].textLevel = .off
            EinkSettings.setTextLevel(.off, for: id)
        }
        reapplyEnhance(displayID: id)
    }

    func setTextLevel(_ level: TextLevel, for id: CGDirectDisplayID) {
        guard let i = index(of: id) else { return }
        guard panels[i].textLevel != level else { return }
        markSelfChange()
        panels[i].textLevel = level
        EinkSettings.setTextLevel(level, for: id)
        if level != .off {
            panels[i].enhance = .off
            EinkSettings.setEnhance(.off, for: id)
        }
        reapplyEnhance(displayID: id)
    }

    func setReduceShaking(_ value: Bool, for id: CGDirectDisplayID) {
        guard let i = index(of: id), panels[i].reduceShaking != value else { return }
        panels[i].reduceShaking = value
        EinkSettings.setReduceShaking(value, for: id)
        Dither.setDisabled(value, displayID: id)
    }

    /// Writes the display role override. Unlike everything else here this needs
    /// an administrator password and a display reconnect, so it runs off the
    /// main thread and is never applied implicitly.
    func setTelevision(_ value: Bool, for id: CGDirectDisplayID) {
        guard let i = index(of: id), panels[i].isTelevision != value else { return }
        let previous = panels[i].isTelevision
        panels[i].isTelevision = value          // optimistic, reverted on failure
        Task.detached {
            do {
                try DisplayRole.setTelevision(value, displayID: id)
                await MainActor.run {
                    DisplayRole.markReconnectNeeded(displayID: id)
                    self.refreshRole(for: id)
                }
            } catch {
                await MainActor.run {
                    if let j = self.index(of: id) { self.panels[j].isTelevision = previous }
                    self.lastError = L("role.error")
                }
            }
        }
    }

    private func refreshRole(for id: CGDirectDisplayID) {
        guard let i = index(of: id) else { return }
        panels[i].isTelevision = DisplayRole.isTelevision(displayID: id)
        panels[i].roleNeedsReconnect = DisplayRole.needsReconnect(displayID: id)
        lastError = nil
    }

    func setAdvanced(_ value: Bool, for id: CGDirectDisplayID) {
        guard let i = index(of: id), panels[i].advanced != value else { return }
        markSelfChange()
        panels[i].advanced = value
        EinkSettings.setAdvanced(value, for: id)
        reapplyEnhance(displayID: id)
    }

    func setCustomCurve(_ curve: ToneCurve, for id: CGDirectDisplayID) {
        guard let i = index(of: id), panels[i].custom != curve else { return }
        markSelfChange()
        panels[i].custom = curve
        EinkSettings.setCustomCurve(curve, for: id)
        reapplyEnhance(displayID: id)
    }

    /// Returns the custom curve to no adjustment. Identity resolves to no
    /// curve at all, so the display is left untouched rather than written with
    /// a pointless table.
    func resetCustomCurve(for id: CGDirectDisplayID) {
        setCustomCurve(ToneCurve(knee: 0.90, gamma: 1.0,
                                 blackPoint: 0.0, whitePoint: 1.0), for: id)
    }

    func savePreset(slot: Int, from id: CGDirectDisplayID) {
        guard let i = index(of: id) else { return }
        CurvePresets.save(panels[i].custom, slot: slot)
        presets = CurvePresets.all()
    }

    func applyPreset(slot: Int, to id: CGDirectDisplayID) {
        guard let curve = CurvePresets.curve(slot: slot) else { return }
        setCustomCurve(curve, for: id)
    }

    func renamePreset(slot: Int, to name: String) {
        CurvePresets.setName(name, slot: slot)
        presetNamesVersion += 1
    }

    func clearPreset(slot: Int) {
        CurvePresets.clear(slot: slot)
        presets = CurvePresets.all()
        presetNamesVersion += 1
    }

    /// Reconnects the app to an already-imported helper after an app rebuild.
    /// Running with no input is an intentional no-op in our fixed helper, so
    /// this validates only that helper without inspecting the shortcut library.
    /// A failed probe may be a transient Shortcuts service failure; preserve a
    /// previously verified marker instead of pretending the helper was deleted.
    private func verifyAccessibilityHelperOnLaunch() {
        let wasPreviouslyKnown = helperReady
        helperTask?.cancel()
        helperRunning = true
        helperTask = Task { @MainActor [weak self] in
            let succeeded = await Task.detached(priority: .utility) {
                Shortcuts.verifyInstalledAndWait()
            }.value
            guard let self, !Task.isCancelled else { return }
            self.helperRunning = false

            if succeeded {
                Shortcuts.wasInstalled = true
                self.helperReady = true
                self.helperFailed = false
                if self.autoAccessibility {
                    self.setAccessibilityEnabled(!self.connectedEinkUUIDs.isEmpty)
                }
            } else if !wasPreviouslyKnown {
                self.helperReady = false
            }
        }
    }

    /// Opens the signed helper bundled with the app. Shortcuts requires one
    /// explicit Add Shortcut confirmation; while it is visible, try only our
    /// fixed helper name until the import completes, then run it automatically.
    func installAndEnableAccessibilityHelper() {
        guard Shortcuts.isAvailable, let installer = Shortcuts.installerURL else {
            helperFailed = true
            return
        }

        helperTask?.cancel()
        helperReady = false
        helperFailed = false
        helperInstalling = true
        guard NSWorkspace.shared.open(installer) else {
            helperInstalling = false
            helperFailed = true
            return
        }

        helperTask = Task { @MainActor [weak self] in
            // Import normally takes only a few seconds. Keep the bounded wait
            // long enough for someone to read Apple's confirmation sheet.
            for _ in 0..<45 {
                try? await Task.sleep(nanoseconds: 2_000_000_000)
                guard !Task.isCancelled, let self else { return }
                let succeeded = await Task.detached(priority: .userInitiated) {
                    Shortcuts.verifyInstalledAndWait()
                }.value
                guard !Task.isCancelled else { return }
                if succeeded {
                    Shortcuts.wasInstalled = true
                    self.helperReady = true
                    self.helperInstalling = false
                    self.helperFailed = false
                    try? await Task.sleep(nanoseconds: 700_000_000)
                    self.refreshAccessibility()
                    // The verification run has no input and is intentionally a
                    // no-op. Complete Install & Enable explicitly: manual mode
                    // turns the settings on, while automatic mode immediately
                    // follows the current marked-display connection state.
                    let desired = self.autoAccessibility
                        ? !self.connectedEinkUUIDs.isEmpty
                        : true
                    DispatchQueue.main.async { self.setAccessibilityEnabled(desired) }
                    return
                }
            }
            guard let self, !Task.isCancelled else { return }
            self.helperInstalling = false
            self.helperFailed = true
        }
    }

    /// Sets both options together through the app-owned helper. The Shortcuts
    /// URL scheme is the supported way for another app to pass text input.
    func setAccessibilityEnabled(_ enabled: Bool) {
        helperTask?.cancel()
        helperFailed = false
        pendingAccessibilityState = enabled
        helperRunning = true
        helperTask = Task { @MainActor [weak self] in
            let launched = Shortcuts.run(enabled ? .on : .off)
            guard let self, !Task.isCancelled else { return }
            guard launched else {
                self.helperRunning = false
                self.pendingAccessibilityState = nil
                self.helperFailed = true
                return
            }

            Shortcuts.wasInstalled = true
            self.helperReady = true
            // Accessibility notifications are not synchronous. Poll briefly
            // so the toggle reflects the actual system result, not our intent.
            var matched = false
            for _ in 0..<8 {
                try? await Task.sleep(nanoseconds: 400_000_000)
                guard !Task.isCancelled else { return }
                self.refreshAccessibility()
                matched = enabled
                    ? (self.reduceMotion && self.reduceTransparency)
                    : (!self.reduceMotion && !self.reduceTransparency)
                if matched { break }
            }
            self.helperRunning = false
            // A URL launch cannot report when Shortcuts finishes, and macOS
            // may publish these accessibility flags after our short polling
            // window. The helper was independently verified at launch, so a
            // late settings notification is not an installation failure.
            self.helperFailed = false
            if matched { self.pendingAccessibilityState = nil }

            // Keep reconciling after the progress indicator disappears. This
            // covers slower Shortcuts runs without letting a stale cached value
            // replace the state the user just requested.
            guard !matched else { return }
            for _ in 0..<10 {
                try? await Task.sleep(nanoseconds: 1_000_000_000)
                guard !Task.isCancelled else { return }
                self.refreshAccessibility()
                self.reconcilePendingAccessibilityState()
                if self.pendingAccessibilityState == nil { return }
            }
            self.pendingAccessibilityState = nil
            self.helperFailed = true
        }
    }

    func setAutoAccessibility(_ enabled: Bool) {
        guard autoAccessibility != enabled else { return }
        autoAccessibility = enabled
        UserDefaults.standard.set(enabled, forKey: "accessibility-auto-follow")
        if !enabled { accessibilityPresenceTask?.cancel() }
        if enabled && helperReady {
            setAccessibilityEnabled(!connectedEinkUUIDs.isEmpty)
        }
    }

    func refreshAccessibility() {
        let workspace = NSWorkspace.shared
        // NSWorkspace can retain its old values after Shortcuts changes these
        // settings while our panel remains open. CFPreferences reads the live
        // universal-access domain; fall back to the public API if a key is not
        // present on a future macOS release.
        reduceTransparency = universalAccessFlag(
            "reduceTransparency",
            fallback: workspace.accessibilityDisplayShouldReduceTransparency)
        reduceMotion = universalAccessFlag(
            "reduceMotion",
            fallback: workspace.accessibilityDisplayShouldReduceMotion)
    }

    private func universalAccessFlag(_ key: String, fallback: Bool) -> Bool {
        let domain = "com.apple.universalaccess" as CFString
        CFPreferencesAppSynchronize(domain)
        guard let value = CFPreferencesCopyAppValue(key as CFString, domain)
        else { return fallback }
        if let number = value as? NSNumber { return number.boolValue }
        return fallback
    }

    private func reconcilePendingAccessibilityState() {
        guard let desired = pendingAccessibilityState else { return }
        let matched = desired
            ? (reduceMotion && reduceTransparency)
            : (!reduceMotion && !reduceTransparency)
        if matched {
            pendingAccessibilityState = nil
            helperFailed = false
        }
    }

    /// Opens Accessibility > Display, where the two settings actually live.
    func openAccessibilitySettings() {
        let url = URL(string:
            "x-apple.systempreferences:com.apple.preference.universalaccess?Seeing_Display")
        if let url { NSWorkspace.shared.open(url) }
    }

    func setLanguage(_ value: AppLanguage) {
        guard language != value else { return }
        Localization.set(value)
        language = value          // republishes, so every L() re-resolves
    }

    func dismissHardwareSetupNotice(permanently: Bool) {
        if permanently {
            UserDefaults.standard.set(
                true, forKey: AssistantModel.hardwareNoticeSuppressKey)
        }
        showsHardwareSetupNotice = false
    }

    func setLaunchAtLogin(_ enabled: Bool) {
        do {
            if enabled {
                if SMAppService.mainApp.status == .enabled {
                    try? SMAppService.mainApp.unregister()
                }
                try SMAppService.mainApp.register()
            } else {
                try SMAppService.mainApp.unregister()
            }
            lastError = nil
        } catch {
            lastError = L("error.login")
        }
        launchAtLogin = SMAppService.mainApp.status == .enabled
    }

    /// Leaves every display as we found it.
    func restoreAll() {
        for panel in panels where panel.isEink {
            clearToneCurveLive(displayID: panel.id)
        }
    }
}

// MARK: - Panel row

struct PanelRow: View {
    @ObservedObject var model: AssistantModel
    // A binding, not a copy. A captured struct keeps reporting its
    // creation-time value during a drag, so the slider springs back.
    @Binding var panel: PanelState
    @State private var showShakingInfo = false
    @State private var rgbExpanded = false
    @State private var renamingSlot: Int?
    @State private var renameDraft = ""

    // 100% is the "off" shortcut — it drops the profile override entirely
    // rather than installing an identity one.
    // Named presets. 100% is the "off" shortcut: it drops the profile override
    // rather than installing an identity one.
    private let presets: [(value: Double, key: String)] = [
        (0.0, "preset.bw"),      (0.5, "preset.faded"),
        (1.0, "preset.factory"), (1.3, "preset.enhanced"),
        (1.5, "preset.vivid"),   (2.0, "preset.anime"),
    ]
    private let levels: [EnhanceLevel] = [.off, .subtle, .medium, .strong]
    private let textLevels: [TextLevel] = [.off, .medium, .strong, .sharp, .solid]

    /// Advanced overrides the presets, then Text over Video: the same order
    /// effectiveCurve() uses when writing the gamma table.
    private var activeCurve: ToneCurve? {
        if panel.advanced { return panel.custom.isIdentity ? nil : panel.custom }
        return panel.textLevel.curve ?? panel.enhance.curve
    }

    private var activeModeName: String {
        if panel.advanced { return L("advanced.title") }
        if panel.textLevel != .off { return L("text.title") }
        if panel.enhance != .off { return L("video.short") }
        return L("level.off")
    }

    /// What the plot draws. With nothing applied this is the identity curve,
    /// which is worth showing: a flat diagonal says "no adjustment" more
    /// clearly than an empty space where a graph used to be.
    private var displayedCurve: ToneCurve {
        activeCurve ?? ToneCurve(knee: 0.90, gamma: 1.0)
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Toggle(isOn: Binding(
                get: { panel.isEink },
                set: { model.setEink($0, for: panel.id) }
            )) {
                HStack(spacing: 5) {
                    Image(systemName: panel.isBuiltin ? "laptopcomputer" : "display")
                        .foregroundStyle(EinkPalette.secondaryText)
                    Text(panel.name).lineLimit(1).truncationMode(.tail)
                    if panel.isBuiltin {
                        Text(L("display.builtin")).font(.system(size: 14)).foregroundStyle(EinkPalette.secondaryText)
                    }
                }
            }
            .font(.system(size: 16, weight: .semibold))
            .toggleStyle(EinkCheckboxToggleStyle())

            if panel.isEink {
                VStack(alignment: .leading, spacing: 8) {
                    shakingSection
                    if panel.shakingSupported { controlGroupDivider }
                    roleSection
                    controlGroupDivider
                    saturationSection
                    controlGroupDivider
                    rgbSection
                    controlGroupDivider
                    if !panel.advanced {
                        textSection
                        controlGroupDivider
                        enhanceSection
                        controlGroupDivider
                    }
                    advancedSection
                    controlGroupDivider
                    curveSection
                }
            }
        }
        .padding(14)
        .einkOutlinedArea()
    }

    /// A true black, single-point rule stays visible on reflective displays
    /// without competing with the heavier outline around the whole monitor.
    private var controlGroupDivider: some View {
        Rectangle()
            .fill(EinkPalette.areaOutline)
            .frame(maxWidth: .infinity)
            .frame(height: 1)
            .accessibilityHidden(true)
    }

    private var saturationSection: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack {
                Text(L("saturation.title")).font(.system(size: 15, weight: .semibold))
                Spacer()
                Text("\(Int((panel.saturation * 100).rounded()))%")
                    .font(.system(size: 20, weight: .semibold))
                    .monospacedDigit()
            }
            EinkSlider(
                value: $panel.saturation,
                in: 0.0...3.0,
                accessibilityLabel: L("saturation.title"),
                // Each change rewrites a display profile, so commit on release.
                onEditingChanged: { editing in
                    if editing { panel.saturationPreset = nil }
                    if !editing { model.setSaturation(panel.saturation, for: panel.id) }
                }
            )
            HStack(spacing: 6) {
                ForEach(0..<presets.count, id: \.self) { i in
                    let preset = presets[i]
                    let selected = panel.saturationPreset == i
                    Button(L(preset.key)) {
                        model.setSaturation(preset.value, presetIndex: i,
                                            for: panel.id)
                    }
                    .buttonStyle(EinkOutlinedButtonStyle(compact: true))
                    .font(.system(size: 14,
                                  weight: selected ? .black : .regular))
                    .foregroundStyle(Color.primary)
                    .overlay(alignment: .bottom) {
                        if selected {
                            Rectangle()
                                .fill(Color.primary)
                                .frame(height: 3)
                                .padding(.horizontal, 3)
                        }
                    }
                }
            }
        }
    }

    private var rgbSection: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack(spacing: 8) {
                Button {
                    rgbExpanded.toggle()
                } label: {
                    HStack(spacing: 6) {
                        Image(systemName: rgbExpanded ? "chevron.down" : "chevron.right")
                            .font(.system(size: 12, weight: .black))
                        Text(L("rgb.title"))
                    }
                }
                .buttonStyle(EinkOutlinedButtonStyle(compact: true))
                .font(.system(size: 15, weight: .semibold))
                .accessibilityLabel(L(rgbExpanded ? "rgb.collapse" : "rgb.expand"))

                Spacer()

                if rgbExpanded {
                    Button(L("rgb.reset")) {
                        model.resetRGBBalance(for: panel.id)
                    }
                    .buttonStyle(EinkOutlinedButtonStyle(compact: true))
                    .disabled(panel.rgbBalance.isIdentity)
                } else {
                    Text(rgbSummary)
                        .font(.system(size: 13, weight: .semibold))
                        .monospacedDigit()
                        .lineLimit(1)
                }
            }

            if rgbExpanded {
                rgbSlider(L("rgb.red"), value: Binding(
                    get: { panel.rgbBalance.red },
                    set: { panel.rgbBalance.red = $0 }
                ), color: .red)
                rgbSlider(L("rgb.green"), value: Binding(
                    get: { panel.rgbBalance.green },
                    set: { panel.rgbBalance.green = $0 }
                ), color: .green)
                rgbSlider(L("rgb.blue"), value: Binding(
                    get: { panel.rgbBalance.blue },
                    set: { panel.rgbBalance.blue = $0 }
                ), color: .blue)
            }
        }
    }

    private var rgbSummary: String {
        let rgb = panel.rgbBalance
        return "R \(Int((rgb.red * 100).rounded()))%  "
            + "G \(Int((rgb.green * 100).rounded()))%  "
            + "B \(Int((rgb.blue * 100).rounded()))%"
    }

    private func rgbSlider(_ title: String, value: Binding<Double>,
                           color: Color) -> some View {
        HStack(spacing: 8) {
            Text(title)
                .font(.system(size: 13, weight: .semibold))
                .frame(width: 48, alignment: .leading)
            EinkSlider(
                value: value,
                in: 0.0...2.0,
                accessibilityLabel: title,
                accentColor: color,
                onEditingChanged: { editing in
                    if !editing {
                        model.setRGBBalance(panel.rgbBalance, for: panel.id)
                    }
                }
            )
            .frame(height: 26)
            Text("\(Int((value.wrappedValue * 100).rounded()))%")
                .font(.system(size: 13, weight: .semibold))
                .monospacedDigit()
                .frame(width: 42, alignment: .trailing)
        }
    }

    /// The tone curve currently applied, drawn the same way the tuning labs
    /// draw it. Always shown, so the graph does not vanish when a mode is off
    /// or the advanced curve is reset.
    private var curveSection: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack {
                Text(L("curve.title")).font(.system(size: 14, weight: .semibold))
                Spacer()
                Text(activeModeName)
                    .font(.system(size: 13)).foregroundStyle(EinkPalette.secondaryText)
            }
            CurvePlot(curve: displayedCurve, height: 96)
        }
    }

    /// Night Shift and True Tone are withheld by macOS from displays it treats
    /// as televisions, so the role override is how they get turned off for one
    /// display. Separate from everything else because it needs a password and a
    /// display reconnect, and is not undone on quit.
    private var roleSection: some View {
        VStack(alignment: .leading, spacing: 4) {
            let roleTitle = String(format: L("role.title"), panel.name)
            HStack(alignment: .top, spacing: 8) {
                Text(roleTitle)
                    .font(.system(size: 15, weight: .semibold))
                    .fixedSize(horizontal: false, vertical: true)
                Spacer(minLength: 12)
                Toggle(isOn: Binding(
                    get: { panel.isTelevision },
                    set: { model.setTelevision($0, for: panel.id) }
                )) { EmptyView() }
                .toggleStyle(EinkSwitchToggleStyle(showsLabel: false))
                .accessibilityLabel(roleTitle)
            }

            if panel.roleNeedsReconnect {
                Label(L("role.restart"), systemImage: "arrow.clockwise.circle")
                    .font(.system(size: 13))
                    .foregroundStyle(Color.orange)
            }
            Text(L("role.note"))
                .font(.system(size: 13)).foregroundStyle(EinkPalette.secondaryText)
                .fixedSize(horizontal: false, vertical: true)
        }
    }

    /// Dithering control. Hidden when no framebuffer could be matched, rather
    /// than offering a toggle that would do nothing.
    @ViewBuilder private var shakingSection: some View {
        if panel.shakingSupported {
            HStack(spacing: 6) {
                Text(L("shaking.title")).font(.system(size: 15, weight: .semibold))
                // A hover tooltip never fires here: .help() relies on
                // NSView.toolTip, which needs an active window, and a menu bar
                // panel is non-activating. A popover works in a panel.
                Button { showShakingInfo.toggle() } label: {
                    Image(systemName: "info.circle")
                        .font(.system(size: 15))
                        .foregroundStyle(EinkPalette.secondaryText)
                }
                .buttonStyle(EinkOutlinedButtonStyle(compact: true))
                .contentShape(Rectangle())
                .popover(isPresented: $showShakingInfo, arrowEdge: .bottom) {
                    Text(L("shaking.info"))
                        .font(.system(size: 14))
                        .fixedSize(horizontal: false, vertical: true)
                        .frame(width: 320)
                        .padding(14)
                }
                Spacer()
                Toggle(isOn: Binding(
                    get: { panel.reduceShaking },
                    set: { model.setReduceShaking($0, for: panel.id) }
                )) { EmptyView() }
                .toggleStyle(EinkSwitchToggleStyle(showsLabel: false))
                .accessibilityLabel(L("shaking.title"))
            }
        }
    }

    /// Full manual control of the curve, replacing the preset pickers.
    @ViewBuilder private var advancedSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack(spacing: 8) {
                Text(L("advanced.title")).font(.system(size: 15, weight: .semibold))
                Spacer(minLength: 12)
                Toggle(isOn: Binding(
                    get: { panel.advanced },
                    set: { model.setAdvanced($0, for: panel.id) }
                )) { EmptyView() }
                .toggleStyle(EinkSwitchToggleStyle(showsLabel: false))
                .accessibilityLabel(L("advanced.title"))
            }

            if panel.advanced {
                Text(L("advanced.note"))
                    .font(.system(size: 13)).foregroundStyle(EinkPalette.secondaryText)
                slider(L("curve.knee"), value: panel.custom.knee, range: 0.05...1.00) {
                    var c = panel.custom; c.knee = $0
                    model.setCustomCurve(c, for: panel.id)
                }
                slider(L("curve.gamma"), value: panel.custom.gamma, range: 0.30...6.00) {
                    var c = panel.custom; c.gamma = $0
                    model.setCustomCurve(c, for: panel.id)
                }
                slider(L("curve.black"), value: panel.custom.blackPoint, range: 0.00...0.40) {
                    var c = panel.custom; c.blackPoint = $0
                    model.setCustomCurve(c, for: panel.id)
                }
                slider(L("curve.white"), value: panel.custom.whitePoint, range: 0.60...1.00) {
                    var c = panel.custom; c.whitePoint = $0
                    model.setCustomCurve(c, for: panel.id)
                }
                HStack {
                    Spacer()
                    Button(L("advanced.reset")) {
                        model.resetCustomCurve(for: panel.id)
                    }
                    .buttonStyle(EinkOutlinedButtonStyle(compact: true))
                    .disabled(panel.custom.isIdentity)
                }
                presetSlots
            }
        }
    }

    /// Five slots for storing curves. Empty slots save, filled slots apply,
    /// and a context menu renames, overwrites or clears. Renaming happens
    /// inline: a sheet or alert would be heavy inside a menu bar panel.
    private var presetSlots: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(L("presets.title")).font(.system(size: 13, weight: .semibold))
            HStack(spacing: 5) {
                ForEach(0..<CurvePresets.slotCount, id: \.self) { slot in
                    slotView(slot)
                }
            }
            Text(L("presets.hint"))
                .font(.system(size: 12)).foregroundStyle(EinkPalette.secondaryText)
                .fixedSize(horizontal: false, vertical: true)
        }
    }

    @ViewBuilder private func slotView(_ slot: Int) -> some View {
        let saved = model.presets[slot]
        if renamingSlot == slot {
            TextField("", text: $renameDraft)
                .textFieldStyle(.roundedBorder)
                .font(.system(size: 13))
                .frame(width: 76)
                .einkOutlinedArea(cornerRadius: 5)
                .onSubmit {
                    model.renamePreset(slot: slot, to: renameDraft)
                    renamingSlot = nil
                }
        } else {
            Button {
                if saved == nil {
                    model.savePreset(slot: slot, from: panel.id)
                } else {
                    model.applyPreset(slot: slot, to: panel.id)
                }
            } label: {
                Text(saved == nil ? "+" : CurvePresets.label(slot: slot))
                    .font(.system(size: 13, weight: saved == nil ? .regular : .semibold))
                    .foregroundStyle(Color.primary)
                    .lineLimit(1)
                    .truncationMode(.tail)
                    .frame(minWidth: 22, maxWidth: 76)
            }
            .buttonStyle(EinkOutlinedButtonStyle(compact: true))
            .help(saved.map { CurvePresets.summary($0) } ?? L("presets.hint"))
            .contextMenu {
                if saved != nil {
                    Button(L("presets.rename")) {
                        renameDraft = CurvePresets.name(slot: slot) ?? ""
                        renamingSlot = slot
                    }
                    Button(L("presets.overwrite")) {
                        model.savePreset(slot: slot, from: panel.id)
                    }
                    Button(L("presets.clear")) { model.clearPreset(slot: slot) }
                }
            }
        }
    }

    private func slider(_ title: String, value: Double,
                        range: ClosedRange<Double>,
                        set: @escaping (Double) -> Void) -> some View {
        VStack(alignment: .leading, spacing: 1) {
            HStack {
                Text(title).font(.system(size: 13))
                Spacer()
                Text(String(format: "%.2f", value))
                    .font(.system(size: 13)).monospacedDigit()
                    .foregroundStyle(EinkPalette.secondaryText)
            }
            EinkSlider(value: Binding(get: { value }, set: set),
                       in: range,
                       accessibilityLabel: title)
        }
    }

    private var textSection: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(L("text.title")).font(.system(size: 15, weight: .semibold))
            segmentedSelector(
                textLevels,
                selection: panel.textLevel,
                label: { $0.label },
                set: { model.setTextLevel($0, for: panel.id) }
            )
        }
    }

    private var enhanceSection: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(L("video.title")).font(.system(size: 15, weight: .semibold))
            segmentedSelector(
                levels,
                selection: panel.enhance,
                label: { $0.label },
                set: { model.setEnhance($0, for: panel.id) }
            )

            if let cost = panel.enhance.textContrastCost {
                Label(String(format: L("video.warning"), cost),
                      systemImage: "exclamationmark.triangle.fill")
                    .font(.system(size: 13))
                    .foregroundStyle(Color.orange)
                    .fixedSize(horizontal: false, vertical: true)
            } else if panel.textLevel != .off {
                Text(L("video.blocked"))
                    .font(.system(size: 13)).foregroundStyle(EinkPalette.secondaryText)
                    .fixedSize(horizontal: false, vertical: true)
            }
        }
    }

    /// Native segmented controls dim unselected labels, which makes them fade
    /// on e-ink. This selector keeps every label at full contrast and conveys
    /// selection with weight and a solid underline instead.
    private func segmentedSelector<Option: Hashable>(
        _ options: [Option],
        selection: Option,
        label: @escaping (Option) -> String,
        set: @escaping (Option) -> Void
    ) -> some View {
        HStack(spacing: 0) {
            ForEach(options.indices, id: \.self) { index in
                let option = options[index]
                let selected = option == selection
                Button {
                    set(option)
                } label: {
                    Text(label(option))
                        .font(.system(size: 13,
                                      weight: selected ? .black : .regular))
                        .foregroundStyle(Color.primary)
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, 4)
                        .contentShape(Rectangle())
                }
                .buttonStyle(.plain)
                .overlay(alignment: .bottom) {
                    if selected {
                        Rectangle().fill(Color.primary).frame(height: 3)
                    }
                }

                if index < options.count - 1 {
                    Rectangle()
                        .fill(Color.primary)
                        .frame(width: 1)
                }
            }
        }
        .einkOutlinedArea(cornerRadius: 5)
    }
}

// MARK: - Explainer

/// A disclosure triangle here read as if the notice below it were part of this
/// section, so the control is an explicit Show more / Show less button.
struct HowItWorks: View {
    /// `L()` reads a runtime-selected bundle rather than a SwiftUI environment
    /// value. Keep the selected language as an explicit input so SwiftUI knows
    /// this otherwise self-contained child must redraw after a language switch.
    let language: AppLanguage
    @State private var expanded = false

    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack(spacing: 8) {
                Text(L("help.title")).font(.system(size: 16, weight: .semibold))
                Button(expanded ? L("help.less") : L("help.more")) {
                    expanded.toggle()
                }
                .buttonStyle(EinkOutlinedButtonStyle(
                    foreground: .accentColor, compact: true))
                .font(.system(size: 15))
                Spacer()
            }

            if expanded {
                VStack(alignment: .leading, spacing: 6) {
                    Text(L("help.saturation"))
                    Text(L("help.video"))
                    Text(L("help.tradeoff"))
                    Text(L("help.volatile"))
                }
                .font(.system(size: 15))
                .foregroundStyle(EinkPalette.secondaryText)
                .fixedSize(horizontal: false, vertical: true)
            }
        }
    }
}

/// System-wide accessibility settings that help on e-ink, shown above the
/// per-display controls because they apply to the whole Mac.
private struct AccessibilityInstallGuide: View {
    @Environment(\.dismiss) private var dismiss
    let install: () -> Void

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text(L("system.install.title"))
                .font(.system(size: 21, weight: .semibold))
            Text(L("system.install.intro"))
                .fixedSize(horizontal: false, vertical: true)

            VStack(alignment: .leading, spacing: 10) {
                guideRow("1.circle", L("system.install.step1"))
                guideRow("2.circle", L("system.install.step2"))
                guideRow("switch.2", L("system.install.future"))
                guideRow("display.2", L("system.install.auto"))
                guideRow("power", L("system.install.quit"))
            }

            Text(L("system.install.scope"))
                .font(.system(size: 14))
                .foregroundStyle(EinkPalette.secondaryText)
                .fixedSize(horizontal: false, vertical: true)

            HStack {
                Spacer()
                Button(L("system.install.cancel")) { dismiss() }
                    .buttonStyle(EinkOutlinedButtonStyle())
                Button(L("system.install.continue")) {
                    dismiss()
                    DispatchQueue.main.asyncAfter(deadline: .now() + 0.25) { install() }
                }
                .buttonStyle(EinkOutlinedButtonStyle(foreground: .accentColor))
                .keyboardShortcut(.defaultAction)
            }
        }
        .font(.system(size: 16))
        .padding(24)
        .frame(width: 480)
    }

    private func guideRow(_ symbol: String, _ text: String) -> some View {
        HStack(alignment: .top, spacing: 10) {
            Image(systemName: symbol)
                .foregroundStyle(EinkPalette.secondaryText)
                .frame(width: 20)
            Text(text).fixedSize(horizontal: false, vertical: true)
        }
    }
}

struct SystemDisplayRow: View {
    @ObservedObject var model: AssistantModel
    @State private var showInstallGuide = false

    private var bothOn: Bool { model.reduceTransparency && model.reduceMotion }
    private var eitherOn: Bool { model.reduceTransparency || model.reduceMotion }
    private var displayedOn: Bool { model.pendingAccessibilityState ?? eitherOn }
    private var status: String {
        if let pending = model.pendingAccessibilityState {
            return pending ? L("system.on") : L("system.off")
        }
        if bothOn { return L("system.on") }
        if eitherOn { return L("system.partial") }
        return L("system.off")
    }

    private var busy: Bool { model.helperInstalling || model.helperRunning }

    /// A static, high-contrast activity mark.  Native spinning indicators are
    /// faint and continuously redraw, which is especially poor on e-ink.
    private var einkBusyIndicator: some View {
        Image(systemName: "hourglass")
            .font(.system(size: 16, weight: .bold))
            .foregroundStyle(Color.accentColor)
            .frame(width: 38, height: 30)
            .einkOutlinedArea(cornerRadius: 5)
            .accessibilityLabel(detail)
    }

    private var detail: String {
        if model.helperInstalling { return L("system.installing") }
        if model.helperRunning { return L("system.running") }
        if model.helperFailed { return L("system.failed") }
        if bothOn { return L("system.hint") }
        if model.helperReady { return L("system.ready") }
        return L("system.guide")
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack(spacing: 8) {
                Text(L("system.title")).font(.system(size: 15, weight: .semibold))
                Text(status).font(.system(size: 14)).foregroundStyle(EinkPalette.secondaryText)
                Spacer()
                if busy {
                    einkBusyIndicator
                } else if model.helperReady {
                    Toggle(L("system.toggle"), isOn: Binding(
                        get: { displayedOn },
                        set: { model.setAccessibilityEnabled($0) }
                    ))
                    .toggleStyle(EinkSwitchToggleStyle(showsLabel: false))
                    .accessibilityLabel(L("system.toggle"))
                } else {
                    Button(L("system.install")) {
                        showInstallGuide = true
                    }
                    .font(.system(size: 13, weight: .semibold))
                    .buttonStyle(.plain)
                    .padding(.horizontal, 7)
                    .frame(height: 30)
                    .einkOutlinedArea(cornerRadius: 5)
                    .fixedSize()
                }
                Menu {
                    Button(L("system.open")) { model.openAccessibilitySettings() }
                    if model.helperReady || model.helperFailed {
                        Divider()
                        Button(L("system.reinstall")) {
                            showInstallGuide = true
                        }
                    }
                } label: {
                    Text(L("system.more"))
                        .font(.system(size: 13))
                }
                .menuStyle(.borderlessButton)
                .padding(.horizontal, 6)
                // The switch keeps a 30-point hit target around a 24-point
                // visible track. Match both dimensions here: 24-point outline
                // centered inside a 30-point row target.
                .fixedSize()
                .frame(height: 24)
                .contentShape(RoundedRectangle(cornerRadius: 5))
                .einkOutlinedArea(cornerRadius: 5)
                .frame(height: 30)
            }
            // Keep guidance for installation and genuine failures, but avoid
            // repeating self-evident instructions once the helper is ready.
            if !model.helperReady || model.helperInstalling || model.helperFailed {
                Text(detail)
                    .font(.system(size: 13)).foregroundStyle(EinkPalette.secondaryText)
                    .fixedSize(horizontal: false, vertical: true)
            }
            if model.helperReady {
                HStack(spacing: 8) {
                    Text(L("system.auto")).font(.system(size: 14))
                    Spacer(minLength: 12)
                    Toggle(isOn: Binding(
                        get: { model.autoAccessibility },
                        set: { model.setAutoAccessibility($0) }
                    )) { EmptyView() }
                    .toggleStyle(EinkSwitchToggleStyle(showsLabel: false))
                    .accessibilityLabel(L("system.auto"))
                }
            }
        }
        .onAppear { model.refreshAccessibility() }
        .sheet(isPresented: $showInstallGuide) {
            AccessibilityInstallGuide {
                model.installAndEnableAccessibilityHelper()
            }
        }
    }
}

// MARK: - Main view

/// Hardware contrast that starts too high leaves the software curve with no
/// recoverable shade detail. Keep this reminder before the system-wide
/// accessibility controls so it is the first setup step below the app title.
private struct HardwareSetupNotice: View {
    @ObservedObject var model: AssistantModel

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack(alignment: .top, spacing: 9) {
                Image(systemName: "display")
                    .font(.system(size: 18))
                    .foregroundStyle(Color.accentColor)
                    .frame(width: 20)
                VStack(alignment: .leading, spacing: 5) {
                    Text(L("hardware.notice.title"))
                        .font(.system(size: 16, weight: .semibold))
                    Text((try? AttributedString(
                        markdown: L("hardware.notice.body")))
                        ?? AttributedString(L("hardware.notice.body")))
                        .lineLimit(2)
                        .minimumScaleFactor(0.88)
                        .allowsTightening(true)
                    Text(L("hardware.notice.bigme"))
                }
                .fixedSize(horizontal: false, vertical: true)
            }

            HStack(spacing: 9) {
                Spacer()
                Button(L("hardware.notice.gotIt")) {
                    model.dismissHardwareSetupNotice(permanently: false)
                }
                .buttonStyle(EinkOutlinedButtonStyle(foreground: .accentColor))
                .keyboardShortcut(.defaultAction)
                Button(L("hardware.notice.never")) {
                    model.dismissHardwareSetupNotice(permanently: true)
                }
                .buttonStyle(EinkOutlinedButtonStyle())
            }
        }
        .padding(14)
        .einkOutlinedArea()
    }
}

struct AssistantView: View {
    @ObservedObject var model: AssistantModel

    var body: some View {
        VStack(alignment: .leading, spacing: 18) {
            if model.showsHardwareSetupNotice {
                HardwareSetupNotice(model: model)
            }

            SystemDisplayRow(model: model)

            Divider()

            if model.panels.isEmpty {
                Text(L("display.none")).foregroundStyle(EinkPalette.secondaryText)
            } else {
                Text(L("display.mark"))
                    .font(.system(size: 14)).foregroundStyle(EinkPalette.secondaryText)
                ForEach($model.panels) { $panel in
                    PanelRow(model: model, panel: $panel)
                }
            }

            if let error = model.lastError {
                Text(error).font(.system(size: 14)).foregroundStyle(.red)
            }

            HowItWorks(language: model.language)

            Divider()

            HStack {
                Text(L("language.title")).font(.system(size: 15))
                Spacer()
                Menu {
                    ForEach(0..<AppLanguage.allCases.count, id: \.self) { i in
                        let lang = AppLanguage.allCases[i]
                        Button(lang.label) { model.setLanguage(lang) }
                    }
                } label: {
                    Text(model.language.label)
                }
                .menuStyle(.borderlessButton)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .einkOutlinedArea(cornerRadius: 5)
                .fixedSize()
            }

            HStack(spacing: 8) {
                Text(L("login.toggle")).font(.system(size: 15))
                Spacer(minLength: 12)
                Toggle(isOn: Binding(
                    get: { model.launchAtLogin },
                    set: { model.setLaunchAtLogin($0) }
                )) { EmptyView() }
                .toggleStyle(EinkSwitchToggleStyle(showsLabel: false))
                .accessibilityLabel(L("login.toggle"))
            }

        }
        .padding(16)
    }
}

/// Reports how tall the panel content actually is.
private struct ContentHeightKey: PreferenceKey {
    static let defaultValue: CGFloat = 0
    static func reduce(value: inout CGFloat, nextValue: () -> CGFloat) {
        value = max(value, nextValue())
    }
}

/// Wraps the panel so a machine with several displays scrolls instead of
/// growing a window taller than the screen.
///
/// A ScrollView has no intrinsic height: given only a maxHeight it collapses to
/// a sliver, because nothing tells it how big its content is. So the content is
/// measured and the window is sized to that, capped to the status item's
/// current screen so it starts scrolling rather than running off the screen.
struct AssistantScroll: View {
    @ObservedObject var model: AssistantModel
    let maximumHeight: CGFloat
    @State private var contentHeight: CGFloat = 420

    private static let minHeight: CGFloat = 160

    /// Height of the pinned header, which the content measurement excludes.
    private static let headerHeight: CGFloat = 48

    private var clampedHeight: CGFloat {
        min(max(contentHeight + Self.headerHeight, Self.minHeight),
            max(maximumHeight, Self.minHeight))
    }

    var body: some View {
        VStack(spacing: 0) {
            // Pinned header: Quit stays at the window's top right instead of
            // scrolling away with the content.
            HStack {
                Text(L("app.title")).font(.system(size: 17, weight: .semibold))
                Text("v" + (Bundle.main.object(forInfoDictionaryKey:
                        "CFBundleShortVersionString") as? String ?? "?"))
                    .font(.system(size: 13))
                    .foregroundStyle(EinkPalette.secondaryText)
                Spacer()
                Button(L("quit")) { NSApp.terminate(nil) }
                    .font(.system(size: 15))
                    .buttonStyle(EinkOutlinedButtonStyle(compact: true))
            }
            .padding(.horizontal, 16)
            .padding(.top, 14)
            .padding(.bottom, 10)

            Divider()

            ScrollView(.vertical) {
                AssistantView(model: model)
                .background(
                        GeometryReader { geometry in
                            Color.clear.preference(key: ContentHeightKey.self,
                                                   value: geometry.size.height)
                        }
                    )
            }
        }
        .font(.system(size: 15))
        .onPreferenceChange(ContentHeightKey.self) { height in
            // Guard against a zero measurement collapsing the window again.
            if height > 1 { contentHeight = height }
        }
        // minWidth cannot be combined with a fixed height, so the height is
        // pinned by giving min and max the same value.
        .frame(width: 540, height: clampedHeight)
    }
}

// MARK: - App

@MainActor
final class AssistantDelegate: NSObject, NSApplicationDelegate {
    /// SwiftUI's NSApplicationDelegateAdaptor does not put this instance in
    /// NSApp.delegate, so looking it up there silently returns nil. Everything
    /// that needs the delegate goes through here instead.
    static private(set) var shared: AssistantDelegate?

    var model: AssistantModel?
    private var statusItem: NSStatusItem?
    private let bubble = BubbleWindow(closesOnOutsideClick: true, activatesApp: true)
    /// Guards against the outside-click monitor closing the bubble on mouse
    /// down and the button action reopening it on mouse up.
    private var lastPanelClose = Date.distantPast

    /// The menu bar button, used to anchor both the panel and the first-run tip.
    var statusButton: NSStatusBarButton? { statusItem?.button }

    var isPanelOpen: Bool { bubble.isVisible }

    private func installStatusItem() {
        let item = NSStatusBar.system.statusItem(withLength: NSStatusItem.squareLength)
        item.button?.image = NSImage(systemSymbolName: "book.pages",
                                     accessibilityDescription: L("app.title"))
        item.button?.target = self
        item.button?.action = #selector(togglePanel)
        statusItem = item
    }

    @objc func togglePanel() {
        if bubble.isVisible {
            closePanel()
        } else if Date().timeIntervalSince(lastPanelClose) > 0.25 {
            openPanel()
        }
    }

    /// Opens the panel, also used by the first-run tip's hand-off.
    func openPanel() {
        WelcomeWindow.close()
        let button = statusItem?.button
        let maximumHeight = bubble.maximumContentHeight(from: button)
        bubble.show(from: button) {
            AssistantScroll(model: .shared, maximumHeight: maximumHeight)
        }
    }

    func closePanel() {
        bubble.close()
        lastPanelClose = Date()
    }

    func applicationDidFinishLaunching(_ note: Notification) {
        AssistantDelegate.shared = self
        NSApp.setActivationPolicy(.accessory)
        model = .shared
        installStatusItem()
        // After the status item exists, so the tip's arrow has something to
        // point at.
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.4) {
            WelcomeWindow.showIfNeeded()
        }
        NSWorkspace.shared.notificationCenter.addObserver(
            forName: NSWorkspace.willPowerOffNotification,
            object: nil, queue: .main
        ) { _ in
            MainActor.assumeIsolated {
                if Shortcuts.wasInstalled { _ = Shortcuts.run(.off) }
                restoreAllDisplaysToneCurves()
                restoreAllDisplaysSaturation()
                Dither.restoreAll()
            }
        }
    }

    /// This is an accessory/menu-bar app, so reopening it from Finder or
    /// Spotlight has no Dock window for macOS to raise.  Open the panel
    /// explicitly instead of appearing to do nothing.
    func applicationShouldHandleReopen(_ sender: NSApplication,
                                       hasVisibleWindows flag: Bool) -> Bool {
        openPanel()
        return true
    }

    func applicationOpenUntitledFile(_ sender: NSApplication) -> Bool {
        openPanel()
        return true
    }

    // Video Enhance lives in a volatile gamma table; don't leave it applied
    // with nothing running to maintain or undo it. This must not depend on the
    // UI model — that is only wired up once the menu bar panel is opened, so
    // quitting without opening it used to leave the curve in place.
    func applicationWillTerminate(_ note: Notification) {
        // These accessibility options are system-wide rather than per display;
        // never leave them enabled on the app's behalf after it exits.
        if Shortcuts.wasInstalled { _ = Shortcuts.run(.off) }
        restoreAllDisplaysToneCurves()
        restoreAllDisplaysSaturation()
        Dither.restoreAll()
    }
}

@main
struct EinkAssistantApp: App {
    @NSApplicationDelegateAdaptor(AssistantDelegate.self) var delegate

    var body: some Scene {
        // The menu bar item is created by the delegate; SwiftUI's MenuBarExtra
        // cannot be opened programmatically, which the first-run tip needs.
        Settings { EmptyView() }
    }
}
