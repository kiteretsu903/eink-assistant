// Display enumeration shared by the CLI and the menu bar app.

import Foundation
import CoreGraphics
import AppKit
import ColorSync

struct Display {
    let id: CGDirectDisplayID
    let index: Int
    let isBuiltin: Bool
    let name: String
    let size: CGSize
}

private func displayIDs(
    from getter: (UInt32, UnsafeMutablePointer<CGDirectDisplayID>?,
                  UnsafeMutablePointer<UInt32>?) -> CGError
) -> [CGDirectDisplayID] {
    var count: UInt32 = 0
    guard getter(0, nil, &count) == .success, count > 0 else { return [] }
    var ids = [CGDirectDisplayID](repeating: 0, count: Int(count))
    guard getter(count, &ids, &count) == .success else { return [] }
    return Array(ids.prefix(Int(count)))
}

func activeDisplayIDs() -> [CGDirectDisplayID] {
    displayIDs(from: CGGetActiveDisplayList)
}

/// Includes active, mirrored, and sleeping displays. Display cleanup needs
/// this broader list because a hardware-mirrored secondary is online but not
/// necessarily active or drawable.
func onlineDisplayIDs() -> [CGDirectDisplayID] {
    displayIDs(from: CGGetOnlineDisplayList)
}

private func colorSyncDisplayName(_ id: CGDirectDisplayID) -> String? {
    guard let uuid = CGDisplayCreateUUIDFromDisplayID(id)?.takeRetainedValue(),
          let info = ColorSyncDeviceCopyDeviceInfo(
            kColorSyncDisplayDeviceClass.takeUnretainedValue(), uuid
          )?.takeRetainedValue() as? [String: Any]
    else { return nil }

    let key = kColorSyncDeviceDescription.takeUnretainedValue() as String
    guard let name = info[key] as? String, !name.isEmpty else { return nil }
    return name
}

private func displays(for ids: [CGDirectDisplayID]) -> [Display] {
    ids.enumerated().map { index, id in
        let screen = NSScreen.screens.first {
            ($0.deviceDescription[NSDeviceDescriptionKey("NSScreenNumber")]
                as? CGDirectDisplayID) == id
        }
        return Display(
            id: id,
            index: index + 1,
            isBuiltin: CGDisplayIsBuiltin(id) != 0,
            name: screen?.localizedName ?? colorSyncDisplayName(id) ?? "Display \(id)",
            size: screen?.frame.size ?? CGDisplayBounds(id).size
        )
    }
}

func activeDisplays() -> [Display] {
    displays(for: activeDisplayIDs())
}

/// The displays the main app can manage. Hardware mirror secondaries are
/// online but absent from CGGetActiveDisplayList, so add those mirror members
/// without also surfacing unrelated sleeping displays such as a closed
/// built-in panel.
func controllableDisplays() -> [Display] {
    let active = activeDisplayIDs()
    let activeSet = Set(active)
    let hiddenMirrorMembers = onlineDisplayIDs().filter {
        !activeSet.contains($0) && CGDisplayIsInMirrorSet($0) != 0
    }
    return displays(for: active + hiddenMirrorMembers)
}

func display(at index: Int) -> Display? {
    activeDisplays().first { $0.index == index }
}

/// Renders a percentage without a trailing ".0" so profile names read as
/// "Saturation 130%" rather than "Saturation 130.0%".
func formatted(_ value: Double) -> String {
    value == value.rounded()
        ? String(Int(value.rounded()))
        : String(format: "%g", value)
}
