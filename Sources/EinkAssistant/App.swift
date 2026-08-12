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
}

@MainActor
final class AssistantModel: ObservableObject {
    @Published var panels: [PanelState] = []
    @Published var launchAtLogin = false
    @Published var language: AppLanguage = Localization.current
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

    private func scheduleReapply() {
        reapplyTask?.cancel()
        reapplyTask = Task { @MainActor [weak self] in
            try? await Task.sleep(nanoseconds: 700_000_000)
            guard !Task.isCancelled, let self else { return }
            self.refresh()
            // A change we caused ourselves needs no response.
            guard Date() >= self.selfChangeUntil else { return }
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
            Task { @MainActor in self?.scheduleReapply() }
        }

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
                textLevel: EinkSettings.textLevel(d.id)
            )
        }
    }

    /// Re-asserts everything this app owns. Both adjustments are now
    /// app-managed: quitting returns displays to their original state, so
    /// launching has to put the stored settings back.
    func reapplyAll() {
        markSelfChange()
        for panel in panels {
            reapplyEnhance(displayID: panel.id)
            guard panel.isEink else { continue }
            let stored = EinkSettings.saturation(panel.id)
            if abs(stored - 1.0) > 0.001 {
                try? applySaturation(stored, displayID: panel.id, displayName: panel.name)
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

        markSelfChange()
        panels[i].isEink = value
        EinkSettings.setEink(value, for: id)

        // Un-marking a display restores it completely: no tone curve, and the
        // factory colour profile back in place. Safe to do unconditionally
        // because of the transition guard above — an accidental write can no
        // longer reach this path.
        if !value {
            panels[i].enhance = .off
            panels[i].textLevel = .off
            EinkSettings.setEnhance(.off, for: id)
            EinkSettings.setTextLevel(.off, for: id)
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

    // 100% is the "off" shortcut — it drops the profile override entirely
    // rather than installing an identity one.
    private let presets: [Double] = [1.0, 1.3, 1.5, 2.0]
    private let levels: [EnhanceLevel] = [.off, .subtle, .medium, .strong]
    private let textLevels: [TextLevel] = [.off, .medium, .strong, .sharp, .solid]

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Toggle(isOn: Binding(
                get: { panel.isEink },
                set: { model.setEink($0, for: panel.id) }
            )) {
                HStack(spacing: 5) {
                    Image(systemName: panel.isBuiltin ? "laptopcomputer" : "display")
                        .foregroundStyle(.secondary)
                    Text(panel.name).lineLimit(1).truncationMode(.tail)
                    if panel.isBuiltin {
                        Text(L("display.builtin")).font(.system(size: 10)).foregroundStyle(.secondary)
                    }
                }
            }
            .font(.system(size: 12, weight: .medium))

            if panel.isEink {
                saturationSection
                textSection
                enhanceSection
            }
        }
        .padding(10)
        .background(Color.primary.opacity(panel.isEink ? 0.05 : 0.02))
        .clipShape(RoundedRectangle(cornerRadius: 6))
    }

    private var saturationSection: some View {
        VStack(alignment: .leading, spacing: 3) {
            HStack {
                Text(L("saturation.title")).font(.system(size: 11, weight: .medium))
                Spacer()
                Text("\(Int((panel.saturation * 100).rounded()))%")
                    .font(.system(size: 11)).monospacedDigit().foregroundStyle(.secondary)
            }
            HStack(spacing: 6) {
                Slider(
                    value: $panel.saturation,
                    in: 1.0...3.0,
                    // Each change rewrites a display profile, so commit on release.
                    onEditingChanged: { editing in
                        if !editing { model.setSaturation(panel.saturation, for: panel.id) }
                    }
                )
                ForEach(0..<presets.count, id: \.self) { i in
                    let value = presets[i]
                    Button("\(Int(value * 100))") {
                        model.setSaturation(value, for: panel.id)
                    }
                    .buttonStyle(.bordered)
                    .controlSize(.small)
                }
            }
            Text(L("saturation.caption"))
                .font(.system(size: 9)).foregroundStyle(.secondary)
        }
    }

    private var textSection: some View {
        VStack(alignment: .leading, spacing: 3) {
            Text(L("text.title")).font(.system(size: 11, weight: .medium))
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
                .font(.system(size: 9)).foregroundStyle(.secondary)
                .fixedSize(horizontal: false, vertical: true)
        }
    }

    private var enhanceSection: some View {
        VStack(alignment: .leading, spacing: 3) {
            Text(L("video.title")).font(.system(size: 11, weight: .medium))
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
                    .font(.system(size: 9))
                    .foregroundStyle(.orange)
                    .fixedSize(horizontal: false, vertical: true)
            } else if panel.textLevel != .off {
                Text(L("video.blocked"))
                    .font(.system(size: 9)).foregroundStyle(.secondary)
                    .fixedSize(horizontal: false, vertical: true)
            } else {
                Text(L("video.caption"))
                    .font(.system(size: 9)).foregroundStyle(.secondary)
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
            .font(.system(size: 10))
            .foregroundStyle(.secondary)
            .fixedSize(horizontal: false, vertical: true)
            .padding(.top, 4)
        } label: {
            Text(L("help.title")).font(.system(size: 11, weight: .medium))
        }
    }
}

// MARK: - Main view

struct AssistantView: View {
    @ObservedObject var model: AssistantModel

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text(L("app.title")).font(.system(size: 13, weight: .semibold))

            if model.panels.isEmpty {
                Text(L("display.none")).foregroundStyle(.secondary)
            } else {
                Text(L("display.mark"))
                    .font(.system(size: 10)).foregroundStyle(.secondary)
                ForEach($model.panels) { $panel in
                    PanelRow(model: model, panel: $panel)
                }
            }

            if let error = model.lastError {
                Text(error).font(.system(size: 10)).foregroundStyle(.red)
            }

            HowItWorks()

            Divider()

            HStack {
                Text(L("language.title")).font(.system(size: 11))
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
            .toggleStyle(.switch).controlSize(.small).font(.system(size: 11))

            HStack {
                Spacer()
                Button(L("quit")) { NSApp.terminate(nil) }.font(.system(size: 11))
            }
        }
        .padding(14)
        .frame(minWidth: 420)
    }
}

// MARK: - App

final class AssistantDelegate: NSObject, NSApplicationDelegate {
    var model: AssistantModel?

    func applicationDidFinishLaunching(_ note: Notification) {
        NSApp.setActivationPolicy(.accessory)
        NSWorkspace.shared.notificationCenter.addObserver(
            forName: NSWorkspace.willPowerOffNotification,
            object: nil, queue: .main
        ) { _ in
            restoreAllDisplaysToneCurves()
            restoreAllDisplaysSaturation()
        }
    }

    // Video Enhance lives in a volatile gamma table; don't leave it applied
    // with nothing running to maintain or undo it. This must not depend on the
    // UI model — that is only wired up once the menu bar panel is opened, so
    // quitting without opening it used to leave the curve in place.
    func applicationWillTerminate(_ note: Notification) {
        restoreAllDisplaysToneCurves()
        restoreAllDisplaysSaturation()
    }
}

@main
struct EinkAssistantApp: App {
    @NSApplicationDelegateAdaptor(AssistantDelegate.self) var delegate
    @StateObject private var model = AssistantModel()

    var body: some Scene {
        MenuBarExtra("E-Ink Assistant", systemImage: "book.pages") {
            AssistantView(model: model)
                .onAppear { delegate.model = model }
        }
        .menuBarExtraStyle(.window)
    }
}
