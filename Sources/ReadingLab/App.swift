// ReadingLab — live tuning for the reading (text contrast) curve.
//
// The mirror of Video Enhance: gamma above 1 darkens the low end while pinning
// white, pushing text toward the panel's floor instead of lifting shadows.
//
// Saturation is deliberately untouched here — reading mode is a tone-only
// adjustment, so anything the colour profile is doing stays exactly as it is.
//
// Drives the display's gamma table directly, so nothing persists: quitting or
// sleeping restores the display.

import SwiftUI
import AppKit
import CoreGraphics

@MainActor
final class ReadingModel: ObservableObject {
    @Published var displays: [Display] = []
    @Published var selected: CGDirectDisplayID = 0
    @Published var curve = ToneCurve(knee: 0.55, gamma: 1.70) { didSet { push() } }
    @Published var live = true { didSet { push() } }

    init() {
        displays = activeDisplays()
        selected = displays.first(where: { !$0.isBuiltin })?.id ?? displays.first?.id ?? 0
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
        if selected != 0 && selected != id { clearToneCurveLive(displayID: selected) }
        selected = id
        push()
    }

    func restoreAll() {
        for d in displays { clearToneCurveLive(displayID: d.id) }
    }
}

// MARK: - Contrast maths

enum WCAG {
    static func lin(_ c: Double) -> Double {
        c <= 0.04045 ? c/12.92 : pow((c + 0.055)/1.055, 2.4)
    }
    static func luminance(_ grey: Double) -> Double { lin(grey) }
    static func ratio(_ a: Double, _ b: Double) -> Double {
        let (x, y) = (luminance(a), luminance(b))
        return (max(x, y) + 0.05) / (min(x, y) + 0.05)
    }
}

// MARK: - Text specimen

/// Real text at the greys macOS actually renders, on a white card. Because the
/// gamma table applies to the whole display, what you see here *is* the result.
struct Specimen: View {
    let sample = "The quick brown fox jumps over the lazy dog. "
        + "Reading on a reflective panel depends on ink coverage as much as contrast."

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            row("Body 13pt", grey: 0.15, size: 13, weight: .regular)
            row("Body 13pt bold", grey: 0.15, size: 13, weight: .semibold)
            row("Small 11pt", grey: 0.15, size: 11, weight: .regular)
            row("Secondary 13pt", grey: 0.45, size: 13, weight: .regular)
            row("Tertiary 11pt", grey: 0.60, size: 11, weight: .regular)
            row("Heading 17pt", grey: 0.10, size: 17, weight: .bold)
        }
        .padding(14)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Color.white)          // a document page; white is pinned by the curve
        .clipShape(RoundedRectangle(cornerRadius: 6))
        .overlay(RoundedRectangle(cornerRadius: 6)
            .stroke(Color.secondary.opacity(0.3), lineWidth: 1))
    }

    private func row(_ label: String, grey: Double,
                     size: CGFloat, weight: Font.Weight) -> some View {
        VStack(alignment: .leading, spacing: 1) {
            Text(label)
                .font(.system(size: 8))
                .foregroundStyle(Color(white: 0.55))
            Text(sample)
                .font(.system(size: size, weight: weight))
                .foregroundStyle(Color(white: grey))
                .fixedSize(horizontal: false, vertical: true)
        }
    }
}

// MARK: - Read-out

struct ContrastReadout: View {
    let curve: ToneCurve
    private let levels: [(String, Double)] = [
        ("body 0.15", 0.15), ("secondary 0.45", 0.45), ("tertiary 0.60", 0.60),
    ]

    var body: some View {
        VStack(alignment: .leading, spacing: 3) {
            Text("Contrast against white").font(.system(size: 10, weight: .medium))
            ForEach(0..<levels.count, id: \.self) { i in
                let name = levels[i].0
                let v = levels[i].1
                let before = WCAG.ratio(v, 1.0)
                let after = WCAG.ratio(curve.value(v), curve.value(1.0))
                HStack(spacing: 6) {
                    Text(name).font(.system(size: 10))
                        .foregroundStyle(.secondary)
                        .frame(width: 92, alignment: .leading)
                    Text(String(format: "%.1f:1", before))
                        .font(.system(size: 10)).monospacedDigit()
                        .foregroundStyle(.secondary)
                    Image(systemName: "arrow.right").font(.system(size: 7))
                        .foregroundStyle(.secondary)
                    Text(String(format: "%.1f:1", after))
                        .font(.system(size: 10, weight: .medium)).monospacedDigit()
                    Text(String(format: "%+.0f%%", (after - before)/before*100))
                        .font(.system(size: 9))
                        .foregroundStyle(after >= before ? Color.green : Color.orange)
                }
            }
            if !curve.isMonotonic() {
                Label("not monotonic", systemImage: "exclamationmark.triangle.fill")
                    .font(.system(size: 9)).foregroundStyle(Color.orange)
            }
        }
    }
}

