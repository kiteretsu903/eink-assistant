// E-Ink Assistant — display tuning for colour e-ink panels.
//
// Two adjustments, deliberately different in how they persist:
//
//   Saturation     written into the display's ICC profile. macOS re-applies it
//                  at login, so it survives quitting this app entirely.
//   Video Enhance  driven through the display's gamma table, which macOS resets
//                  on sleep and display changes. That needs the app running to
//                  be re-applied, which is why it is framed as a mode you turn
//                  on and off rather than a setting.

import SwiftUI
import AppKit
import CoreGraphics
import ServiceManagement


// MARK: - Model

struct PanelState: Identifiable, Equatable {
    let id: CGDirectDisplayID
    let name: String
    let isBuiltin: Bool
    var isEink: Bool
    var saturation: Double
    var enhance: EnhanceLevel
    var textLevel: TextLevel
    var advanced: Bool
    var custom: ToneCurve
    var reduceShaking: Bool
    var shakingSupported: Bool
    var isTelevision: Bool
    var roleNeedsRestart: Bool
}

@MainActor
final class AssistantModel: ObservableObject {
    @Published var panels: [PanelState] = []
    @Published var launchAtLogin = false
    @Published var language: AppLanguage = Localization.current
    @Published var presets: [ToneCurve?] = CurvePresets.all()
    /// Bumped on rename so rows redraw; names live in CurvePresets.
    @Published var presetNamesVersion = 0
    @Published var lastError: String?

    // Writing a gamma table or a colour profile makes macOS post a storm of
    // didChangeScreenParameters notifications — measured at 13 and 15 for a
    // single write. Re-applying on each one feeds back into itself and the
    // display visibly flickers, so our own changes are ignored for a moment
    // and external ones are coalesced.
    private var selfChangeUntil = Date.distantPast
    private var reapplyTask: Task<Void, Never>?

    private func markSelfChange() {
        selfChangeUntil = Date().addingTimeInterval(1.5)
    }

