// Per-display dithering control.
//
// macOS dithers display output to smooth gradients. On a colour e-ink panel the
// dither pattern is visible as a constant shimmer, because the panel holds each
// pixel rather than refreshing it away.
//
// The mechanism here (setting `enableDither` on IOMobileFramebufferAP registry
// entries) comes from Stillcolor by Abdullah Arif, MIT licensed:
//
//     https://github.com/aiaf/Stillcolor
//
// Stillcolor applies the setting to all displays, or to embedded/external as a
// group. This narrows it to a single display by matching each framebuffer to a
// CGDirectDisplayID, so an e-ink panel can have dithering off while the
// built-in screen keeps it.

import Foundation
import IOKit
import CoreGraphics

enum Dither {
    private static let serviceClass = "IOMobileFramebufferAP"
    private static let key = "enableDither"

    private static func property(_ name: String, _ entry: io_registry_entry_t) -> CFTypeRef? {
        IORegistryEntrySearchCFProperty(entry, kIOServicePlane, name as CFString,
                                        kCFAllocatorDefault, IOOptionBits(0))
    }

    /// A three-letter EISA/PNP manufacturer code as the number CoreGraphics
    /// reports for the same display. "CPO" and 3599 are the same vendor.
    private static func eisaToVendor(_ code: String) -> UInt32? {
        let scalars = code.uppercased().unicodeScalars.map { $0.value }
        guard scalars.count == 3, scalars.allSatisfy({ $0 >= 65 && $0 <= 90 }) else { return nil }
        return ((scalars[0] - 64) << 10) | ((scalars[1] - 64) << 5) | (scalars[2] - 64)
    }

    private static func productAttributes(_ service: io_service_t) -> [String: Any]? {
        guard let display = property("DisplayAttributes", service) as? [String: Any] else {
            return nil
        }
        return display["ProductAttributes"] as? [String: Any]
    }

    /// Whether a framebuffer entry is the one backing `displayID`.
    private static func matches(_ service: io_service_t, _ displayID: CGDirectDisplayID) -> Bool {
        let isExternal = (property("external", service) as? Bool) ?? false
        let wantBuiltin = CGDisplayIsBuiltin(displayID) != 0

        // The built-in panel is identified by position: its registry entry
        // carries no usable product attributes.
        if wantBuiltin { return !isExternal }
        guard isExternal, let attributes = productAttributes(service) else { return false }

        guard let productID = attributes["ProductID"] as? Int,
              UInt32(productID) == CGDisplayModelNumber(displayID) else { return false }

        // Product id alone can collide across vendors, so confirm the vendor
        // when the entry exposes one.
        if let code = attributes["ManufacturerID"] as? String,
           let vendor = eisaToVendor(code) {
            return vendor == CGDisplayVendorNumber(displayID)
        }
        return true
    }

    /// Runs `body` against the framebuffer backing `displayID`, handling the
    /// iterator and release so callers cannot leak a registry entry.
    @discardableResult
    private static func withFramebuffer<T>(for displayID: CGDirectDisplayID,
                                           _ body: (io_service_t) -> T) -> T? {
        var iterator = io_iterator_t()
        guard IOServiceGetMatchingServices(kIOMainPortDefault,
                                           IOServiceMatching(serviceClass),
                                           &iterator) == KERN_SUCCESS,
              iterator != IO_OBJECT_NULL
        else { return nil }
        defer { IOObjectRelease(iterator) }

        var result: T?
        while case let service = IOIteratorNext(iterator), service != 0 {
            if result == nil, matches(service, displayID) {
                result = body(service)
            }
            IOObjectRelease(service)
        }
        return result
    }

    /// True when this display's framebuffer was found and reports dithering off.
    static func isDisabled(displayID: CGDirectDisplayID) -> Bool {
        withFramebuffer(for: displayID) { service in
            (property(key, service) as? Bool) == false
        } ?? false
    }

    /// What the system currently reports for this display's role. The override
    /// on disk may differ until the machine is restarted.
    static func reportedIsTelevision(displayID: CGDirectDisplayID) -> Bool {
        withFramebuffer(for: displayID) { service in
            (property("DisplayIsTV", service) as? Bool) ?? false
        } ?? false
    }

    /// Whether a framebuffer could be matched at all. Used to hide the control
    /// on hardware where this does not apply, rather than offering a dead toggle.
    static func isSupported(displayID: CGDirectDisplayID) -> Bool {
        withFramebuffer(for: displayID) { _ in true } ?? false
    }

    @discardableResult
    static func setDisabled(_ disabled: Bool, displayID: CGDirectDisplayID) -> Bool {
        withFramebuffer(for: displayID) { service in
            let value: CFBoolean = disabled ? kCFBooleanFalse : kCFBooleanTrue
            // Skip a redundant write; the property survives across processes.
            if let current = property(key, service) as? Bool, current == !disabled {
                return true
            }
            return IORegistryEntrySetCFProperty(service, key as CFString, value) == KERN_SUCCESS
        } ?? false
    }

    /// Turns dithering back on everywhere. Used on quit so the app leaves no
    /// hardware state behind, and it deliberately needs no app state.
    static func restoreAll() {
        var iterator = io_iterator_t()
        guard IOServiceGetMatchingServices(kIOMainPortDefault,
                                           IOServiceMatching(serviceClass),
                                           &iterator) == KERN_SUCCESS,
              iterator != IO_OBJECT_NULL
        else { return }
        defer { IOObjectRelease(iterator) }

        while case let service = IOIteratorNext(iterator), service != 0 {
            if (property(key, service) as? Bool) == false {
                IORegistryEntrySetCFProperty(service, key as CFString, kCFBooleanTrue)
            }
            IOObjectRelease(service)
        }
    }
}
