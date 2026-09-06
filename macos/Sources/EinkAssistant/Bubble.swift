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

@MainActor
private final class BubbleAnchor: ObservableObject {
    @Published var arrowX: CGFloat = 0
}

private struct Bubble: View {
    @ObservedObject var anchor: BubbleAnchor
    let content: AnyView

    /// Vertical room reserved above the content for the menu-bar arrow.
    static let arrowRoom: CGFloat = 10
    /// Margin around the bubble so its shadow is not clipped by the window.
    static let inset: CGFloat = 18

    var body: some View {
        content
            .padding(.top, Self.arrowRoom)
            .background(
                BubbleShape(arrowX: anchor.arrowX)
                    .fill(Color(nsColor: .windowBackgroundColor))
                    .shadow(color: .black.opacity(0.28), radius: 14, y: 5)
            )
            .padding(Self.inset)
    }
}

/// A borderless window cannot become key by default, which leaves every control
/// drawn in its inactive gray state.
private final class KeyablePanel: NSPanel {
    override var canBecomeKey: Bool { true }
    override var canBecomeMain: Bool { true }
}

@MainActor
final class BubbleWindow {
    private var panel: NSPanel?
    private var outsideClickMonitor: Any?
    private var resizeObserver: NSObjectProtocol?
    private let closesOnOutsideClick: Bool
    /// Controls draw in their inactive gray state unless the app is active.
    /// Worth doing for a panel the user just clicked open; not for a tip that
    /// appears by itself at launch and should not steal focus.
    private let activatesApp: Bool

    /// Side clearance can stay compact; the bottom needs enough breathing room
    /// for the bubble's shadow to remain visible and feel detached from the
    /// screen edge or Dock.
    private let horizontalScreenMargin: CGFloat = 14
    private let bottomScreenMargin: CGFloat = 40

    /// Pulls the bubble up so the arrow sits against the menu bar rather than
    /// floating below it. The status item's window is taller than the visible
    /// menu bar, so aligning to its bottom edge leaves a gap.
    private let menuBarOverlap: CGFloat = 7

    init(closesOnOutsideClick: Bool, activatesApp: Bool = false) {
        self.closesOnOutsideClick = closesOnOutsideClick
        self.activatesApp = activatesApp
    }

    var isVisible: Bool { panel != nil }

    /// Maximum height available to the caller's content on the screen that
    /// owns the status item. The bubble's arrow and overlap consume a few
    /// points above the content; `bottomScreenMargin` leaves a natural gap
    /// between the visible bubble and the Dock or bottom edge.
    func maximumContentHeight(from button: NSStatusBarButton?) -> CGFloat {
        let screen = screen(for: button)
        // Use the visible frame rather than the status item's window origin:
        // menu-bar window coordinates can extend into the menu-bar region and
        // therefore overstate the height available below the icon.
        return screen.visibleFrame.height - bottomScreenMargin
            - Bubble.arrowRoom - Bubble.inset
    }

    private func screen(for button: NSStatusBarButton?) -> NSScreen {
        let anchor = button?.window
        return anchor.flatMap { window in
            NSScreen.screens.first { $0.frame.intersects(window.frame) }
        } ?? NSScreen.main ?? NSScreen.screens[0]
    }

    func show<Content: View>(from button: NSStatusBarButton?,
                             @ViewBuilder content: () -> Content) {
        close()
        let wrapped = AnyView(content())
        let bubbleAnchor = BubbleAnchor()
        let hosting = NSHostingController(rootView: Bubble(anchor: bubbleAnchor, content: wrapped))
        let size = hosting.view.fittingSize
        let inset = Bubble.inset

        let anchor = button?.window
        let screen = screen(for: button)

        let origin: CGPoint
        let arrowX: CGFloat
        if let anchor {
            let iconCentre = anchor.frame.midX
            // The clamp NSPopover does not do.
            let lower = screen.visibleFrame.minX - inset + horizontalScreenMargin
            let upper = screen.visibleFrame.maxX - size.width + inset
                - horizontalScreenMargin
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
        bubbleAnchor.arrowX = arrowX

        // .nonactivatingPanel prevents the app becoming active at all, which is
        // what kept the controls gray. The panel the user just clicked open
        // takes focus; the tip, which appears by itself, does not.
        var style: NSWindow.StyleMask = [.borderless]
        if !activatesApp { style.insert(.nonactivatingPanel) }
        let window = KeyablePanel(contentRect: CGRect(origin: origin, size: size),
                                  styleMask: style,
                                  backing: .buffered, defer: false)
        window.contentViewController = hosting
        window.isFloatingPanel = true
        window.level = .floating
        window.backgroundColor = .clear
        window.isOpaque = false
        window.hasShadow = false               // the bubble draws its own
        window.hidesOnDeactivate = false
        // Without becoming key the controls draw in their inactive gray state.
        // A non-activating panel can be key without bringing the app forward.
        window.becomesKeyOnlyIfNeeded = false
        window.setFrame(CGRect(origin: origin, size: size), display: true)
        if activatesApp {
            NSApp.activate(ignoringOtherApps: true)
            window.makeKeyAndOrderFront(nil)
        } else {
            window.orderFrontRegardless()
        }
        panel = window

        // SwiftUI resizes the hosting window when a longer language is selected.
        // Re-clamp its position and arrow without replacing the view hierarchy.
        resizeObserver = NotificationCenter.default.addObserver(
            forName: NSWindow.didResizeNotification, object: window, queue: .main
        ) { [weak self, weak window, weak button] _ in
            Task { @MainActor in
                guard let self, let window, self.panel === window else { return }
                let screen = self.screen(for: button)
                let width = window.frame.width
                let lower = screen.visibleFrame.minX - inset + self.horizontalScreenMargin
                let upper = screen.visibleFrame.maxX - width + inset - self.horizontalScreenMargin
                let centre = button?.window?.frame.midX ?? screen.visibleFrame.midX
                let x = min(max(centre - width / 2, lower), max(lower, upper))
                let y = button?.window.map { $0.frame.minY - window.frame.height + inset + self.menuBarOverlap }
                    ?? (screen.visibleFrame.midY - window.frame.height / 2)
                window.setFrameOrigin(CGPoint(x: x, y: y))
                bubbleAnchor.arrowX = centre - x - inset
            }
        }

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
        if let observer = resizeObserver {
            NotificationCenter.default.removeObserver(observer)
            resizeObserver = nil
        }
        if let monitor = outsideClickMonitor {
            NSEvent.removeMonitor(monitor)
            outsideClickMonitor = nil
        }
        panel?.orderOut(nil)
        panel = nil
    }
}
