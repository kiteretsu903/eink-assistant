// A bubble anchored under the menu bar icon.
//
// Used for both the panel and the first-run tip, in place of NSPopover.
// NSPopover does not clamp itself to the screen when its anchor is a status
// item near a corner: it centres on the icon and lets the overflow hang off the
// edge, which clipped both the tip and the panel. Positioning the window
// ourselves lets the frame be clamped while the arrow keeps pointing at the
// icon.

import SwiftUI
import AppKit

/// Rounded rectangle with a triangular arrow on top, pointing at `arrowX`
/// (measured from the bubble's leading edge).
struct BubbleShape: Shape {
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

private struct Bubble: View {
    let arrowX: CGFloat
    let content: AnyView

    /// Margin around the bubble so its shadow is not clipped by the window.
    static let inset: CGFloat = 18

    var body: some View {
        content
            .padding(.top, 10)                 // room for the arrow
            .background(
                BubbleShape(arrowX: arrowX)
                    .fill(Color(nsColor: .windowBackgroundColor))
                    .shadow(color: .black.opacity(0.28), radius: 14, y: 5)
            )
            .padding(Self.inset)
    }
}

@MainActor
final class BubbleWindow {
    private var panel: NSPanel?
    private var outsideClickMonitor: Any?
    private let closesOnOutsideClick: Bool
    /// Controls draw in their inactive grey state unless the app is active.
    /// Worth doing for a panel the user just clicked open; not for a tip that
    /// appears by itself at launch and should not steal focus.
    private let activatesApp: Bool

    /// Distance kept between the bubble and the screen edge.
    private let screenMargin: CGFloat = 14

    /// Pulls the bubble up so the arrow sits against the menu bar rather than
    /// floating below it. The status item's window is taller than the visible
    /// menu bar, so aligning to its bottom edge leaves a gap.
    private let menuBarOverlap: CGFloat = 7

    init(closesOnOutsideClick: Bool, activatesApp: Bool = false) {
        self.closesOnOutsideClick = closesOnOutsideClick
        self.activatesApp = activatesApp
    }

    var isVisible: Bool { panel != nil }

    func show<Content: View>(from button: NSStatusBarButton?,
                             @ViewBuilder content: () -> Content) {
        close()
        let wrapped = AnyView(content())
        let hosting = NSHostingController(rootView: Bubble(arrowX: 0, content: wrapped))
        let size = hosting.view.fittingSize
        let inset = Bubble.inset

        let anchor = button?.window
        let screen = anchor.flatMap { window in
            NSScreen.screens.first { $0.frame.intersects(window.frame) }
        } ?? NSScreen.main ?? NSScreen.screens[0]

        let origin: CGPoint
        let arrowX: CGFloat
        if let anchor {
            let iconCentre = anchor.frame.midX
            // The clamp NSPopover does not do.
            let lower = screen.visibleFrame.minX - inset + screenMargin
            let upper = screen.visibleFrame.maxX - size.width + inset - screenMargin
            let x = min(max(iconCentre - size.width / 2, lower), max(lower, upper))
            origin = CGPoint(x: x,
                             y: anchor.frame.minY - size.height + inset + menuBarOverlap)
            // The arrow still points at the icon once the bubble is clamped.
            arrowX = iconCentre - x - inset
        } else {
            origin = CGPoint(x: screen.frame.midX - size.width / 2,
                             y: screen.frame.midY - size.height / 2)
            arrowX = size.width / 2
        }
        hosting.rootView = Bubble(arrowX: arrowX, content: wrapped)

        // Non-activating, so showing it never brings the app forward or
        // disturbs whatever the user is working in.
        let window = NSPanel(contentRect: CGRect(origin: origin, size: size),
                             styleMask: [.borderless, .nonactivatingPanel],
                             backing: .buffered, defer: false)
        window.contentViewController = hosting
        window.isFloatingPanel = true
        window.level = .floating
        window.backgroundColor = .clear
        window.isOpaque = false
        window.hasShadow = false               // the bubble draws its own
        window.hidesOnDeactivate = false
        // Without becoming key the controls draw in their inactive grey state.
        // A non-activating panel can be key without bringing the app forward.
        window.becomesKeyOnlyIfNeeded = false
        window.setFrame(CGRect(origin: origin, size: size), display: true)
        window.orderFrontRegardless()
        if activatesApp {
            NSApp.activate(ignoringOtherApps: true)
            window.makeKeyAndOrderFront(nil)
        }
        panel = window

        guard closesOnOutsideClick else { return }
        outsideClickMonitor = NSEvent.addGlobalMonitorForEvents(
            matching: [.leftMouseDown, .rightMouseDown]
        ) { [weak self] _ in
            Task { @MainActor in
                guard let self else { return }
                // A click on the icon is the toggle's business, not ours.
                if let iconFrame = button?.window?.frame,
                   iconFrame.contains(NSEvent.mouseLocation) { return }
                self.close()
            }
        }
    }

    func close() {
        if let monitor = outsideClickMonitor {
            NSEvent.removeMonitor(monitor)
            outsideClickMonitor = nil
        }
        panel?.orderOut(nil)
        panel = nil
    }
}
