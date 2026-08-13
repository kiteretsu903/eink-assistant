// Running a user shortcut.
//
// Reduce Transparency and Reduce Motion cannot be set by an app: macOS refuses
// writes to com.apple.universalaccess. But the system ships Shortcuts actions
// for both:
//
//     com.apple.UniversalAccess.UASettingsShortcuts.UAToggleReduceMotionIntent
//     com.apple.UniversalAccess.UASettingsShortcuts.UAToggleTransparencyIntent
//
// So a shortcut the user builds from those actions can do what the app cannot,
// and the app can run it on their behalf.

import Foundation

enum Shortcuts {
    private static let tool = "/usr/bin/shortcuts"
    private static let key = "accessibility-shortcut"

    /// The shortcut the user picked for the accessibility toggles.
    static var chosen: String? {
        get {
            let name = UserDefaults.standard.string(forKey: key)
            return (name?.isEmpty ?? true) ? nil : name
        }
        set { UserDefaults.standard.set(newValue, forKey: key) }
    }

    static var isAvailable: Bool {
        FileManager.default.isExecutableFile(atPath: tool)
    }

    /// Names of the user's shortcuts, for the picker.
    static func list() -> [String] {
        guard isAvailable else { return [] }
        let process = Process()
        process.executableURL = URL(fileURLWithPath: tool)
        process.arguments = ["list"]
        let pipe = Pipe()
        process.standardOutput = pipe
        process.standardError = Pipe()
        guard (try? process.run()) != nil else { return [] }
        let data = pipe.fileHandleForReading.readDataToEndOfFile()
        process.waitUntilExit()
        return String(data: data, encoding: .utf8)?
            .split(separator: "\n")
            .map(String.init)
            .filter { !$0.isEmpty }
            .sorted() ?? []
    }

    /// Runs a shortcut by name. Returns false if it could not be started; the
    /// shortcut's own success is its business, not ours.
    @discardableResult
    static func run(_ name: String) -> Bool {
        guard isAvailable, !name.isEmpty else { return false }
        let process = Process()
        process.executableURL = URL(fileURLWithPath: tool)
        process.arguments = ["run", name]
        process.standardOutput = Pipe()
        process.standardError = Pipe()
        do {
            try process.run()
            return true
        } catch {
            return false
        }
    }
}
