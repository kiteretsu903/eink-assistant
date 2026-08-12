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
}

@MainActor
final class AssistantModel: ObservableObject {
    @Published var panels: [PanelState] = []
    @Published var launchAtLogin = false
    @Published var lastError: String?

    init() {
        refresh()
        launchAtLogin = SMAppService.mainApp.status == .enabled

        // The gamma table is volatile: macOS clears it on wake and on any
        // display reconfiguration, so Video Enhance has to be re-asserted.
        NotificationCenter.default.addObserver(
            forName: NSApplication.didChangeScreenParametersNotification,
            object: nil, queue: .main
        ) { [weak self] _ in
            Task { @MainActor in self?.refresh(); self?.reapplyAll() }
        }
        NSWorkspace.shared.notificationCenter.addObserver(
            forName: NSWorkspace.didWakeNotification,
            object: nil, queue: .main
        ) { [weak self] _ in
            Task { @MainActor in self?.reapplyAll() }
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
                // The profile is authoritative for saturation, not a stored value.
                saturation: installedSaturation(displayID: d.id) ?? 1.0,
                enhance: EinkSettings.enhance(d.id)
            )
        }
    }

    func reapplyAll() {
        for panel in panels { reapplyEnhance(displayID: panel.id) }
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

        panels[i].isEink = value
        EinkSettings.setEink(value, for: id)

        // Un-marking a display should leave nothing of ours behind on it. The
        // tone curve is ours and volatile, so always clear it; saturation is a
        // profile the user may have deliberately set, so it is left alone.
        if !value {
            panels[i].enhance = .off
            EinkSettings.setEnhance(.off, for: id)
            clearToneCurveLive(displayID: id)
        }
    }

    func setSaturation(_ amount: Double, for id: CGDirectDisplayID) {
        guard let i = index(of: id) else { return }
        panels[i].saturation = amount
        do {
            try applySaturation(amount, displayID: id, displayName: panels[i].name)
            lastError = nil
        } catch {
            lastError = "Could not apply saturation to \(panels[i].name)."
        }
    }

    func setEnhance(_ level: EnhanceLevel, for id: CGDirectDisplayID) {
        guard let i = index(of: id) else { return }
        guard panels[i].enhance != level else { return }   // ignore phantom writes
        panels[i].enhance = level
        EinkSettings.setEnhance(level, for: id)
        reapplyEnhance(displayID: id)
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
            lastError = "Could not change launch at login."
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
    let panel: PanelState

    private let presets: [Double] = [1.3, 1.5, 2.0]
    private let levels: [EnhanceLevel] = [.off, .subtle, .medium, .strong]

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
                        Text("built-in").font(.system(size: 10)).foregroundStyle(.secondary)
                    }
                }
            }
            .font(.system(size: 12, weight: .medium))

            if panel.isEink {
                saturationSection
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
                Text("Saturation").font(.system(size: 11, weight: .medium))
                Spacer()
                Text("\(Int((panel.saturation * 100).rounded()))%")
                    .font(.system(size: 11)).monospacedDigit().foregroundStyle(.secondary)
            }
            HStack(spacing: 6) {
                Slider(
                    value: Binding(
                        get: { panel.saturation },
                        set: { v in
                            if let i = model.panels.firstIndex(where: { $0.id == panel.id }) {
                                model.panels[i].saturation = v
                            }
                        }
                    ),
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
            Text("Compensates for the narrow colour gamut. Persists after quitting.")
                .font(.system(size: 9)).foregroundStyle(.secondary)
        }
    }

    private var enhanceSection: some View {
        VStack(alignment: .leading, spacing: 3) {
            Text("Video Enhance").font(.system(size: 11, weight: .medium))
            Picker("", selection: Binding(
                get: { panel.enhance },
                set: { model.setEnhance($0, for: panel.id) }
            )) {
                ForEach(0..<levels.count, id: \.self) { i in
                    Text(levels[i].label).tag(levels[i])
                }
            }
            .pickerStyle(.segmented)
            .labelsHidden()

            if let cost = panel.enhance.textContrastCost {
                Label("Text is lighter — \(cost). Best turned off for reading.",
                      systemImage: "exclamationmark.triangle.fill")
                    .font(.system(size: 9))
                    .foregroundStyle(.orange)
                    .fixedSize(horizontal: false, vertical: true)
            } else {
                Text("Brightens dark areas only. Turn on for video and photos.")
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
                Text("Saturation rewrites the display's colour profile so macOS "
                     + "sends more vivid signals to a panel with a narrow gamut. "
                     + "It is stored in the profile, so it keeps working after "
                     + "you quit this app or restart.")
                Text("Video Enhance brightens only the darkest part of the image "
                     + "and leaves mid-tones and highlights exactly as they were. "
                     + "Colour e-ink has a low contrast ratio, so dark detail "
                     + "collapses into an undifferentiated mush; this spreads "
                     + "those tones apart so they become visible.")
                Text("The trade-off: it cannot tell dark video from dark text. "
                     + "Anything dark gets lighter, so body text loses contrast — "
                     + "noticeably so in Dark Mode, where the background lifts "
                     + "too. That is why it is recommended for video and photos, "
                     + "and off while reading.")
                Text("Video Enhance uses the display's gamma table, which macOS "
                     + "clears on sleep and when displays change. This app "
                     + "re-applies it, so it needs to stay running — unlike "
                     + "saturation.")
            }
            .font(.system(size: 10))
            .foregroundStyle(.secondary)
            .fixedSize(horizontal: false, vertical: true)
            .padding(.top, 4)
        } label: {
            Text("How this works").font(.system(size: 11, weight: .medium))
        }
    }
}

// MARK: - Main view

struct AssistantView: View {
    @ObservedObject var model: AssistantModel

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("E-Ink Assistant").font(.system(size: 13, weight: .semibold))

            if model.panels.isEmpty {
                Text("No displays found.").foregroundStyle(.secondary)
            } else {
                Text("Mark your e-ink panels:")
                    .font(.system(size: 10)).foregroundStyle(.secondary)
                ForEach(0..<model.panels.count, id: \.self) { i in
                    PanelRow(model: model, panel: model.panels[i])
                }
            }

            if let error = model.lastError {
                Text(error).font(.system(size: 10)).foregroundStyle(.red)
            }

            HowItWorks()

            Divider()

            Toggle("Launch at Login", isOn: Binding(
                get: { model.launchAtLogin },
                set: { model.setLaunchAtLogin($0) }
            ))
            .toggleStyle(.switch).controlSize(.small).font(.system(size: 11))

            HStack {
                Spacer()
                Button("Quit") { NSApp.terminate(nil) }.font(.system(size: 11))
            }
        }
        .padding(14)
        .frame(width: 340)
    }
}

// MARK: - App

final class AssistantDelegate: NSObject, NSApplicationDelegate {
    var model: AssistantModel?

    func applicationDidFinishLaunching(_ note: Notification) {
        NSApp.setActivationPolicy(.accessory)
    }

    // Video Enhance lives in a volatile gamma table; don't leave it applied
    // with nothing running to maintain or undo it.
    func applicationWillTerminate(_ note: Notification) {
        model?.restoreAll()
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
