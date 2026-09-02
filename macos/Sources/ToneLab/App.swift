// ToneLab — live tuning for the shadow-lift curve.
//
// Drives the display's gamma table directly so the panel updates while you drag
// a slider. Nothing here persists: quitting or sleeping restores the display.
// The point is to find values by eye, then bake them into an ICC profile.

import SwiftUI
import AppKit
import CoreGraphics

@MainActor
final class ToneModel: ObservableObject {
    @Published var displays: [Display] = []
    @Published var selected: CGDirectDisplayID = 0
    @Published var curve = ToneCurve() { didSet { push() } }
    @Published var live = true { didSet { push() } }

    init() {
        displays = activeDisplays()
        // Default to an external panel — the built-in rarely needs this.
        selected = displays.first(where: { !$0.isBuiltin })?.id
            ?? displays.first?.id ?? 0
    }

    func push() {
        guard selected != 0 else { return }
        if live && !curve.isIdentity {
            applyToneCurveLive(curve, displayID: selected)
        } else {
            clearToneCurveLive(displayID: selected)
        }
    }

    func select(_ id: CGDirectDisplayID) {
        // Leave the previous display as we found it.
        if selected != 0 && selected != id { clearToneCurveLive(displayID: selected) }
        selected = id
        push()
    }

    func reset() {
        curve = ToneCurve(knee: curve.knee, gamma: 1.0)
    }

    func restoreAll() {
        for d in displays { clearToneCurveLive(displayID: d.id) }
    }
}

// MARK: - Test patterns

/// A wedge of dark steps. If the panel can separate these, the curve is doing
/// its job; if they turn into visible bands, it is pushed too far.
struct StepWedge: View {
    let from: Double
    let to: Double
    let steps: Int
    let label: String

    var body: some View {
        VStack(alignment: .leading, spacing: 3) {
            Text(label)
                .font(.system(size: 12))
                .foregroundStyle(EinkPalette.secondaryText)
            HStack(spacing: 1) {
                ForEach(0..<steps, id: \.self) { i in
                    let v = from + (to - from) * Double(i) / Double(steps - 1)
                    Rectangle().fill(Color(white: v))
                }
            }
            .frame(height: 34)
        }
    }
}

/// Smooth ramp, for spotting banding the stepped wedge would hide.
struct Ramp: View {
    let from: Double
    let to: Double
    let label: String

    var body: some View {
        VStack(alignment: .leading, spacing: 3) {
            Text(label)
                .font(.system(size: 12))
                .foregroundStyle(EinkPalette.secondaryText)
            LinearGradient(
                colors: stride(from: 0.0, through: 1.0, by: 0.02)
                    .map { Color(white: from + (to - from) * $0) },
                startPoint: .leading, endPoint: .trailing
            )
            .frame(height: 34)
        }
    }
}

// MARK: - Read-outs

/// Concrete numbers for what the curve does at the dark end.
struct ReadoutRow: View {
    let curve: ToneCurve
    private let levels: [Double] = [0.05, 0.10, 0.20, 0.30, 0.50]

    var body: some View {
        HStack(spacing: 14) {
            ForEach(0..<levels.count, id: \.self) { i in
                let x = levels[i]
                VStack(spacing: 1) {
                    Text(String(format: "%.2f", x))
                        .font(.system(size: 12)).foregroundStyle(EinkPalette.secondaryText)
                    Text(String(format: "%.2f", curve.value(x)))
                        .font(.system(size: 13, weight: .medium)).monospacedDigit()
                    Text(String(format: "%.1f×", curve.lift(at: x)))
                        .font(.system(size: 11)).foregroundStyle(Color.accentColor)
                }
            }
            Spacer()
            if !curve.isMonotonic() {
                Label("not monotonic", systemImage: "exclamationmark.triangle.fill")
                    .font(.system(size: 12)).foregroundStyle(.orange)
            }
        }
    }
}

struct Preset: Identifiable {
    let id: String
    let knee: Double
    let gamma: Double
}

