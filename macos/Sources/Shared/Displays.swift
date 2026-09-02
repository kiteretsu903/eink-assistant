// Display enumeration shared by the CLI and the menu bar app.

import Foundation
import CoreGraphics
import AppKit

struct Display {
    let id: CGDirectDisplayID
    let index: Int
    let isBuiltin: Bool
    let name: String
    let size: CGSize
}

func activeDisplays() -> [Display] {
    var count: UInt32 = 0
    CGGetActiveDisplayList(0, nil, &count)
    var ids = [CGDirectDisplayID](repeating: 0, count: Int(count))
    CGGetActiveDisplayList(count, &ids, &count)

    return ids.enumerated().map { index, id in
        let screen = NSScreen.screens.first {
            ($0.deviceDescription[NSDeviceDescriptionKey("NSScreenNumber")]
                as? CGDirectDisplayID) == id
        }
        return Display(
            id: id,
            index: index + 1,
            isBuiltin: CGDisplayIsBuiltin(id) != 0,
            name: screen?.localizedName ?? "Display \(id)",
            size: screen?.frame.size ?? CGDisplayBounds(id).size
        )
    }
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

