// Display role override: computer monitor vs television.
//
// macOS withholds Night Shift and True Tone from displays it classifies as
// televisions. Marking an e-ink panel as a television is therefore a way to
// keep those color features off it, which matters because both of them shift
// the very tone and color this app is trying to control.
//
// Same mechanism as the standalone script:
//     https://github.com/kiteretsu903/macos-display-role-switcher
//
// Unlike everything else in this app, this writes a system file, needs an
// administrator password, and takes effect after the display is disconnected
// and reconnected. It survives quitting and is deliberately excluded from quit
// cleanup: undoing it also needs a password and another display reconnect.

import Foundation
import CoreGraphics
import IOKit

enum DisplayRole {
    private static let overridesRoot =
        "/Library/Displays/Contents/Resources/Overrides"
    private static let pendingPrefix = "role-reconnect-pending-"
    private static let displayIDPrefix = "role-reconnect-display-id-"

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
    /// An installed override wins because it is the user's chosen state.
    /// Otherwise fall back to the current display-mode classification.
    static func isTelevision(displayID: CGDirectDisplayID) -> Bool {
        if let override = existingOverride(for: displayID),
           let value = override["DisplayIsTV"] as? Bool {
            return value
        }
        return liveIsTelevision(displayID: displayID) ?? false
    }

    /// The active mode's television-output classification. This is useful as a
    /// fallback when no override exists, but it is not an applied-state signal:
    /// on Apple Silicon macOS can consume `DisplayIsTV` for CoreBrightness while
    /// leaving this mode flag unset.
    static func liveIsTelevision(displayID: CGDirectDisplayID) -> Bool? {
        guard let mode = CGDisplayCopyDisplayMode(displayID) else { return nil }
        return (mode.ioFlags & UInt32(kDisplayModeTelevisionFlag)) != 0
    }

    /// True after this app writes an override and until it observes the display
    /// being added again. macOS exposes no reliable per-display status for
    /// whether CoreBrightness has consumed `DisplayIsTV`, so the app reports the
    /// reconnect action it can observe instead of inventing an applied state.
    static func needsReconnect(displayID: CGDirectDisplayID) -> Bool {
        guard let uuid = displayUUIDString(displayID) else { return false }
        let defaults = UserDefaults.standard
        let pendingKey = pendingPrefix + uuid
        guard defaults.bool(forKey: pendingKey) else { return false }

        // Display IDs normally change across a reconnect. This also clears a
        // pending marker when the reconnect occurred while the app was closed.
        let writtenID = defaults.object(forKey: displayIDPrefix + uuid) as? NSNumber
        if let writtenID, writtenID.uint32Value != displayID {
            clearReconnectNeeded(displayID: displayID)
            return false
        }
        return true
    }

    static func markReconnectNeeded(displayID: CGDirectDisplayID) {
        guard let uuid = displayUUIDString(displayID) else { return }
        let defaults = UserDefaults.standard
        defaults.set(true, forKey: pendingPrefix + uuid)
        defaults.set(Int(displayID), forKey: displayIDPrefix + uuid)
    }

    static func clearReconnectNeeded(displayID: CGDirectDisplayID) {
        guard let uuid = displayUUIDString(displayID) else { return }
        let defaults = UserDefaults.standard
        defaults.removeObject(forKey: pendingPrefix + uuid)
        defaults.removeObject(forKey: displayIDPrefix + uuid)
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