struct PresetButtons: View {
    @ObservedObject var model: ToneModel
    private let presets = [
        Preset(id: "subtle", knee: 0.25, gamma: 0.75),
        Preset(id: "start", knee: 0.35, gamma: 0.60),
        Preset(id: "strong", knee: 0.45, gamma: 0.45),
    ]

    var body: some View {
        ForEach(0..<presets.count, id: \.self) { i in
            let preset = presets[i]
            Button(preset.id) {
                model.curve = ToneCurve(knee: preset.knee, gamma: preset.gamma)
            }
        }
    }
}

// MARK: - Main view

struct ToneLabView: View {
    @ObservedObject var model: ToneModel

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            HStack {
                Text("ToneLab — shadow lift").font(.headline)
                Spacer()
                Toggle("Live", isOn: $model.live).toggleStyle(.switch)
            }

            Picker("Display", selection: Binding(
                get: { model.selected },
                set: { model.select($0) }
            )) {
                ForEach(0..<model.displays.count, id: \.self) { i in
                    let d = model.displays[i]
                    Text("\(d.name)\(d.isBuiltin ? " (built-in)" : "")").tag(d.id)
                }
            }

            HStack(alignment: .top, spacing: 16) {
                CurvePlot(curve: model.curve).frame(width: 200)

                VStack(alignment: .leading, spacing: 10) {
                    VStack(alignment: .leading, spacing: 2) {
                        HStack {
                            Text("Knee").font(.system(size: 14, weight: .medium))
                            Spacer()
                            Text(String(format: "%.2f", model.curve.knee))
                                .monospacedDigit().foregroundStyle(EinkPalette.secondaryText)
                        }
                        EinkSlider(value: $model.curve.knee,
                                   in: 0.05...0.80,
                                   accessibilityLabel: "Knee")
                        Text("Above this level the image is untouched.")
                            .font(.system(size: 12)).foregroundStyle(EinkPalette.secondaryText)
                    }
                    VStack(alignment: .leading, spacing: 2) {
                        HStack {
                            Text("Shadow strength (γ)")
                                .font(.system(size: 14, weight: .medium))
                            Spacer()
                            Text(String(format: "%.2f", model.curve.gamma))
                                .monospacedDigit().foregroundStyle(EinkPalette.secondaryText)
                        }
                        EinkSlider(value: $model.curve.gamma,
                                   in: 0.30...1.00,
                                   accessibilityLabel: "Shadow strength")
                        Text("Lower lifts shadows harder. 1.00 is off.")
                            .font(.system(size: 12)).foregroundStyle(EinkPalette.secondaryText)
                    }
                }
            }

            ReadoutRow(curve: model.curve)

            Divider()

            StepWedge(from: 0.0, to: 0.35, steps: 12, label: "Dark steps (0.00 – 0.35)")
            Ramp(from: 0.0, to: 0.35, label: "Dark ramp — look for banding")
            Ramp(from: 0.0, to: 1.0, label: "Full range")

            Divider()

            HStack {
                Button("Reset curve") { model.reset() }
                Spacer()
                PresetButtons(model: model)
            }

            Text("Live tuning only — the gamma table is reset by sleep, display "
                 + "changes and logout. Once you settle on values, they get baked "
                 + "into an ICC profile to persist.")
                .font(.system(size: 12)).foregroundStyle(EinkPalette.secondaryText)
                .fixedSize(horizontal: false, vertical: true)
        }
        .font(.system(size: 15))
        .padding(18)
        .frame(width: 680)
    }
}

// MARK: - App

final class ToneLabDelegate: NSObject, NSApplicationDelegate {
    var model: ToneModel?
    func applicationShouldTerminateAfterLastWindowClosed(_ s: NSApplication) -> Bool { true }
    // Never leave a display stuck on a prototype curve.
    func applicationWillTerminate(_ note: Notification) {
        model?.restoreAll()
    }
}

@main
struct ToneLabApp: App {
    @NSApplicationDelegateAdaptor(ToneLabDelegate.self) var delegate
    @StateObject private var model = ToneModel()

    var body: some Scene {
        WindowGroup("ToneLab") {
            ToneLabView(model: model)
                .onAppear { delegate.model = model }
        }
        .windowResizability(.contentSize)
    }
}
