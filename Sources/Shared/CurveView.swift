// Curve plot shared by the tuning labs.

import SwiftUI
import CoreGraphics

// MARK: - Curve plot

struct CurvePlot: View {
    let curve: ToneCurve
    /// The labs want a large plot; the app panel wants a compact one.
    var height: CGFloat = 150

    var body: some View {
        Canvas { context, size in
            func point(_ x: Double, _ y: Double) -> CGPoint {
                CGPoint(x: x * size.width, y: (1 - y) * size.height)
            }
            // Identity reference.
            var identity = Path()
            identity.move(to: point(0, 0))
            identity.addLine(to: point(1, 1))
            context.stroke(identity, with: .color(.secondary.opacity(0.35)),
                           style: StrokeStyle(lineWidth: 1, dash: [3, 3]))

            // Knee marker: everything right of this line is untouched.
            var knee = Path()
            knee.move(to: point(curve.knee, 0))
            knee.addLine(to: point(curve.knee, 1))
            context.stroke(knee, with: .color(.orange.opacity(0.5)),
                           style: StrokeStyle(lineWidth: 1))

            // The curve itself.
            var path = Path()
            let steps = 256
            for i in 0...steps {
                let x = Double(i) / Double(steps)
                let p = point(x, curve.value(x))
                if i == 0 { path.move(to: p) } else { path.addLine(to: p) }
            }
            context.stroke(path, with: .color(.accentColor),
                           style: StrokeStyle(lineWidth: 2, lineJoin: .round))
        }
        .frame(height: height)
        .background(Color.primary.opacity(0.04))
        .overlay(RoundedRectangle(cornerRadius: 4)
            .stroke(Color.secondary.opacity(0.25), lineWidth: 1))
        .clipShape(RoundedRectangle(cornerRadius: 4))
    }
}

