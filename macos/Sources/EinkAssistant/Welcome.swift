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
        let maximumHeight = bubble.maximumContentHeight(from: button)
        bubble.show(from: button) {
            HeightLimitedLocalizationView(width: 480, maximumHeight: maximumHeight) {
                WelcomeView(model: .shared, close: dismissAndOpenPanel)
            }
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

struct WelcomeView: View {
    @ObservedObject var model: AssistantModel
    let close: () -> Void
    @State private var suppress = WelcomeWindow.isSuppressed

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text(L("welcome.title"))
                .font(.system(size: 22, weight: .semibold))

            VStack(alignment: .leading, spacing: 12) {
                tip("menubar.arrow.up.rectangle",
                    L("welcome.menubar.title"), L("welcome.menubar"))
                tip("display", L("welcome.bigme.title"), L("welcome.bigme.body"))
                tip("slider.horizontal.3",
                    L("welcome.other.title"), L("welcome.other.body"))
            }

            Divider()

            HStack {
                Toggle(L("welcome.hide"), isOn: Binding(
                    get: { suppress },
                    set: { suppress = $0; WelcomeWindow.setSuppressed($0) }
                ))
                .toggleStyle(EinkCheckboxToggleStyle())
                Spacer()
                Button(L("welcome.done")) { close() }
                    .buttonStyle(EinkOutlinedButtonStyle(
                        foreground: .accentColor))
                    .keyboardShortcut(.defaultAction)
            }
        }
        .font(.system(size: 16))
        .padding(24)
        .frame(width: 480)
        .environment(\.locale, Locale(identifier: Localization.resource))
        .environment(\.layoutDirection, Localization.isRightToLeft ? .rightToLeft : .leftToRight)
    }

    private func tip(_ symbol: String, _ title: String, _ body: String) -> some View {
        HStack(alignment: .top, spacing: 12) {
            ZStack {
                RoundedRectangle(cornerRadius: 8)
                    .stroke(EinkPalette.areaOutline, lineWidth: 2)
                    .frame(width: 38, height: 38)
                Image(systemName: symbol)
                    .font(.system(size: 17, weight: .semibold))
                    .foregroundStyle(Color.accentColor)
            }
            VStack(alignment: .leading, spacing: 2) {
                Text(title).font(.system(size: 16, weight: .semibold))
                Text(body)
                    .font(.system(size: 14))
                    .foregroundStyle(EinkPalette.secondaryText)
                    .fixedSize(horizontal: false, vertical: true)
            }
        }
        .accessibilityElement(children: .combine)
    }
}
