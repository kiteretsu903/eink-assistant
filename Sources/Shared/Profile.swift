// Per-display saturation via a synthesized ICC display profile.
//
// Why this works where compositor filters do not:
//
// macOS colour-manages every pixel from its source colour space into the
// *display's* profile, per display, at the scanout stage. That conversion is a
// full 3x3 matrix operation, so it can express the cross-channel mixing that a
// 1D gamma LUT cannot.
//
// The trick: if we tell the system the display's primaries are MORE saturated
// than they physically are, the compositor compensates by sending less
// saturated signals — the picture desaturates. Narrowing them oversaturates.
//
// The primaries are bent starting from the display's own factory profile
// (see factoryBaseProfile), not from sRGB, so the panel's measured tone curve
// and white point survive untouched.

import Foundation
import CoreGraphics
import ApplicationServices

struct Chromaticity {
    var x: Double
    var y: Double
}

/// sRGB / Rec.709 primaries and D65 white, the reference we bend away from.
enum SRGB {
    static let red = Chromaticity(x: 0.6400, y: 0.3300)
    static let green = Chromaticity(x: 0.3000, y: 0.6000)
    static let blue = Chromaticity(x: 0.1500, y: 0.0600)
    static let white = Chromaticity(x: 0.3127, y: 0.3290)
}

func xyY_to_XYZ(_ c: Chromaticity, Y: Double) -> (Double, Double, Double) {
    guard c.y != 0 else { return (0, 0, 0) }
    return (c.x * Y / c.y, Y, (1 - c.x - c.y) * Y / c.y)
}

/// Standard derivation of the RGB->XYZ matrix from primaries + white point.
/// Returns a column-major 3x3 as CoreGraphics expects.
func rgbToXYZMatrix(r: Chromaticity, g: Chromaticity, b: Chromaticity,
                    w: Chromaticity) -> [CGFloat]? {
    let (xr, yr, zr) = (r.x / r.y, 1.0, (1 - r.x - r.y) / r.y)
    let (xg, yg, zg) = (g.x / g.y, 1.0, (1 - g.x - g.y) / g.y)
    let (xb, yb, zb) = (b.x / b.y, 1.0, (1 - b.x - b.y) / b.y)
    let (xw, yw, zw) = xyY_to_XYZ(w, Y: 1.0)

    // Invert the primary matrix to find per-channel luminance scalars.
    let m = [xr, xg, xb, yr, yg, yb, zr, zg, zb]
    let det = m[0] * (m[4] * m[8] - m[5] * m[7])
            - m[1] * (m[3] * m[8] - m[5] * m[6])
            + m[2] * (m[3] * m[7] - m[4] * m[6])
    guard abs(det) > 1e-12 else { return nil }

    let inv = [
        (m[4] * m[8] - m[5] * m[7]) / det, (m[2] * m[7] - m[1] * m[8]) / det,
        (m[1] * m[5] - m[2] * m[4]) / det,
        (m[5] * m[6] - m[3] * m[8]) / det, (m[0] * m[8] - m[2] * m[6]) / det,
        (m[2] * m[3] - m[0] * m[5]) / det,
        (m[3] * m[7] - m[4] * m[6]) / det, (m[1] * m[6] - m[0] * m[7]) / det,
        (m[0] * m[4] - m[1] * m[3]) / det,
    ]
    let sr = inv[0] * xw + inv[1] * yw + inv[2] * zw
    let sg = inv[3] * xw + inv[4] * yw + inv[5] * zw
    let sb = inv[6] * xw + inv[7] * yw + inv[8] * zw

    // Column-major: column per primary.
    return [
        CGFloat(sr * xr), CGFloat(sr * yr), CGFloat(sr * zr),
        CGFloat(sg * xg), CGFloat(sg * yg), CGFloat(sg * zg),
        CGFloat(sb * xb), CGFloat(sb * yb), CGFloat(sb * zb),
    ]
}

/// Rec.709 luminance weights — the axis a saturation transform rotates around.
enum Luma {
    static let r = 0.2126
    static let g = 0.7152
    static let b = 0.0722
}