    /// `force` skips the self-change guard and re-applies everything
    /// unconditionally. Only safe for wake events: those cannot be caused by
    /// our own writes, so there is no feedback loop to start. Reconfiguration
    /// events *are* triggered by our writes, and must stay guarded.
    func scheduleReapply(force: Bool = false) {
        reapplyTask?.cancel()
        reapplyTask = Task { @MainActor [weak self] in
            try? await Task.sleep(nanoseconds: 700_000_000)
            guard !Task.isCancelled, let self else { return }
            self.refresh()
            // A change we caused ourselves needs no response.
            guard force || Date() >= self.selfChangeUntil else { return }
            self.reapplyAll()

            // Displays can come back progressively after a wake, so assert a
            // second time once things have settled.
            guard force else { return }
            try? await Task.sleep(nanoseconds: 3_000_000_000)
            guard !Task.isCancelled else { return }
            self.reapplyAll()
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
        registerDisplayReconfigurationCallback()

        reapplyAll()
    }

    func refresh() {
        panels = activeDisplays().map { d in
            PanelState(
                id: d.id,
                name: d.name,
                isBuiltin: d.isBuiltin,
                isEink: EinkSettings.isEink(d.id),
                // Stored value is authoritative now: the profile is removed on
                // quit, so it is absent on a fresh launch.
                saturation: EinkSettings.saturation(d.id),
                enhance: EinkSettings.enhance(d.id),
                textLevel: EinkSettings.textLevel(d.id),
                advanced: EinkSettings.advanced(d.id),
                custom: EinkSettings.customCurve(d.id),
                reduceShaking: EinkSettings.reduceShaking(d.id),
                shakingSupported: Dither.isSupported(displayID: d.id),
                isTelevision: DisplayRole.isTelevision(displayID: d.id),
                roleNeedsRestart: DisplayRole.needsRestart(displayID: d.id)
            )
        }
    }

    /// Re-asserts everything this app owns. Both adjustments are now
    /// app-managed: quitting returns displays to their original state, so
    /// launching has to put the stored settings back.
    func reapplyAll() {
        markSelfChange()
        // Saturation first, then curves. Installing a colour profile clears the
        // display's gamma table, so a curve applied before it is silently wiped
        // — which is why settings appeared not to reload at launch.
        for panel in panels where panel.isEink {
            let stored = EinkSettings.saturation(panel.id)
            if abs(stored - 1.0) > 0.001 {
                try? applySaturation(stored, displayID: panel.id, displayName: panel.name)
            }
        }
        for panel in panels { reapplyEnhance(displayID: panel.id) }
        // Dithering is hardware state that survives across processes and is
        // reset by display reconfiguration, so it is re-asserted here too.
        for panel in panels {
            Dither.setDisabled(panel.isEink && panel.reduceShaking, displayID: panel.id)
        }
        reassertCurvesSoon()
    }

    /// AppKit's didChangeScreenParameters does not fire for every display
    /// reconfiguration. Stillcolor uses the CoreGraphics callback for exactly
    /// this reason, so it is registered here as well.
    private func registerDisplayReconfigurationCallback() {
        let context = Unmanaged.passUnretained(self).toOpaque()
        CGDisplayRegisterReconfigurationCallback({ _, flags, userInfo in
            guard let userInfo else { return }
            let relevant: CGDisplayChangeSummaryFlags =
                [.addFlag, .removeFlag, .enabledFlag, .disabledFlag, .setModeFlag]
            guard !flags.intersection(relevant).isEmpty else { return }
            let model = Unmanaged<AssistantModel>.fromOpaque(userInfo).takeUnretainedValue()
            Task { @MainActor in model.scheduleReapply() }
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
            for panel in self.panels { reapplyEnhance(displayID: panel.id) }
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

        markSelfChange()
        panels[i].isEink = value
        EinkSettings.setEink(value, for: id)

        // Dithering shimmer is the first thing people notice on e-ink, so
        // marking a display turns this on rather than making them find it.
        if value {
            panels[i].reduceShaking = true
            EinkSettings.setReduceShaking(true, for: id)
            Dither.setDisabled(true, displayID: id)
        }

        // Un-marking a display restores it completely: no tone curve, and the
        // factory colour profile back in place. Safe to do unconditionally
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
            setSaturation(1.0, for: id)
        }
    }

    func setSaturation(_ amount: Double, for id: CGDirectDisplayID) {
        guard let i = index(of: id) else { return }
        markSelfChange()
        panels[i].saturation = amount
        EinkSettings.setSaturation(amount, for: id)
        do {
            try applySaturation(amount, displayID: id, displayName: panels[i].name)
            lastError = nil
        } catch {
            lastError = String(format: L("error.saturation"), panels[i].name)
        }
        // The profile write just cleared this display's gamma table; put any
        // active curve back.
        reapplyEnhance(displayID: id)
        reassertCurvesSoon()
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
    /// an administrator password and a restart, so it runs off the main thread
    /// and is never applied implicitly.
    func setTelevision(_ value: Bool, for id: CGDirectDisplayID) {
        guard let i = index(of: id), panels[i].isTelevision != value else { return }
        let previous = panels[i].isTelevision
        panels[i].isTelevision = value          // optimistic, reverted on failure
        Task.detached {
            do {
                try DisplayRole.setTelevision(value, displayID: id)
                await MainActor.run { self.refreshRole(for: id) }
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
        panels[i].roleNeedsRestart = DisplayRole.needsRestart(displayID: id)
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

    func setLanguage(_ value: AppLanguage) {
        guard language != value else { return }
        Localization.set(value)
        language = value          // republishes, so every L() re-resolves
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
        for panel in panels { clearToneCurveLive(displayID: panel.id) }
    }
}

// MARK: - Panel row

struct PanelRow: View {
    @ObservedObject var model: AssistantModel
    // A binding, not a copy. A captured struct keeps reporting its
    // creation-time value during a drag, so the slider springs back.
    @Binding var panel: PanelState
    @State private var showShakingInfo = false
    @State private var renamingSlot: Int?
    @State private var renameDraft = ""

    // 100% is the "off" shortcut — it drops the profile override entirely
    // rather than installing an identity one.
    // Named presets. 100% is the "off" shortcut: it drops the profile override
    // rather than installing an identity one.
    private let presets: [(value: Double, key: String)] = [
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
                        .foregroundStyle(.secondary)
                    Text(panel.name).lineLimit(1).truncationMode(.tail)
                    if panel.isBuiltin {
                        Text(L("display.builtin")).font(.system(size: 12)).foregroundStyle(.secondary)
                    }
                }
            }
            .font(.system(size: 14, weight: .medium))

            if panel.isEink {
                shakingSection
                roleSection
                saturationSection
                if !panel.advanced {
                    textSection
                    enhanceSection
                }
                advancedSection
                curveSection
            }
        }
        .padding(14)
        .background(Color.primary.opacity(panel.isEink ? 0.05 : 0.02))
        .clipShape(RoundedRectangle(cornerRadius: 6))
    }

    private var saturationSection: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack {
                Text(L("saturation.title")).font(.system(size: 13, weight: .medium))
                Spacer()
                Text("\(Int((panel.saturation * 100).rounded()))%")
                    .font(.system(size: 18, weight: .semibold))
                    .monospacedDigit()
            }
            Slider(
                    value: $panel.saturation,
                    in: 0.6...3.0,
                    // Each change rewrites a display profile, so commit on release.
                    onEditingChanged: { editing in
                        if !editing { model.setSaturation(panel.saturation, for: panel.id) }
                    }
                )
            HStack(spacing: 6) {
                ForEach(0..<presets.count, id: \.self) { i in
                    let preset = presets[i]
                    Button(L(preset.key)) {
                        model.setSaturation(preset.value, for: panel.id)
                    }
                    .buttonStyle(.bordered)
                    .controlSize(.small)
                    .font(.system(size: 12))
                }
            }
            Text(L("saturation.caption"))
                .font(.system(size: 11)).foregroundStyle(.secondary)
        }
    }

    /// The tone curve currently applied, drawn the same way the tuning labs
    /// draw it. Always shown, so the graph does not vanish when a mode is off
    /// or the advanced curve is reset.
    private var curveSection: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack {
                Text(L("curve.title")).font(.system(size: 12, weight: .medium))
                Spacer()
                Text(activeModeName)
                    .font(.system(size: 11)).foregroundStyle(.secondary)
            }
            CurvePlot(curve: displayedCurve, height: 96)
        }
    }

    /// Night Shift and True Tone are withheld by macOS from displays it treats
    /// as televisions, so the role override is how they get turned off for one
    /// display. Separate from everything else because it needs a password and a
    /// restart, and is not undone on quit.
    private var roleSection: some View {
        VStack(alignment: .leading, spacing: 4) {
            Toggle(isOn: Binding(
                get: { panel.isTelevision },
                set: { model.setTelevision($0, for: panel.id) }
            )) {
                Text(L("role.title")).font(.system(size: 13, weight: .medium))
            }
            .toggleStyle(.switch)
            .controlSize(.small)

            if panel.roleNeedsRestart {
                Label(L("role.restart"), systemImage: "arrow.clockwise.circle")
                    .font(.system(size: 11))
                    .foregroundStyle(Color.orange)
            }
            Text(L("role.note"))
                .font(.system(size: 11)).foregroundStyle(.secondary)
                .fixedSize(horizontal: false, vertical: true)
        }
    }

    /// Dithering control. Hidden when no framebuffer could be matched, rather
    /// than offering a toggle that would do nothing.
    @ViewBuilder private var shakingSection: some View {
        if panel.shakingSupported {
            HStack(spacing: 6) {
                Toggle(isOn: Binding(
                    get: { panel.reduceShaking },
                    set: { model.setReduceShaking($0, for: panel.id) }
                )) {
                    Text(L("shaking.title")).font(.system(size: 13, weight: .medium))
                }
                .toggleStyle(.switch)
                .controlSize(.small)
                // A hover tooltip never fires here: .help() relies on
                // NSView.toolTip, which needs an active window, and a menu bar
                // panel is non-activating. A popover works in a panel.
                Button { showShakingInfo.toggle() } label: {
                    Image(systemName: "info.circle")
                        .font(.system(size: 13))
                        .foregroundStyle(.secondary)
                }
                .buttonStyle(.plain)
                .contentShape(Rectangle())
                .popover(isPresented: $showShakingInfo, arrowEdge: .bottom) {
                    Text(L("shaking.info"))
                        .font(.system(size: 12))
                        .fixedSize(horizontal: false, vertical: true)
                        .frame(width: 280)
                        .padding(14)
                }
                Spacer()
            }
        }
    }

    /// Full manual control of the curve, replacing the preset pickers.
    @ViewBuilder private var advancedSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Toggle(isOn: Binding(
                get: { panel.advanced },
                set: { model.setAdvanced($0, for: panel.id) }
            )) {
                Text(L("advanced.title")).font(.system(size: 13, weight: .medium))
            }
            .toggleStyle(.switch)
            .controlSize(.small)

            if panel.advanced {
                Text(L("advanced.note"))
                    .font(.system(size: 11)).foregroundStyle(.secondary)
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
                    .controlSize(.small)
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
            Text(L("presets.title")).font(.system(size: 11, weight: .medium))
            HStack(spacing: 5) {
                ForEach(0..<CurvePresets.slotCount, id: \.self) { slot in
                    slotView(slot)
                }
            }
            Text(L("presets.hint"))
                .font(.system(size: 10)).foregroundStyle(.secondary)
                .fixedSize(horizontal: false, vertical: true)
        }
    }

    @ViewBuilder private func slotView(_ slot: Int) -> some View {
        let saved = model.presets[slot]
        if renamingSlot == slot {
            TextField("", text: $renameDraft)
                .textFieldStyle(.roundedBorder)
                .font(.system(size: 11))
                .frame(width: 76)
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
                    .font(.system(size: 11, weight: saved == nil ? .regular : .semibold))
                    .lineLimit(1)
                    .truncationMode(.tail)
                    .frame(minWidth: 22, maxWidth: 76)
            }
            .buttonStyle(.bordered)
            .controlSize(.small)
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
                Text(title).font(.system(size: 11))
                Spacer()
                Text(String(format: "%.2f", value))
                    .font(.system(size: 11)).monospacedDigit()
                    .foregroundStyle(.secondary)
            }
            Slider(value: Binding(get: { value }, set: set), in: range)
                .controlSize(.small)
        }
    }

    private var textSection: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(L("text.title")).font(.system(size: 13, weight: .medium))
            Picker("", selection: Binding(
                get: { panel.textLevel },
                set: { model.setTextLevel($0, for: panel.id) }
            )) {
                ForEach(0..<textLevels.count, id: \.self) { i in
                    Text(textLevels[i].label).tag(textLevels[i])
                }
            }
            .pickerStyle(.segmented)
            .controlSize(.small)
            .labelsHidden()

            Text(panel.textLevel.detail
                 ?? L("text.caption"))
                .font(.system(size: 11)).foregroundStyle(.secondary)
                .fixedSize(horizontal: false, vertical: true)
        }
    }

    private var enhanceSection: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(L("video.title")).font(.system(size: 13, weight: .medium))
            Picker("", selection: Binding(
                get: { panel.enhance },
                set: { model.setEnhance($0, for: panel.id) }
            )) {
                ForEach(0..<levels.count, id: \.self) { i in
                    Text(levels[i].label).tag(levels[i])
                }
            }
            .pickerStyle(.segmented)
            .controlSize(.small)
            .labelsHidden()

            if let cost = panel.enhance.textContrastCost {
                Label(String(format: L("video.warning"), cost),
                      systemImage: "exclamationmark.triangle.fill")
                    .font(.system(size: 11))
                    .foregroundStyle(.orange)
                    .fixedSize(horizontal: false, vertical: true)
            } else if panel.textLevel != .off {
                Text(L("video.blocked"))
                    .font(.system(size: 11)).foregroundStyle(.secondary)
                    .fixedSize(horizontal: false, vertical: true)
            } else {
                Text(L("video.caption"))
                    .font(.system(size: 11)).foregroundStyle(.secondary)
            }
        }
    }
}

