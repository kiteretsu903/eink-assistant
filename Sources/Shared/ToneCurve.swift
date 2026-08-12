// Shadow-lifting tone curve.
//
// Identity above the knee, gamma-lifted below it:
//
//     w(x)   = 1 - smoothstep(0, knee, x)      1 in deep shadow, 0 at the knee
//     out(x) = (1 - w(x))*x + w(x)*x^gamma
//
// Endpoints are pinned: 0 -> 0, and anything at or above the knee passes
// through untouched, so highlights and upper midtones are left alone. The point
// is to expand the dark range that a low-contrast panel would otherwise crush
// into an undifferentiated mush.

import Foundation
import CoreGraphics

struct ToneCurve: Equatable {
    /// Above this input level the curve is exactly identity.
    var knee: Double = 0.35
    /// Shadow strength. Below 1 lifts shadows; above 1 darkens them; 1.0 is off.
    var gamma: Double = 0.60

    /// Everything below this input is crushed to pure black, and the remaining
    /// range is rescaled so white still lands on white.
    ///
    /// Aimed at text: antialiased glyph edges render as mid-greys, which a
    /// panel with few grey levels turns into fuzz rather than a smooth edge.
    /// Crushing them makes stems solid. It is the display-side equivalent of
    /// turning font smoothing down, but per display and without touching any
    /// system-wide setting.
    var blackPoint: Double = 0.0

    static let identity = ToneCurve(knee: 0.35, gamma: 1.0)

    var isIdentity: Bool {
        (abs(gamma - 1.0) < 0.001 || knee <= 0.0) && blackPoint <= 0.0
    }

    func value(_ x: Double) -> Double {
        // Black-point crush first, so the tone curve shapes what survives it.
        var v = max(0, min(x, 1))
        if blackPoint > 0 {
            v = blackPoint >= 1 ? 0 : max(0, (v - blackPoint) / (1 - blackPoint))
        }
        guard knee > 0, v > 0 else { return v }
        guard v < knee else { return min(v, 1) }
        let u = min(max(v / knee, 0), 1)
        let w = 1 - (u * u * (3 - 2 * u))   // 1 - smoothstep
        return min(max((1 - w) * v + w * pow(v, gamma), 0), 1)
    }

    /// Sampled curve for a display transfer table.
    func table(count: Int) -> [CGGammaValue] {
        (0..<count).map { i in
            CGGammaValue(value(Double(i) / Double(count - 1)))
        }
    }

    /// The ICC route has to invert this curve, which is only possible if it is
    /// strictly increasing. Worth checking rather than assuming — some
    /// parameter combinations can produce a dip.
    func isMonotonic(samples: Int = 1024) -> Bool {
        var previous = -1.0
        for i in 0..<samples {
            let v = value(Double(i) / Double(samples - 1))
            if v < previous - 1e-9 { return false }
            previous = v
        }
        return true
    }

    /// How much a given input level is lifted, for display in the UI.
    func lift(at x: Double) -> Double {
        guard x > 0 else { return 1 }
        return value(x) / x
    }
}

// MARK: - Applying live via the display transfer table

/// Applies a tone curve to one display using the public gamma-table API.
///
/// This is the *prototyping* path: it takes effect instantly with no profile
/// install, which makes live slider tuning possible. It is deliberately not the
/// shipping path — macOS resets these tables on sleep, display reconfiguration
/// and logout, so nothing set here survives.
@discardableResult
func applyToneCurveLive(_ curve: ToneCurve, displayID: CGDirectDisplayID) -> CGError {
    let count = Int(CGDisplayGammaTableCapacity(displayID))
    guard count > 1 else { return .failure }
    var table = curve.table(count: count)
    return CGSetDisplayTransferByTable(displayID, UInt32(count), &table, &table, &table)
}

/// Puts the display's transfer table back to a straight ramp.
@discardableResult
func clearToneCurveLive(displayID: CGDirectDisplayID) -> CGError {
    applyToneCurveLive(ToneCurve(knee: 0, gamma: 1.0), displayID: displayID)
}
