// First-run tip.
//
// A menu bar app with no Dock icon is easy to launch and then lose, so this
// says where to find it. It also carries the one thing the app cannot do for
// you: the monitor's own hardware settings have to be right before the software
// tuning means much.
//
// Shown on every launch until the user opts out, since that advice is worth
// repeating while someone is still setting up.
//
// Drawn as a custom panel rather than an NSPopover. A popover anchored to a
// status item near the screen corner does not clamp itself to the screen and
// ran off the right edge, and showing one required activating the app, which
// made SwiftUI present the MenuBarExtra panel on top of the tip. A panel we
// position ourselves has neither problem: the arrow points at the icon, the
// frame is clamped, and it appears without activating anything.

import SwiftUI
import AppKit

// MARK: - Bubble

/// Rounded rectangle with a triangular arrow on top, pointing at `arrowX`
/// (measured from the bubble's leading edge).
private struct BubbleShape: Shape {
    var arrowX: CGFloat
    var arrowHeight: CGFloat = 10
    var arrowWidth: CGFloat = 22
    var radius: CGFloat = 12

    func path(in rect: CGRect) -> Path {
        let body = CGRect(x: rect.minX, y: rect.minY + arrowHeight,
                          width: rect.width, height: rect.height - arrowHeight)
        var path = Path(roundedRect: body, cornerRadius: radius)

        // Keep the arrow clear of the rounded corners.
        let limit = radius + arrowWidth / 2
        let tip = min(max(arrowX, body.minX + limit), body.maxX - limit)

        var arrow = Path()
        arrow.move(to: CGPoint(x: tip - arrowWidth / 2, y: body.minY + 0.5))
        arrow.addLine(to: CGPoint(x: tip, y: rect.minY))
        arrow.addLine(to: CGPoint(x: tip + arrowWidth / 2, y: body.minY + 0.5))
        arrow.closeSubpath()
        path.addPath(arrow)
        return path
    }
}

private struct TipBubble: View {
    let arrowX: CGFloat
    let close: () -> Void

    /// Margin around the bubble so its shadow is not clipped by the window.
    static let inset: CGFloat = 18

    var body: some View {
        WelcomeView(close: close)
            .padding(.top, 10)                 // room for the arrow
            .background(
                BubbleShape(arrowX: arrowX)
                    .fill(Color(nsColor: .windowBackgroundColor))
                    .shadow(color: .black.opacity(0.28), radius: 14, y: 5)
            )
            .padding(Self.inset)
    }
}

// MARK: - Presentation

enum WelcomeWindow {
    private static let suppressKey = "hide-welcome"
    private static var panel: NSPanel?
    private static var panelWatch: Timer?

    static var isSuppressed: Bool { UserDefaults.standard.bool(forKey: suppressKey) }
    static func setSuppressed(_ value: Bool) {
        UserDefaults.standard.set(value, forKey: suppressKey)
    }

    static func showIfNeeded() {
        guard !isSuppressed else { return }
        show()
    }

    /// The status item MenuBarExtra creates is not exposed to us, so it is found
    /// by class name. With separate Spaces per display there are several of
    /// these, including placeholders parked at x = 0, so this prefers a real
    /// slot on the active screen.
    private static func statusItemWindow() -> NSWindow? {
        let candidates = NSApp.windows.filter {
            NSStringFromClass(type(of: $0)).contains("StatusBarWindow") && $0.isVisible
        }
        let placed = candidates.filter { $0.frame.minX > 0 }
        if let screen = NSScreen.main,
           let onActiveScreen = placed.first(where: { screen.frame.intersects($0.frame) }) {
            return onActiveScreen
        }
        return placed.first ?? candidates.first
    }