/// Inverse of the standard saturation matrix
///     S = s*I + (1-s)*1*wᵀ        (w = luminance weights)
/// By Sherman-Morrison, and using wᵀ*1 == 1, this collapses to a closed form:
///     S⁻¹ = (1/s)*I - ((1-s)/s)*1*wᵀ
/// Returned row-major.
func inverseSaturationMatrix(_ s: Double) -> [Double] {
    let a = 1.0 / s
    let k = (1.0 - s) / s
    return [
        a - k * Luma.r, -k * Luma.g, -k * Luma.b,
        -k * Luma.r, a - k * Luma.g, -k * Luma.b,
        -k * Luma.r, -k * Luma.g, a - k * Luma.b,
    ]
}

/// Builds an ICC profile that makes the compositor apply an exact saturation
/// transform.
///
/// macOS renders `displayRGB = M⁻¹ · XYZ`, and content arrives as
/// `XYZ = M_sRGB · srcRGB`. We want `displayRGB = S · srcRGB`, so we need
/// `M⁻¹ · M_sRGB = S`, i.e. the profile matrix is `M = M_sRGB · S⁻¹`.
/// That is an exact 3x3 cross-channel mix — the thing a 1D gamma LUT cannot do.
func makeSaturationProfileData(saturation: Double, gamma: Double = 2.2) -> Data? {
    // s = 0 makes S singular (rank 1); clamp just above.
    let s = max(0.001, min(saturation, 4.0))
    let w = SRGB.white
    guard let base = rgbToXYZMatrix(
        r: SRGB.red, g: SRGB.green, b: SRGB.blue, w: w) else { return nil }

    // `base` is column-major (column per primary); read it as row-major M_sRGB.
    let mSRGB = [
        Double(base[0]), Double(base[3]), Double(base[6]),
        Double(base[1]), Double(base[4]), Double(base[7]),
        Double(base[2]), Double(base[5]), Double(base[8]),
    ]
    let sInv = inverseSaturationMatrix(s)

    // M = M_sRGB · S⁻¹  (row-major multiply)
    var m = [Double](repeating: 0, count: 9)
    for row in 0..<3 {
        for col in 0..<3 {
            m[row * 3 + col] = (0..<3).reduce(0.0) {
                $0 + mSRGB[row * 3 + $1] * sInv[$1 * 3 + col]
            }
        }
    }
    // Back to the column-major layout CoreGraphics wants.
    let matrix: [CGFloat] = [
        CGFloat(m[0]), CGFloat(m[3]), CGFloat(m[6]),
        CGFloat(m[1]), CGFloat(m[4]), CGFloat(m[7]),
        CGFloat(m[2]), CGFloat(m[5]), CGFloat(m[8]),
    ]

    let (wx, wy, wz) = xyY_to_XYZ(w, Y: 1.0)
    let whitePoint: [CGFloat] = [CGFloat(wx), CGFloat(wy), CGFloat(wz)]
    let blackPoint: [CGFloat] = [0, 0, 0]
    let gammaV: [CGFloat] = [CGFloat(gamma), CGFloat(gamma), CGFloat(gamma)]

    guard let space = CGColorSpace(
        calibratedRGBWhitePoint: whitePoint,
        blackPoint: blackPoint,
        gamma: gammaV,
        matrix: matrix
    ) else { return nil }

    return space.copyICCData() as Data?
}

// MARK: - Installing the profile on one display

let profileDirectory = URL(fileURLWithPath: NSHomeDirectory())
    .appendingPathComponent("Library/ColorSync/Profiles")

func installProfile(_ data: Data, displayID: CGDirectDisplayID, tag: String) throws -> URL {
    try FileManager.default.createDirectory(
        at: profileDirectory, withIntermediateDirectories: true)
    let url = profileDirectory.appendingPathComponent("satctl-\(tag).icc")
    try data.write(to: url)

    guard let uuid = CGDisplayCreateUUIDFromDisplayID(displayID)?.takeRetainedValue() else {
        throw SatError.noDisplayUUID
    }
    let info: [CFString: Any] = [
        kColorSyncDeviceDefaultProfileID.takeUnretainedValue(): url as CFURL
    ]
    let ok = ColorSyncDeviceSetCustomProfiles(
        kColorSyncDisplayDeviceClass.takeUnretainedValue(),
        uuid,
        info as CFDictionary
    )
    guard ok else { throw SatError.colorSyncRejected }
    return url
}