struct ReadingPresets: View {
    @ObservedObject var model: ReadingModel
    private let presets: [(String, Double, Double)] = [
        ("off", 0.55, 1.00), ("light", 0.45, 1.35),
        ("medium", 0.55, 1.70), ("strong", 0.65, 2.10),
    ]

    var body: some View {
        HStack(spacing: 6) {
            ForEach(0..<presets.count, id: \.self) { i in
                let p = presets[i]
                Button(p.0) { model.curve = ToneCurve(knee: p.1, gamma: p.2) }
                    .controlSize(.small)
            }
        }
    }
}

// MARK: - Main view

struct ReadingLabView: View {
    @ObservedObject var model: ReadingModel

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            HStack {
                Text("ReadingLab — text contrast").font(.headline)
                Spacer()
                Toggle("Live", isOn: $model.live).toggleStyle(.switch)
            }

            Picker("Display", selection: Binding(
                get: { model.selected }, set: { model.select($0) }
            )) {
                ForEach(0..<model.displays.count, id: \.self) { i in
                    let d = model.displays[i]
                    Text("\(d.name)\(d.isBuiltin ? " (built-in)" : "")").tag(d.id)
                }
            }

            HStack(alignment: .top, spacing: 16) {
                CurvePlot(curve: model.curve).frame(width: 170)

                VStack(alignment: .leading, spacing: 10) {
                    VStack(alignment: .leading, spacing: 2) {
                        HStack {
                            Text("Knee").font(.system(size: 12, weight: .medium))
                            Spacer()
                            Text(String(format: "%.2f", model.curve.knee))
                                .monospacedDigit().foregroundStyle(.secondary)
                        }
                        Slider(value: $model.curve.knee, in: 0.20...0.90)
                        Text("Below this level text is darkened; above it, untouched.")
                            .font(.system(size: 9)).foregroundStyle(.secondary)
                    }
                    VStack(alignment: .leading, spacing: 2) {
                        HStack {
                            Text("Darkening (γ)").font(.system(size: 12, weight: .medium))
                            Spacer()
                            Text(String(format: "%.2f", model.curve.gamma))
                                .monospacedDigit().foregroundStyle(.secondary)
                        }
                        Slider(value: $model.curve.gamma, in: 1.00...3.00)
                        Text("Higher pushes text toward the panel's floor. 1.00 is off.")
                            .font(.system(size: 9)).foregroundStyle(.secondary)
                    }
                    ReadingPresets(model: model)
                }
            }

            ContrastReadout(curve: model.curve)

            Divider()
            Specimen()

            Text("Saturation is not touched — this is a tone-only adjustment, so "
                 + "whatever your colour profile is doing stays as it is. Live "
                 + "tuning only: the gamma table is reset by sleep, display "
                 + "changes and quitting.")
                .font(.system(size: 9)).foregroundStyle(.secondary)
                .fixedSize(horizontal: false, vertical: true)
        }
        .padding(18)
        .frame(width: 640)
    }
}

// MARK: - App

final class ReadingLabDelegate: NSObject, NSApplicationDelegate {
    var model: ReadingModel?
    func applicationShouldTerminateAfterLastWindowClosed(_ s: NSApplication) -> Bool { true }
    func applicationWillTerminate(_ note: Notification) { model?.restoreAll() }
}

@main
struct ReadingLabApp: App {
    @NSApplicationDelegateAdaptor(ReadingLabDelegate.self) var delegate
    @StateObject private var model = ReadingModel()

    var body: some Scene {
        WindowGroup("ReadingLab") {
            ReadingLabView(model: model)
                .onAppear { delegate.model = model }
        }
        .windowResizability(.contentSize)
    }
}