    static func show() {
        guard panel == nil else { return }

        let hosting = NSHostingController(rootView: TipBubble(arrowX: 0, close: close))
        let size = hosting.view.fittingSize
        let inset = TipBubble.inset

        let icon = statusItemWindow()
        let screen = icon.flatMap { window in
            NSScreen.screens.first { $0.frame.intersects(window.frame) }
        } ?? NSScreen.main ?? NSScreen.screens[0]

        let origin: CGPoint
        let arrowX: CGFloat
        if let icon {
            let iconCentre = icon.frame.midX
            // Clamp so the bubble never runs off the screen, which is exactly
            // what NSPopover failed to do near the corner.
            let lower = screen.visibleFrame.minX - inset
            let upper = screen.visibleFrame.maxX - size.width + inset
            let x = min(max(iconCentre - size.width / 2, lower), max(lower, upper))
            origin = CGPoint(x: x, y: icon.frame.minY - size.height + inset)
            // The arrow still points at the icon once the bubble is clamped.
            arrowX = iconCentre - x - inset
        } else {
            origin = CGPoint(x: screen.frame.midX - size.width / 2,
                             y: screen.frame.midY - size.height / 2)
            arrowX = size.width / 2
        }
        hosting.rootView = TipBubble(arrowX: arrowX, close: close)

        // A non-activating panel takes clicks without bringing the app forward,
        // so the MenuBarExtra panel is never presented on top of the tip.
        let tip = NSPanel(contentRect: CGRect(origin: origin, size: size),
                          styleMask: [.borderless, .nonactivatingPanel],
                          backing: .buffered, defer: false)
        tip.contentViewController = hosting
        tip.isFloatingPanel = true
        tip.level = .floating
        tip.backgroundColor = .clear
        tip.isOpaque = false
        tip.hasShadow = false                  // the bubble draws its own
        tip.hidesOnDeactivate = false
        tip.setFrame(CGRect(origin: origin, size: size), display: true)
        tip.orderFrontRegardless()
        panel = tip
        watchForPanel()
    }

    /// Once the menu bar panel is open the user has found the app and the tip is
    /// only in the way, so it dismisses itself.
    private static func watchForPanel() {
        panelWatch?.invalidate()
        panelWatch = Timer.scheduledTimer(withTimeInterval: 0.4, repeats: true) { timer in
            Task { @MainActor in
                guard panel != nil else { timer.invalidate(); return }
                let panelOpen = NSApp.windows.contains {
                    NSStringFromClass(type(of: $0)).contains("MenuBarExtraWindow")
                        && $0.isVisible
                }
                if panelOpen { close() }
            }
        }
    }

    static func close() {
        panelWatch?.invalidate()
        panelWatch = nil
        panel?.orderOut(nil)
        panel = nil
    }
}

// MARK: - Content

/// Renders **bold** from the localized string. Text(String) is verbatim, so the
/// markup has to be parsed rather than passed through.
private func md(_ text: String) -> AttributedString {
    (try? AttributedString(markdown: text)) ?? AttributedString(text)
}

struct WelcomeView: View {
    let close: () -> Void
    @State private var suppress = WelcomeWindow.isSuppressed

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text(L("welcome.title"))
                .font(.system(size: 20, weight: .semibold))

            HStack(alignment: .top, spacing: 8) {
                Image(systemName: "menubar.arrow.up.rectangle")
                    .font(.system(size: 17))
                    .foregroundStyle(Color.accentColor)
                Text(md(L("welcome.menubar")))
                    .fixedSize(horizontal: false, vertical: true)
            }

            Divider()

            tip("display", L("welcome.bigme.title"), L("welcome.bigme.body"))
            tip("display.2", L("welcome.other.title"), L("welcome.other.body"))

            Divider()

            HStack {
                Toggle(L("welcome.hide"), isOn: Binding(
                    get: { suppress },
                    set: { suppress = $0; WelcomeWindow.setSuppressed($0) }
                ))
                Spacer()
                Button(L("welcome.done")) { close() }
                    .keyboardShortcut(.defaultAction)
            }
        }
        .font(.system(size: 14))
        .padding(24)
        .frame(width: 380)
    }

    private func tip(_ symbol: String, _ title: String, _ body: String) -> some View {
        HStack(alignment: .top, spacing: 8) {
            Image(systemName: symbol)
                .font(.system(size: 17))
                .foregroundStyle(.secondary)
                .frame(width: 20)
            VStack(alignment: .leading, spacing: 4) {
                Text(title).font(.system(size: 15, weight: .semibold))
                Text(md(body))
                    .foregroundStyle(.secondary)
                    .fixedSize(horizontal: false, vertical: true)
            }
        }
    }
}