/// Drops our override and lets the display fall back to its factory profile.
func restoreProfile(displayID: CGDirectDisplayID) {
    guard let uuid = CGDisplayCreateUUIDFromDisplayID(displayID)?.takeRetainedValue() else {
        return
    }
    // kCFNull is what actually clears the override. Passing a URL — or, as an
    // earlier version did, a key constant — just reassigns it, leaving the
    // custom profile in place and "reset" doing nothing.
    let info: [CFString: Any] = [
        kColorSyncDeviceDefaultProfileID.takeUnretainedValue(): kCFNull
    ]
    _ = ColorSyncDeviceSetCustomProfiles(
        kColorSyncDisplayDeviceClass.takeUnretainedValue(), uuid, info as CFDictionary)
}

enum SatError: Error {
    case noDisplayUUID
    case colorSyncRejected
    case profileBuildFailed
}

// MARK: - Naming the profile

/// CoreGraphics stamps every profile it emits with the generic name
/// "CG Cal RGB", which makes our profiles indistinguishable from each other in
/// System Settings. This rewrites the ICC `desc` tag so each one is
/// identifiable, rebuilding the tag table since the tag changes size.
func setProfileDescription(_ data: Data, to name: String) -> Data? {
    let bytes = [UInt8](data)
    guard bytes.count > 132 else { return nil }

    func u32(_ o: Int) -> Int {
        (Int(bytes[o]) << 24) | (Int(bytes[o + 1]) << 16)
            | (Int(bytes[o + 2]) << 8) | Int(bytes[o + 3])
    }
    func be32(_ v: Int) -> [UInt8] {
        [UInt8((v >> 24) & 0xff), UInt8((v >> 16) & 0xff),
         UInt8((v >> 8) & 0xff), UInt8(v & 0xff)]
    }

    let tagCount = u32(128)
    guard tagCount > 0, 132 + tagCount * 12 <= bytes.count else { return nil }

    // Build a replacement `desc` as a single-record 'mluc' (en-US, UTF-16BE).
    let utf16 = Array(name.utf16).flatMap { [UInt8($0 >> 8), UInt8($0 & 0xff)] }
    var desc: [UInt8] = Array("mluc".utf8) + be32(0) + be32(1) + be32(12)
    desc += Array("enUS".utf8) + be32(utf16.count) + be32(28) + utf16

    // Collect tags, substituting our new desc.
    var tags: [(sig: [UInt8], payload: [UInt8])] = []
    for i in 0..<tagCount {
        let e = 132 + i * 12
        let sig = Array(bytes[e..<(e + 4)])
        let off = u32(e + 4), size = u32(e + 8)
        guard off + size <= bytes.count else { return nil }
        let payload = String(bytes: sig, encoding: .utf8) == "desc"
            ? desc
            : Array(bytes[off..<(off + size)])
        tags.append((sig, payload))
    }

    // Reassemble. Identical payloads (the shared *TRC curves) are stored once,
    // matching how CoreGraphics emitted them.
    var table: [UInt8] = []
    var body: [UInt8] = []
    let bodyStart = 132 + tagCount * 12
    var seen: [String: Int] = [:]

    for tag in tags {
        let key = tag.payload.map { String($0) }.joined(separator: ",")
        let offset: Int
        if let existing = seen[key] {
            offset = existing
        } else {
            offset = bodyStart + body.count
            seen[key] = offset
            body += tag.payload
            while body.count % 4 != 0 { body.append(0) }  // 4-byte alignment
        }
        table += tag.sig + be32(offset) + be32(tag.payload.count)
    }

    var out = Array(bytes[0..<128]) + be32(tagCount) + table + body
    out.replaceSubrange(0..<4, with: be32(out.count))  // header size field
    return Data(out)
}

// MARK: - Deriving from the display's own factory profile

/// The parts of a display's ICC profile we need in order to add saturation on
/// top of it rather than replacing its characterization with a guess.
struct BaseProfile {
    var matrix: [Double]      // row-major RGB->XYZ
    var whitePoint: [Double]  // XYZ
    var gamma: Double
}

