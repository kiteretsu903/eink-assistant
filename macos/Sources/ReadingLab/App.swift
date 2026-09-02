// ReadingLab — live tuning for the reading (text contrast) curve.
//
// The mirror of Video Enhance: gamma above 1 darkens the low end while pinning
// white, pushing text toward the panel's floor instead of lifting shadows.
//
// Saturation is deliberately untouched here — reading mode is a tone-only
// adjustment, so anything the color profile is doing stays exactly as it is.
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
    @Published var curve = ToneCurve(knee: 1.00, gamma: 5.00, blackPoint: 0.10) { didSet { push() } }
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
    static func luminance(_ gray: Double) -> Double { lin(gray) }
    static func ratio(_ a: Double, _ b: Double) -> Double {
        let (x, y) = (luminance(a), luminance(b))
        return (max(x, y) + 0.05) / (min(x, y) + 0.05)
    }
}

// MARK: - Text specimen

/// Real text at the grays macOS actually renders, on a white card. Because the
/// gamma table applies to the whole display, what you see here *is* the result.
struct Specimen: View {
    let sample = "The quick brown fox jumps over the lazy dog. "
        + "Reading on a reflective panel depends on ink coverage as much as contrast."

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            row("Body 13pt", gray: 0.15, size: 13, weight: .regular)
            row("Body 13pt bold", gray: 0.15, size: 13, weight: .semibold)
            row("Small 11pt", gray: 0.15, size: 11, weight: .regular)
            row("Secondary 13pt", gray: 0.45, size: 13, weight: .regular)
            row("Tertiary 11pt", gray: 0.60, size: 11, weight: .regular)
            row("Heading 17pt", gray: 0.10, size: 17, weight: .bold)
        }
        .padding(14)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Color.white)          // a document page; white is pinned by the curve
        .einkOutlinedArea()
    }

    private func row(_ label: String, gray: Double,
                     size: CGFloat, weight: Font.Weight) -> some View {
        VStack(alignment: .leading, spacing: 1) {
            Text(label)
                .font(.system(size: 10))
                .foregroundStyle(Color(white: 0.55))
            Text(sample)
                .font(.system(size: size, weight: weight))
                .foregroundStyle(Color(white: gray))
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
            Text("Contrast against white").font(.system(size: 12, weight: .medium))
            ForEach(0..<levels.count, id: \.self) { i in
                let name = levels[i].0
                let v = levels[i].1
                let before = WCAG.ratio(v, 1.0)
                let after = WCAG.ratio(curve.value(v), curve.value(1.0))
                HStack(spacing: 6) {
                    Text(name).font(.system(size: 12))
                        .foregroundStyle(EinkPalette.secondaryText)
                        .frame(width: 104, alignment: .leading)
                    Text(String(format: "%.1f:1", before))
                        .font(.system(size: 12)).monospacedDigit()
                        .foregroundStyle(EinkPalette.secondaryText)
                    Image(systemName: "arrow.right").font(.system(size: 9))
                        .foregroundStyle(EinkPalette.secondaryText)
                    Text(String(format: "%.1f:1", after))
                        .font(.system(size: 12, weight: .medium)).monospacedDigit()
                    Text(String(format: "%+.0f%%", (after - before)/before*100))
                        .font(.system(size: 11))
                        .foregroundStyle(after >= before ? Color.green : Color.orange)
                }
            }
            if !curve.isMonotonic() {
                Label("not monotonic", systemImage: "exclamationmark.triangle.fill")
                    .font(.system(size: 11)).foregroundStyle(Color.orange)
            }
        }
    }
}

struct ReadingPresets: View {
    @ObservedObject var model: ReadingModel
    // Same definitions the app ships, so the lab and the app cannot drift.
    private let levels: [TextLevel] = [.off, .medium, .strong, .sharp, .solid]

    var body: some View {
        HStack(spacing: 6) {
            ForEach(0..<levels.count, id: \.self) { i in
                let level = levels[i]
                Button(level.label) {
                    model.curve = level.curve ?? ToneCurve(knee: 0.90, gamma: 1.0)
                }
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
                            Text("Knee").font(.system(size: 14, weight: .medium))
                            Spacer()
                            Text(String(format: "%.2f", model.curve.knee))
                                .monospacedDigit().foregroundStyle(EinkPalette.secondaryText)
                        }
                        EinkSlider(value: $model.curve.knee,
                                   in: 0.20...1.00,
                                   accessibilityLabel: "Knee")
                        Text("Below this level text is darkened; above it, untouched.")
                            .font(.system(size: 11)).foregroundStyle(EinkPalette.secondaryText)
                    }
                    VStack(alignment: .leading, spacing: 2) {
                        HStack {
                            Text("Darkening (γ)").font(.system(size: 14, weight: .medium))
                            Spacer()
                            Text(String(format: "%.2f", model.curve.gamma))
                                .monospacedDigit().foregroundStyle(EinkPalette.secondaryText)
                        }
                        EinkSlider(value: $model.curve.gamma,
                                   in: 1.00...6.00,
                                   accessibilityLabel: "Darkening")
                        Text("Higher pushes text toward the panel's floor. 1.00 is off.")
                            .font(.system(size: 11)).foregroundStyle(EinkPalette.secondaryText)
                    }
                    VStack(alignment: .leading, spacing: 2) {
                        HStack {
                            Text("Black point").font(.system(size: 14, weight: .medium))
                            Spacer()
                            Text(String(format: "%.2f", model.curve.blackPoint))
                                .monospacedDigit().foregroundStyle(EinkPalette.secondaryText)
                        }
                        EinkSlider(value: $model.curve.blackPoint,
                                   in: 0.00...0.40,
                                   accessibilityLabel: "Black point")
                        Text("Crushes antialiased edges to solid black. 0 is off.")
                            .font(.system(size: 11)).foregroundStyle(EinkPalette.secondaryText)
                    }
                    VStack(alignment: .leading, spacing: 2) {
                        HStack {
                            Text("White point").font(.system(size: 14, weight: .medium))
                            Spacer()
                            Text(String(format: "%.2f", model.curve.whitePoint))
                                .monospacedDigit().foregroundStyle(EinkPalette.secondaryText)
                        }
                        EinkSlider(value: $model.curve.whitePoint,
                                   in: 0.60...1.00,
                                   accessibilityLabel: "White point")
                        Text("Pushes the gray halo around glyphs to pure white. 1 is off.")
                            .font(.system(size: 11)).foregroundStyle(EinkPalette.secondaryText)
                    }
                    ReadingPresets(model: model)
                }
            }

            ContrastReadout(curve: model.curve)

            Divider()
            Specimen()

            Text("Saturation is not touched — this is a tone-only adjustment, so "
                 + "whatever your color profile is doing stays as it is. Live "
                 + "tuning only: the gamma table is reset by sleep, display "
                 + "changes and quitting.")
                .font(.system(size: 11)).foregroundStyle(EinkPalette.secondaryText)
                .fixedSize(horizontal: false, vertical: true)
        }
        .font(.system(size: 15))
        .padding(18)
        .frame(width: 700)
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