// MARK: - Explainer

struct HowItWorks: View {
    @State private var expanded = false

    var body: some View {
        DisclosureGroup(isExpanded: $expanded) {
            VStack(alignment: .leading, spacing: 6) {
                Text(L("help.saturation"))
                Text(L("help.video"))
                Text(L("help.tradeoff"))
                Text(L("help.volatile"))
            }
            .font(.system(size: 13))
            .foregroundStyle(.secondary)
            .fixedSize(horizontal: false, vertical: true)
            .padding(.top, 4)
        } label: {
            Text(L("help.title")).font(.system(size: 14, weight: .medium))
        }
    }
}

// MARK: - Main view

struct AssistantView: View {
    @ObservedObject var model: AssistantModel

    var body: some View {
        VStack(alignment: .leading, spacing: 18) {
            if model.panels.isEmpty {
                Text(L("display.none")).foregroundStyle(.secondary)
            } else {
                Text(L("display.mark"))
                    .font(.system(size: 12)).foregroundStyle(.secondary)
                ForEach($model.panels) { $panel in
                    PanelRow(model: model, panel: $panel)
                }
            }

            if let error = model.lastError {
                Text(error).font(.system(size: 12)).foregroundStyle(.red)
            }

            HowItWorks()

            // The settings here were tuned against one specific panel in one
            // specific configuration. Say so, rather than implying they are
            // universal.
            HStack(alignment: .top, spacing: 5) {
                Image(systemName: "info.circle")
                    .font(.system(size: 13))
                    .foregroundStyle(.secondary)
                VStack(alignment: .leading, spacing: 2) {
                    Text(L("notice.tuned"))
                    Text(L("notice.risk"))
                }
                .font(.system(size: 13))
                .foregroundStyle(.secondary)
                .fixedSize(horizontal: false, vertical: true)
            }

            Divider()

            HStack {
                Text(L("language.title")).font(.system(size: 13))
                Spacer()
                Picker("", selection: Binding(
                    get: { model.language },
                    set: { model.setLanguage($0) }
                )) {
                    ForEach(0..<AppLanguage.allCases.count, id: \.self) { i in
                        let lang = AppLanguage.allCases[i]
                        Text(lang.label).tag(lang)
                    }
                }
                .labelsHidden()
                .pickerStyle(.menu)
                .controlSize(.small)
                .fixedSize()
            }

            Toggle(L("login.toggle"), isOn: Binding(
                get: { model.launchAtLogin },
                set: { model.setLaunchAtLogin($0) }
            ))
            .toggleStyle(.switch).controlSize(.small).font(.system(size: 13))

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
/// measured and the window is sized to that, capped so it starts scrolling
/// rather than running off the screen.
struct AssistantScroll: View {
    @ObservedObject var model: AssistantModel
    @State private var contentHeight: CGFloat = 420

    private static let maxHeight: CGFloat = 640
    private static let minHeight: CGFloat = 160

    /// Height of the pinned header, which the content measurement excludes.
    private static let headerHeight: CGFloat = 48

    private var clampedHeight: CGFloat {
        min(max(contentHeight + Self.headerHeight, Self.minHeight), Self.maxHeight)
    }

    var body: some View {
        VStack(spacing: 0) {
            // Pinned header: Quit stays at the window's top right instead of
            // scrolling away with the content.
            HStack {
                Text(L("app.title")).font(.system(size: 15, weight: .semibold))
                Text("v" + (Bundle.main.object(forInfoDictionaryKey:
                        "CFBundleShortVersionString") as? String ?? "?"))
                    .font(.system(size: 11))
                    .foregroundStyle(.secondary)
                Spacer()
                Button(L("quit")) { NSApp.terminate(nil) }
                    .font(.system(size: 13))
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
        .onPreferenceChange(ContentHeightKey.self) { height in
            // Guard against a zero measurement collapsing the window again.
            if height > 1 { contentHeight = height }
        }
        // minWidth cannot be combined with a fixed height, so the height is
        // pinned by giving min and max the same value.
        .frame(minWidth: 500, minHeight: clampedHeight, maxHeight: clampedHeight)
    }
}

// MARK: - App

final class AssistantDelegate: NSObject, NSApplicationDelegate {
    var model: AssistantModel?

    func applicationDidFinishLaunching(_ note: Notification) {
        NSApp.setActivationPolicy(.accessory)
        // After the menu bar item exists, so the tip's arrow points at something.
        DispatchQueue.main.async { WelcomeWindow.showIfNeeded() }
        NSWorkspace.shared.notificationCenter.addObserver(
            forName: NSWorkspace.willPowerOffNotification,
            object: nil, queue: .main
        ) { _ in
            restoreAllDisplaysToneCurves()
            restoreAllDisplaysSaturation()
            Dither.restoreAll()
        }
    }

    // Video Enhance lives in a volatile gamma table; don't leave it applied
    // with nothing running to maintain or undo it. This must not depend on the
    // UI model — that is only wired up once the menu bar panel is opened, so
    // quitting without opening it used to leave the curve in place.
    func applicationWillTerminate(_ note: Notification) {
        restoreAllDisplaysToneCurves()
        restoreAllDisplaysSaturation()
        Dither.restoreAll()
    }
}

@main
struct EinkAssistantApp: App {
    @NSApplicationDelegateAdaptor(AssistantDelegate.self) var delegate
    @StateObject private var model = AssistantModel()

    var body: some Scene {
        MenuBarExtra("E-Ink Assistant", systemImage: "book.pages") {
            AssistantScroll(model: model)
                .onAppear { delegate.model = model }
        }
        .menuBarExtraStyle(.window)
    }
}
