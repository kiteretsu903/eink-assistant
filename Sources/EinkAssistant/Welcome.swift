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

// MARK: - Presentation

@MainActor
enum WelcomeWindow {
    private static let suppressKey = "hide-welcome"
    private static let bubble = BubbleWindow(closesOnOutsideClick: false)
    private static var panelWatch: Timer?

    static var isSuppressed: Bool { UserDefaults.standard.bool(forKey: suppressKey) }
    static func setSuppressed(_ value: Bool) {
        UserDefaults.standard.set(value, forKey: suppressKey)
    }

    /// The status item's window is not laid out immediately after launch, and
    /// anchoring to a missing one drops the tip into the middle of the screen.
    /// So wait for a real frame before showing.
    static func showIfNeeded(retries: Int = 12) {
        guard !isSuppressed else { return }
        let anchor = AssistantDelegate.shared?.statusButton?.window
        guard let frame = anchor?.frame, frame.width > 1 else {
            guard retries > 0 else { show(); return }
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
                showIfNeeded(retries: retries - 1)
            }
            return
        }
        show()
    }

    static func show() {
        guard !bubble.isVisible else { return }
        let button = AssistantDelegate.shared?.statusButton
        bubble.show(from: button) {
            WelcomeView(close: dismissAndOpenPanel)
        }
        watchForPanel()
    }

    /// Dismisses the tip and opens the panel, so "Got it" leads somewhere
    /// rather than leaving the user to find the icon themselves.
    static func dismissAndOpenPanel() {
        close()
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.15) {
            AssistantDelegate.shared?.openPanel()
        }
    }

    /// Once the panel is open the user has found the app and the tip is only in
    /// the way, so it dismisses itself.
    private static func watchForPanel() {
        panelWatch?.invalidate()
        panelWatch = Timer.scheduledTimer(withTimeInterval: 0.4, repeats: true) { timer in
            Task { @MainActor in
                guard bubble.isVisible else { timer.invalidate(); return }
                if AssistantDelegate.shared?.isPanelOpen == true { close() }
            }
        }
    }

    static func close() {
        panelWatch?.invalidate()
        panelWatch = nil
        bubble.close()
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