/// Parses primaries, white point and tone curve out of an ICC profile.
/// Only the matrix/TRC form is handled — which is what macOS emits for displays.
func parseBaseProfile(_ data: Data) -> BaseProfile? {
    let b = [UInt8](data)
    guard b.count > 132 else { return nil }
    func u32(_ o: Int) -> Int {
        (Int(b[o]) << 24) | (Int(b[o+1]) << 16) | (Int(b[o+2]) << 8) | Int(b[o+3])
    }
    func s15(_ o: Int) -> Double {
        Double(Int32(bitPattern: UInt32(u32(o)))) / 65536.0
    }

    let count = u32(128)
    guard count > 0, 132 + count * 12 <= b.count else { return nil }

    var xyz: [String: [Double]] = [:]
    var gamma: Double?

    for i in 0..<count {
        let e = 132 + i * 12
        guard let sig = String(bytes: b[e..<(e+4)], encoding: .utf8) else { continue }
        let off = u32(e + 4), size = u32(e + 8)
        guard off + size <= b.count, size >= 12 else { continue }

        switch sig {
        case "rXYZ", "gXYZ", "bXYZ", "wtpt":
            xyz[sig] = [s15(off + 8), s15(off + 12), s15(off + 16)]
        case "rTRC":
            let type = String(bytes: b[off..<(off+4)], encoding: .utf8) ?? ""
            if type == "para" {
                // Type 0 is a plain gamma exponent; other types start with it too.
                gamma = s15(off + 12)
            } else if type == "curv", u32(off + 8) == 1 {
                gamma = Double((Int(b[off+12]) << 8) | Int(b[off+13])) / 256.0
            }
        default:
            break
        }
    }

    guard let r = xyz["rXYZ"], let g = xyz["gXYZ"],
          let bl = xyz["bXYZ"], let w = xyz["wtpt"] else { return nil }

    return BaseProfile(
        matrix: [r[0], g[0], bl[0],
                 r[1], g[1], bl[1],
                 r[2], g[2], bl[2]],
        whitePoint: w,
        gamma: gamma ?? 2.2
    )
}

/// The display's factory profile — the panel's real characterization, taken
/// before any override this tool installed.
func factoryBaseProfile(displayID: CGDirectDisplayID) -> BaseProfile? {
    guard let uuid = CGDisplayCreateUUIDFromDisplayID(displayID)?.takeRetainedValue(),
          let info = ColorSyncDeviceCopyDeviceInfo(
              kColorSyncDisplayDeviceClass.takeUnretainedValue(), uuid)?
              .takeRetainedValue() as? [String: Any],
          let factory = info[kColorSyncFactoryProfiles.takeUnretainedValue() as String]
              as? [String: Any]
    else { return nil }

    // FactoryProfiles is keyed by device mode ("HDMI HD", "HDMI SD"), plus a
    // DeviceDefaultProfileID entry naming which mode is actually in use. Each
    // mode maps to a dictionary holding DeviceProfileURL.
    let urlKey = kColorSyncDeviceProfileURL.takeUnretainedValue() as String
    let defaultKey = kColorSyncDeviceDefaultProfileID.takeUnretainedValue() as String

    func url(inMode value: Any) -> URL? {
        guard let entry = value as? [String: Any], let raw = entry[urlKey] else { return nil }
        if let u = raw as? URL { return u }
        if let s = raw as? String { return URL(string: s) }
        return nil
    }

    // Prefer the mode the display is actually running in.
    var candidates: [Any] = []
    if let mode = factory[defaultKey] as? String, let entry = factory[mode] {
        candidates.append(entry)
    }
    candidates.append(contentsOf: factory.filter { $0.key != defaultKey }.map { $0.value })

    for candidate in candidates {
        guard let u = url(inMode: candidate),
              // Never derive from a profile this tool installed, or saturation
              // would compound every time `set` is run.
              !u.lastPathComponent.hasPrefix("satctl-"),
              let data = try? Data(contentsOf: u),
              let base = parseBaseProfile(data)
        else { continue }
        return base
    }
    return nil
}

/// Applies saturation on top of an existing display characterization, keeping
/// its primaries, white point and tone curve intact. `M_new = M_display · S⁻¹`.
func makeSaturationProfileData(saturation: Double, base: BaseProfile) -> Data? {
    let s = max(0.001, min(saturation, 4.0))
    let sInv = inverseSaturationMatrix(s)

    var m = [Double](repeating: 0, count: 9)
    for row in 0..<3 {
        for col in 0..<3 {
            m[row * 3 + col] = (0..<3).reduce(0.0) {
                $0 + base.matrix[row * 3 + $1] * sInv[$1 * 3 + col]
            }
        }
    }
    let matrix: [CGFloat] = [
        CGFloat(m[0]), CGFloat(m[3]), CGFloat(m[6]),
        CGFloat(m[1]), CGFloat(m[4]), CGFloat(m[7]),
        CGFloat(m[2]), CGFloat(m[5]), CGFloat(m[8]),
    ]
    let wp = base.whitePoint.map { CGFloat($0) }
    let gammaV = [CGFloat](repeating: CGFloat(base.gamma), count: 3)

    guard let space = CGColorSpace(
        calibratedRGBWhitePoint: wp,
        blackPoint: [0, 0, 0],
        gamma: gammaV,
        matrix: matrix
    ) else { return nil }
    return space.copyICCData() as Data?
}

