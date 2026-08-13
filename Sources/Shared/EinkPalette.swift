// Full-contrast text colors for reflective, low-contrast displays.
// SwiftUI's semantic .secondary is intentionally subtle on LCD/OLED, but that
// tonal hierarchy becomes washed-out text on e-ink. All readable text
// therefore uses the primary foreground; hierarchy comes from size, weight,
// spacing, and grouping instead of gray.

import SwiftUI
import AppKit

enum EinkPalette {
    static let secondaryText = Color.primary
    static let areaOutline = Color.primary
    static let areaOutlineWidth: CGFloat = 2
}

/// E-ink preserves hard edges more reliably than low-contrast surface fills.
/// Keep area interiors clear and communicate grouping with a solid outline.
extension View {
    func einkOutlinedArea(cornerRadius: CGFloat = 6) -> some View {
        clipShape(RoundedRectangle(cornerRadius: cornerRadius))
            .overlay {
                RoundedRectangle(cornerRadius: cornerRadius)
                    .stroke(EinkPalette.areaOutline,
                            lineWidth: EinkPalette.areaOutlineWidth)
            }
    }
}

/// Transparent button with a persistent hard edge. Disabled buttons stay
/// legible and use a dashed outline instead of fading to gray.
struct EinkOutlinedButtonStyle: ButtonStyle {
    @Environment(\.isEnabled) private var isEnabled
    var foreground: Color = .primary
    var compact = false

    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .foregroundStyle(foreground)
            .padding(.horizontal, compact ? 7 : 11)
            .padding(.vertical, compact ? 4 : 6)
            .contentShape(RoundedRectangle(cornerRadius: 5))
            .overlay {
                RoundedRectangle(cornerRadius: 5)
                    .stroke(
                        EinkPalette.areaOutline,
                        style: StrokeStyle(
                            lineWidth: configuration.isPressed ? 3 : 2,
                            dash: isEnabled ? [] : [4, 3]
                        )
                    )
            }
    }
}

/// Native macOS checkboxes use a pale filled square when off. This keeps both
/// states white/clear with a black frame and adds a blue check only when on.
struct EinkCheckboxToggleStyle: ToggleStyle {
    func makeBody(configuration: Configuration) -> some View {
        Button { configuration.isOn.toggle() } label: {
            HStack(spacing: 8) {
                ZStack {
                    RoundedRectangle(cornerRadius: 3)
                        .stroke(EinkPalette.areaOutline, lineWidth: 2)
                        .frame(width: 22, height: 22)
                    if configuration.isOn {
                        Image(systemName: "checkmark")
                            .font(.system(size: 14, weight: .bold))
                            .foregroundStyle(Color.accentColor)
                    }
                }
                configuration.label
            }
            .contentShape(Rectangle())
        }
        .buttonStyle(.plain)
        .foregroundStyle(Color.primary)
    }
}

/// A hard-outline switch: no gray track, no animated transition, and a blue
/// marker for On. The instant state change also avoids unnecessary e-ink work.
struct EinkSwitchToggleStyle: ToggleStyle {
    var showsLabel = true

    func makeBody(configuration: Configuration) -> some View {
        HStack(spacing: showsLabel ? 8 : 0) {
            if showsLabel { configuration.label }
            Button { configuration.isOn.toggle() } label: {
                ZStack(alignment: configuration.isOn ? .trailing : .leading) {
                    RoundedRectangle(cornerRadius: 5)
                        .stroke(EinkPalette.areaOutline, lineWidth: 2)
                        .frame(width: 42, height: 24)
                    RoundedRectangle(cornerRadius: 3)
                        .fill(configuration.isOn ? Color.accentColor : Color.primary)
                        .frame(width: 16, height: 16)
                        .padding(4)
                }
                // Keep a generous, fixed hit target in both states.  The dark
                // Off marker must be just as easy to click as the blue On one.
                .frame(width: 50, height: 30)
                .contentShape(Rectangle())
            }
            .buttonStyle(.plain)
            .accessibilityValue(configuration.isOn ? "on" : "off")
        }
        .foregroundStyle(Color.primary)
    }
}

/// High-contrast slider for reflective displays. The native macOS slider uses
/// a thin gray rail and a pale thumb; both disappear easily on e-ink.
struct EinkSlider: View {
    @Binding var value: Double
    let range: ClosedRange<Double>
    let accessibilityLabel: String
    var accentColor: Color
    var onEditingChanged: ((Bool) -> Void)?

    @State private var isEditing = false

    init(value: Binding<Double>,
         in range: ClosedRange<Double>,
         accessibilityLabel: String,
         accentColor: Color = .accentColor,
         onEditingChanged: ((Bool) -> Void)? = nil) {
        _value = value
        self.range = range
        self.accessibilityLabel = accessibilityLabel
        self.accentColor = accentColor
        self.onEditingChanged = onEditingChanged
    }

    private var normalizedValue: Double {
        let span = range.upperBound - range.lowerBound
        guard span > 0 else { return 0 }
        return min(max((value - range.lowerBound) / span, 0), 1)
    }

    var body: some View {
        GeometryReader { geometry in
            let thumbSize: CGFloat = 24
            let railWidth = max(geometry.size.width - thumbSize, 1)
            let thumbX = railWidth * CGFloat(normalizedValue)

            ZStack(alignment: .leading) {
                RoundedRectangle(cornerRadius: 5)
                    .fill(Color(nsColor: .windowBackgroundColor))
                    .overlay {
                        RoundedRectangle(cornerRadius: 5)
                            .stroke(EinkPalette.areaOutline, lineWidth: 2)
                    }
                    .frame(width: railWidth, height: 10)
                    .offset(x: thumbSize / 2)

                RoundedRectangle(cornerRadius: 3)
                    .fill(accentColor)
                    .frame(width: max(thumbX, 0), height: 6)
                    .offset(x: thumbSize / 2)

                Circle()
                    .fill(Color(nsColor: .windowBackgroundColor))
                    .overlay {
                        Circle().stroke(EinkPalette.areaOutline, lineWidth: 3)
                    }
                    .frame(width: thumbSize, height: thumbSize)
                    .offset(x: thumbX)
            }
            .frame(maxHeight: .infinity)
            .contentShape(Rectangle())
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { gesture in
                        if !isEditing {
                            isEditing = true
                            onEditingChanged?(true)
                        }
                        setValue(at: gesture.location.x,
                                 width: geometry.size.width,
                                 thumbSize: thumbSize)
                    }
                    .onEnded { gesture in
                        setValue(at: gesture.location.x,
                                 width: geometry.size.width,
                                 thumbSize: thumbSize)
                        isEditing = false
                        onEditingChanged?(false)
                    }
            )
        }
        .frame(height: 28)
        .accessibilityElement()
        .accessibilityLabel(accessibilityLabel)
        .accessibilityValue(String(format: "%.2f", value))
        .accessibilityAdjustableAction { direction in
            let increment = (range.upperBound - range.lowerBound) / 20
            onEditingChanged?(true)
            switch direction {
            case .increment:
                value = min(value + increment, range.upperBound)
            case .decrement:
                value = max(value - increment, range.lowerBound)
            @unknown default:
                break
            }
            onEditingChanged?(false)
        }
    }

    private func setValue(at x: CGFloat, width: CGFloat, thumbSize: CGFloat) {
        let railWidth = max(width - thumbSize, 1)
        let position = min(max(x - thumbSize / 2, 0), railWidth)
        let fraction = Double(position / railWidth)
        value = range.lowerBound + fraction * (range.upperBound - range.lowerBound)
    }
}
