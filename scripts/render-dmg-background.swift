#!/usr/bin/env swift

import AppKit

let output = CommandLine.arguments.dropFirst().first ?? "Resources/dmg-background.png"
let size = NSSize(width: 660, height: 400)
let image = NSImage(size: size)

image.lockFocus()

let bounds = NSRect(origin: .zero, size: size)
NSGradient(colors: [
    NSColor(calibratedRed: 0.97, green: 0.98, blue: 1.00, alpha: 1),
    NSColor(calibratedRed: 0.89, green: 0.92, blue: 1.00, alpha: 1),
])!.draw(in: bounds, angle: 90)

// Very light install targets keep the icon locations obvious without
// competing with Finder's app and Applications icons.
for centerX in [170.0, 490.0] {
    let target = NSRect(x: centerX - 65, y: 115, width: 130, height: 130)
    let path = NSBezierPath(roundedRect: target, xRadius: 26, yRadius: 26)
    NSColor.white.withAlphaComponent(0.48).setFill()
    path.fill()
    NSColor(calibratedWhite: 0.30, alpha: 0.12).setStroke()
    path.lineWidth = 1
    path.stroke()
}

let paragraph = NSMutableParagraphStyle()
paragraph.alignment = .center

let title = "Install E-Ink Assistant"
title.draw(in: NSRect(x: 40, y: 330, width: 580, height: 34), withAttributes: [
    .font: NSFont.systemFont(ofSize: 26, weight: .bold),
    .foregroundColor: NSColor(calibratedWhite: 0.10, alpha: 1),
    .paragraphStyle: paragraph,
])

let instruction = "Drag E-Ink Assistant to Applications"
instruction.draw(in: NSRect(x: 40, y: 296, width: 580, height: 26), withAttributes: [
    .font: NSFont.systemFont(ofSize: 16, weight: .medium),
    .foregroundColor: NSColor(calibratedWhite: 0.24, alpha: 1),
    .paragraphStyle: paragraph,
])

let arrow = NSBezierPath()
arrow.move(to: NSPoint(x: 252, y: 180))
arrow.line(to: NSPoint(x: 408, y: 180))
arrow.lineWidth = 8
arrow.lineCapStyle = .round
NSColor(calibratedRed: 0.12, green: 0.40, blue: 0.95, alpha: 0.85).setStroke()
arrow.stroke()

let arrowHead = NSBezierPath()
arrowHead.move(to: NSPoint(x: 408, y: 180))
arrowHead.line(to: NSPoint(x: 380, y: 199))
arrowHead.line(to: NSPoint(x: 380, y: 161))
arrowHead.close()
NSColor(calibratedRed: 0.12, green: 0.40, blue: 0.95, alpha: 0.85).setFill()
arrowHead.fill()

let requirement = "macOS 14 or later  •  Apple Silicon"
requirement.draw(in: NSRect(x: 40, y: 28, width: 580, height: 22), withAttributes: [
    .font: NSFont.systemFont(ofSize: 13, weight: .regular),
    .foregroundColor: NSColor(calibratedWhite: 0.30, alpha: 1),
    .paragraphStyle: paragraph,
])

image.unlockFocus()

guard let tiff = image.tiffRepresentation,
      let bitmap = NSBitmapImageRep(data: tiff),
      let png = bitmap.representation(using: .png, properties: [:]) else {
    fatalError("Could not render DMG background")
}

try png.write(to: URL(fileURLWithPath: output))
print("Wrote \(output)")