// MARK: - The one apply path

/// Applies a saturation factor to one display, shared by the CLI and the menu
/// bar app so both behave identically. Returns the installed profile's name, or
/// nil when the adjustment was removed rather than installed.
@discardableResult
func applySaturation(_ amount: Double, displayID: CGDirectDisplayID,
                     displayName: String) throws -> String? {
    // Exactly 1.0 means "no adjustment" — drop the override rather than leave
    // an identity profile sitting over the display's real calibration.
    guard abs(amount - 1.0) > 0.001 else {
        restoreProfile(displayID: displayID)
        return nil
    }
    // Prefer the panel's own characterization so we add saturation on top of
    // its real primaries and tone curve instead of imposing sRGB assumptions.
    let base = factoryBaseProfile(displayID: displayID)
    let built = base.flatMap { makeSaturationProfileData(saturation: amount, base: $0) }
        ?? makeSaturationProfileData(saturation: amount)
    guard let raw = built else { throw SatError.profileBuildFailed }

    let label = "\(displayName) — Saturation \(Int((amount * 100).rounded()))%"
    let data = setProfileDescription(raw, to: label) ?? raw
    _ = try installProfile(data, displayID: displayID, tag: "\(displayID)")
    return label
}

// MARK: - Reading back what is actually installed

/// Pulls the human-readable name out of an ICC profile's `desc` tag.
func profileDescription(_ data: Data) -> String? {
    let b = [UInt8](data)
    guard b.count > 132 else { return nil }
    func u32(_ o: Int) -> Int {
        (Int(b[o]) << 24) | (Int(b[o+1]) << 16) | (Int(b[o+2]) << 8) | Int(b[o+3])
    }
    let count = u32(128)
    guard count > 0, 132 + count * 12 <= b.count else { return nil }

    for i in 0..<count {
        let e = 132 + i * 12
        guard String(bytes: b[e..<(e+4)], encoding: .utf8) == "desc" else { continue }
        let off = u32(e + 4), size = u32(e + 8)
        guard off + size <= b.count, size > 28 else { return nil }
        guard String(bytes: b[off..<(off+4)], encoding: .utf8) == "mluc" else { return nil }
        let length = u32(off + 20), strOff = u32(off + 24)
        guard off + strOff + length <= b.count else { return nil }
        return String(bytes: b[(off + strOff)..<(off + strOff + length)], encoding: .utf16BigEndian)
    }
    return nil
}

/// The saturation currently applied to a display, recovered from the profile
/// actually assigned to it.
///
/// This is the source of truth, not a stored preference: CGDirectDisplayID is
/// not stable across reboots or reconnections, so anything keyed on it can go
/// stale and leave the UI disagreeing with the display.
func installedSaturation(displayID: CGDirectDisplayID) -> Double? {
    guard let uuid = CGDisplayCreateUUIDFromDisplayID(displayID)?.takeRetainedValue(),
          let info = ColorSyncDeviceCopyDeviceInfo(
              kColorSyncDisplayDeviceClass.takeUnretainedValue(), uuid)?
              .takeRetainedValue() as? [String: Any],
          let custom = info[kColorSyncCustomProfiles.takeUnretainedValue() as String]
              as? [String: Any]
    else { return nil }

    for (_, value) in custom {
        let url: URL? = (value as? URL) ?? (value as? String).flatMap { URL(string: $0) }
        guard let u = url, let data = try? Data(contentsOf: u),
              let name = profileDescription(data),
              let range = name.range(of: #"Saturation ([0-9.]+)%"#, options: .regularExpression)
        else { continue }
        let digits = name[range]
            .replacingOccurrences(of: "Saturation ", with: "")
            .replacingOccurrences(of: "%", with: "")
        if let percent = Double(digits) { return percent / 100.0 }
    }
    return nil
}
