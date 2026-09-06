// Standalone preview executable; never instantiate AssistantDelegate or .shared.
import AppKit
import SwiftUI

@MainActor
struct LocalizationPreviewView: View {
    @ObservedObject var model: AssistantModel
    var body: some View {
        VStack(alignment: .leading) {
            Text("Localization preview — fictional display; controls disabled")
                .font(.headline)
            Picker("Preview language", selection: Binding(get: { model.language }, set: { model.setLanguage($0) })) {
                ForEach(AppLanguage.allCases.filter { $0 != .system }) { locale in
                    Text(locale.label).tag(locale)
                }
            }
            HStack(alignment: .top) {
                ScrollView { AssistantView(model: model).disabled(true) }
                    .frame(width: 540)
                ScrollView { WelcomeView(model: model, close: {}).disabled(true) }
            }
        }
        .padding()
        .environment(\.locale, Locale(identifier: Localization.resource))
        .environment(\.layoutDirection, Localization.isRightToLeft ? .rightToLeft : .leftToRight)
    }
}

@main
@MainActor
enum LocalizationPreview {
    static func main() throws {
        let arguments = CommandLine.arguments
        func option(_ name: String) -> String? {
            guard let index = arguments.firstIndex(of: name), index + 1 < arguments.count else { return nil }
            return arguments[index + 1]
        }
        let initial = AppLanguage(rawValue: option("--locale") ?? "en") ?? .english
        Localization.usePreviewLanguage(initial)
        let app = NSApplication.shared
        app.setActivationPolicy(option("--snapshot") == nil ? .regular : .prohibited)
        let model = AssistantModel(localizationPreview: true)
        model.language = initial
        if arguments.contains("--advanced") { model.panels[0].advanced = true }

        if let directory = option("--snapshot") {
            let output = URL(fileURLWithPath: directory, isDirectory: true)
            try FileManager.default.createDirectory(at: output, withIntermediateDirectories: true)
            let locales = arguments.contains("--all") ? AppLanguage.allCases.filter { $0 != .system } : [initial]
            for language in locales {
                guard Bundle.main.path(forResource: language.rawValue, ofType: "lproj") != nil else {
                    throw NSError(domain: "LocalizationPreviewMissingResource", code: 1,
                        userInfo: [NSLocalizedDescriptionKey: "Missing \(language.rawValue) resource; generate it before claiming a localized preview."])
                }
                model.setLanguage(language)
                try snapshot(AssistantView(model: model).disabled(true)
                    .frame(width: 540)
                    .environment(\.locale, Locale(identifier: Localization.resource))
                    .environment(\.layoutDirection, Localization.isRightToLeft ? .rightToLeft : .leftToRight),
                    to: output.appendingPathComponent("\(language.rawValue)-app.png"))
                if !arguments.contains("--advanced") {
                    try snapshot(WelcomeView(model: model, close: {}).disabled(true),
                        to: output.appendingPathComponent("\(language.rawValue)-welcome.png"))
                    try snapshot(AccessibilityInstallGuide(install: {}).disabled(true)
                        .environment(\.locale, Locale(identifier: Localization.resource))
                        .environment(\.layoutDirection, Localization.isRightToLeft ? .rightToLeft : .leftToRight),
                        to: output.appendingPathComponent("\(language.rawValue)-helper.png"))
                }
                print("Rendered \(language.rawValue)")
            }
            return
        }
        let window = NSWindow(contentRect: NSRect(x: 100, y: 100, width: 1080, height: 800),
                              styleMask: [.titled, .closable, .resizable], backing: .buffered, defer: false)
        window.title = "E-Ink Assistant Localization Preview"
        window.contentView = NSHostingView(rootView: LocalizationPreviewView(model: model))
        window.makeKeyAndOrderFront(nil)
        app.activate(ignoringOtherApps: true)
        app.run()
    }

    static func snapshot<Content: View>(_ content: Content, to url: URL) throws {
        let hosting = NSHostingView(rootView: content.environment(\.colorScheme, .light).background(Color.white))
        let size = hosting.fittingSize
        guard size.width > 0, size.height > 0, size.height < 12000 else {
            throw NSError(domain: "LocalizationPreviewInvalidLayout", code: 1)
        }
        let window = NSWindow(contentRect: NSRect(origin: .zero, size: size),
                              styleMask: [.borderless], backing: .buffered, defer: false)
        window.contentView = hosting
        hosting.frame = NSRect(origin: .zero, size: size)
        hosting.layoutSubtreeIfNeeded()
        // Allow the font cascade and SwiftUI drawing layers to settle before
        // caching. A single short pass can omit shapes in the first RTL frame.
        RunLoop.current.run(until: Date().addingTimeInterval(0.2))
        hosting.layoutSubtreeIfNeeded()
        hosting.displayIfNeeded()
        if ProcessInfo.processInfo.environment["EINK_PREVIEW_LAYOUT_DEBUG"] == "1" {
            print("LAYOUT \(url.lastPathComponent): fitting=\(size), actual=\(hosting.frame), updated=\(hosting.fittingSize), window=\(window.frame)")
        }
        guard let image = hosting.bitmapImageRepForCachingDisplay(in: hosting.bounds) else {
            throw NSError(domain: "LocalizationPreviewBitmap", code: 1)
        }
        hosting.cacheDisplay(in: hosting.bounds, to: image)
        guard let png = image.representation(using: .png, properties: [:]) else {
            throw NSError(domain: "LocalizationPreviewPNG", code: 1)
        }
        try png.write(to: url)
        window.orderOut(nil)
    }
}
