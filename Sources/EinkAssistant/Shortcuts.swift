// App-owned accessibility helper.
//
// macOS refuses direct writes to com.apple.universalaccess, but Shortcuts has
// first-party actions for changing Reduce Motion and Reduce Transparency. The
// app ships one signed shortcut containing only these two actions and a strict
// text protocol: "off" turns both off, "on" turns both on, and missing or
// unrecognized input does nothing.
// Importing a shortcut always requires one confirmation in Shortcuts; after
// that the app invokes this fixed helper by name.
//
// Deliberately do not list or inspect the user's shortcuts. A successful run is
// the only installation check, and the helper uses explicit On/Off actions
// rather than toggles, so running either state repeatedly is safe.

import Foundation
import AppKit

enum AccessibilityHelperState: String {
    case on, off
}

enum Shortcuts {
    static let helperName = "E-Ink Assistant Accessibility Helper"
    static let helperResource = "E-Ink Assistant Accessibility Helper"

    private static let tool = "/usr/bin/shortcuts"
    private static let installedKey = "accessibility-helper-version"
    private static let legacyInstalledKey = "accessibility-helper-installed"
    // v5 accepts Text only, recognizes exact "on" and "off" commands, ignores
    // everything else, and ends with the Nothing action. That explicit final
    // action prevents Shortcuts from implicitly returning the input command as
    // output even though Provide Output is disabled. The helper is also hidden
    // from Share Sheet, Spotlight, Quick Actions, and locked-screen execution.
    private static let currentVersion = 5

    static var isAvailable: Bool {
        FileManager.default.isExecutableFile(atPath: tool)
    }

    /// This is only a UI hint. It becomes true after this app successfully
    /// runs its helper; it is never inferred by enumerating the user's library.
    /// Older builds stored a Boolean. Migrate it once so rebuilding the app
    /// does not make an already-imported helper appear to be missing.
    static var wasInstalled: Bool {
        get {
            let defaults = UserDefaults.standard
            if defaults.integer(forKey: installedKey) == currentVersion {
                return true
            }
            if defaults.bool(forKey: legacyInstalledKey) {
                defaults.set(currentVersion, forKey: installedKey)
                defaults.removeObject(forKey: legacyInstalledKey)
                return true
            }
            return false
        }
        set {
            if newValue {
                UserDefaults.standard.set(currentVersion, forKey: installedKey)
                UserDefaults.standard.removeObject(forKey: legacyInstalledKey)
            } else {
                UserDefaults.standard.removeObject(forKey: installedKey)
                UserDefaults.standard.removeObject(forKey: legacyInstalledKey)
            }
        }
    }

    static var installerURL: URL? {
        Bundle.main.url(forResource: helperResource, withExtension: "shortcut")
    }

    /// Runs the helper without input and waits for a completion status. Missing
    /// input is deliberately a no-op, so this safely verifies the post-import
    /// helper without enumerating the user's shortcut library.
    static func verifyInstalledAndWait() -> Bool {
        guard isAvailable else { return false }
        let process = Process()
        process.executableURL = URL(fileURLWithPath: tool)
        process.arguments = ["run", helperName]
        process.standardOutput = Pipe()
        process.standardError = Pipe()
        do {
            try process.run()
            process.waitUntilExit()
            return process.terminationStatus == 0
        } catch {
            return false
        }
    }

    /// Runs either state through Apple's documented Shortcuts URL scheme. URL
    /// input is text (unlike the CLI's --input-path, which passes a file), so
    /// the helper can reliably branch on the literal "off" value.
    @MainActor
    static func run(_ state: AccessibilityHelperState) -> Bool {
        var components = URLComponents()
        components.scheme = "shortcuts"
        components.host = "run-shortcut"
        components.queryItems = [
            URLQueryItem(name: "name", value: helperName),
            URLQueryItem(name: "input", value: "text"),
            URLQueryItem(name: "text", value: state.rawValue),
        ]
        guard let url = components.url else { return false }
        return NSWorkspace.shared.open(url)
    }
}
