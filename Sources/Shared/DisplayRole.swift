// Display role override: computer monitor vs television.
//
// macOS withholds Night Shift and True Tone from displays it classifies as
// televisions. Marking an e-ink panel as a television is therefore a way to
// keep those colour features off it, which matters because both of them shift
// the very tone and colour this app is trying to control.
//
// Same mechanism as the standalone script:
//     https://github.com/kiteretsu903/macos-display-role-switcher
//
// Unlike everything else in this app, this writes a system file, needs an
// administrator password, and only takes effect after a restart. It therefore
// also survives quitting, and is deliberately excluded from the quit cleanup:
// there would be no way to undo it without another password prompt and another
// restart.

import Foundation
import CoreGraphics

enum DisplayRole {
    private static let overridesRoot =
        "/Library/Displays/Contents/Resources/Overrides"

    /// Overrides are keyed by lowercase hex vendor and product ids.
    static func overridePath(for displayID: CGDirectDisplayID) -> String {
        let vendor = String(CGDisplayVendorNumber(displayID), radix: 16)
        let product = String(CGDisplayModelNumber(displayID), radix: 16)
        return "\(overridesRoot)/DisplayVendorID-\(vendor)/DisplayProductID-\(product)"
    }

    /// Any override already present. Read directly, since these files are
    /// world-readable even though writing them needs root.
    private static func existingOverride(for displayID: CGDirectDisplayID) -> [String: Any]? {
        let path = overridePath(for: displayID)
        guard let data = FileManager.default.contents(atPath: path),
              let plist = try? PropertyListSerialization.propertyList(
                  from: data, options: [], format: nil) as? [String: Any]
        else { return nil }
        return plist
    }

    /// Whether this display is currently marked as a television.
    ///
    /// An installed override wins, because that is the pending state the user
    /// will get after restarting. Otherwise fall back to what the system
    /// currently reports.
    static func isTelevision(displayID: CGDirectDisplayID) -> Bool {
        if let override = existingOverride(for: displayID),
           let value = override["DisplayIsTV"] as? Bool {
            return value
        }
        return Dither.reportedIsTelevision(displayID: displayID)
    }

    /// True when a restart is still needed for the stored setting to apply.
    static func needsRestart(displayID: CGDirectDisplayID) -> Bool {
        guard let override = existingOverride(for: displayID),
              let wanted = override["DisplayIsTV"] as? Bool
        else { return false }
        return wanted != Dither.reportedIsTelevision(displayID: displayID)
    }

    enum RoleError: Error { case serializationFailed, authorizationFailed }

    /// Writes DisplayIsTV into the display's override, preserving every other
    /// key. Custom scaled resolutions in particular live in the same file and
    /// must survive.
    static func setTelevision(_ isTV: Bool, displayID: CGDirectDisplayID) throws {
        var plist = existingOverride(for: displayID) ?? [:]
        plist["DisplayIsTV"] = isTV
        // These identify the file's target and are expected to be present.
        plist["DisplayVendorID"] = Int(CGDisplayVendorNumber(displayID))
        plist["DisplayProductID"] = Int(CGDisplayModelNumber(displayID))

        guard let data = try? PropertyListSerialization.data(
            fromPropertyList: plist, format: .xml, options: 0)
        else { throw RoleError.serializationFailed }

        let staging = FileManager.default.temporaryDirectory
            .appendingPathComponent("eink-display-override.plist")
        try data.write(to: staging)

        let destination = overridePath(for: displayID)
        let directory = (destination as NSString).deletingLastPathComponent
        let shell = "/bin/mkdir -p '\(directory)' && "
            + "/bin/cp '\(staging.path)' '\(destination)' && "
            + "/bin/chmod 644 '\(destination)'"

        // One administrator prompt, via the standard authorization dialog.
        let script = "do shell script \"\(shell.replacingOccurrences(of: "\"", with: "\\\""))\" "
            + "with administrator privileges"
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/osascript")
        process.arguments = ["-e", script]
        process.standardOutput = Pipe()
        process.standardError = Pipe()
        try process.run()
        process.waitUntilExit()
        try? FileManager.default.removeItem(at: staging)

        guard process.terminationStatus == 0 else { throw RoleError.authorizationFailed }
    }
}
