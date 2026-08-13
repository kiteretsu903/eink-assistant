// First-run tip window.
//
// A menu bar app with no Dock icon is easy to launch and then lose, so this
// says where to find it. It also carries the one thing the app cannot do for
// you: the monitor's own hardware settings have to be right before any of the
// software tuning means much.
//
// Shown on every launch until the user opts out, since the hardware advice is
// worth repeating while someone is still setting up.

import SwiftUI
import AppKit

enum WelcomeWindow {
    private static let suppressKey = "hide-welcome"
    private static var window: NSWindow?
    private static var popover: NSPopover?

    static var isSuppressed: Bool {
        UserDefaults.standard.bool(forKey: suppressKey)
    }

    static func setSuppressed(_ value: Bool) {
        UserDefaults.standard.set(value, forKey: suppressKey)
    }

    static func showIfNeeded() {
        guard !isSuppressed else { return }
        show()
    }

    /// The status item MenuBarExtra creates is not exposed to us, so it is
    /// found by class name.
    ///
    /// With separate Spaces per display there are several of these, including
    /// placeholders parked at x = 0. Taking the first one put the tip on the
    /// wrong screen, so this prefers a real slot on the active screen.
    private static func statusItemView() -> NSView? {
        let candidates = NSApp.windows.filter {
            NSStringFromClass(type(of: $0)).contains("StatusBarWindow")
                && $0.isVisible && $0.contentView != nil
        }
        // A parked placeholder sits at x = 0; a real menu bar slot does not.
        let placed = candidates.filter { $0.frame.minX > 0 }
        if let screen = NSScreen.main,
           let onActiveScreen = placed.first(where: { screen.frame.intersects($0.frame) }) {
            return onActiveScreen.contentView
        }
        return (placed.first ?? candidates.first)?.contentView
    }

    static func show() {
        // Preferred: a popover hanging off the menu bar icon, so the tip points
        // at the thing it is telling you to click.
        //
        // Deliberately without NSApp.activate: activating an accessory app with
        // a MenuBarExtra makes SwiftUI present the menu bar panel too, so the
        // panel opened on top of the tip.
        if let anchor = statusItemView() {
            if let existing = popover, existing.isShown { return }
            let popover = NSPopover()
            popover.contentViewController =
                NSHostingController(rootView: WelcomeView(close: close))
            // .transient dismisses as soon as the app resigns active, which
            // for an accessory app is immediately after launch. This one stays
            // until it is closed deliberately.
            popover.behavior = .applicationDefined
            popover.show(relativeTo: anchor.bounds, of: anchor, preferredEdge: .minY)
            self.popover = popover
            // The popover only surfaces if the app activates, but activating
            // also makes SwiftUI present the MenuBarExtra panel, which sits at
            // a higher window level and covers the tip. Dismiss just that.
            NSApp.activate(ignoringOtherApps: true)
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) { dismissMenuBarPanel() }
            return
        }

        // The fallback window does need activating to come forward.
        NSApp.activate(ignoringOtherApps: true)
        // Reuse the window if it is already up rather than stacking copies.
        if let existing = window {
            existing.makeKeyAndOrderFront(nil)
            return
        }
        let hosting = NSHostingController(rootView: WelcomeView(close: close))
        let panel = NSWindow(contentViewController: hosting)
        panel.title = ""
        panel.styleMask = [.titled, .closable]
        panel.isReleasedWhenClosed = false
        panel.center()
        window = panel
        panel.makeKeyAndOrderFront(nil)
    }

    /// Hides SwiftUI's MenuBarExtra panel without touching the status item.
    private static func dismissMenuBarPanel() {
        for window in NSApp.windows
        where NSStringFromClass(type(of: window)).contains("MenuBarExtraWindow") {
            window.orderOut(nil)
        }
    }

    static func close() {
        popover?.performClose(nil)
        popover = nil
        window?.close()
        window = nil
    }
}

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
        .frame(width: 360)
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
