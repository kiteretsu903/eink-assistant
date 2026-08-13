// Language selection.
//
// Strings are resolved through a bundle chosen at runtime rather than through
// Bundle.main, so switching language takes effect immediately. Setting
// AppleLanguages would also work but only applies on the next launch, which is
// a poor experience for a menu bar app you rarely restart.

import Foundation

enum AppLanguage: String, CaseIterable {
    case system
    case english = "en"
    case simplifiedChinese = "zh-Hans"
    case traditionalChinese = "zh-Hant"
    case japanese = "ja"

    /// Shown in the picker. Deliberately in each language's own name, so the
    /// options stay recognisable whichever language is currently active.
    var label: String {
        switch self {
        case .system: return L("language.system")
        case .english: return "English"
        case .simplifiedChinese: return "简体中文"
        case .traditionalChinese: return "繁體中文"
        case .japanese: return "日本語"
        }
    }
}

enum Localization {
    private static let key = "app-language"

    /// The bundle strings are read from. Bundle.main follows the system
    /// language; an .lproj bundle pins one.
    private(set) static var bundle: Bundle = .main

    static var current: AppLanguage {
        AppLanguage(rawValue: UserDefaults.standard.string(forKey: key) ?? "")
            ?? .system
    }

    static func set(_ language: AppLanguage) {
        UserDefaults.standard.set(language.rawValue, forKey: key)
        refresh()
    }

    /// Resolves the stored preference to a concrete supported localization.
    /// If the Mac's primary language is unsupported, System deliberately uses
    /// English instead of relying on Bundle.main's implicit fallback rules.
    static func refresh() {
        let resource = current == .system ? systemResource : current.rawValue
        guard let path = Bundle.main.path(forResource: resource, ofType: "lproj"),
              let localized = Bundle(path: path)
        else {
            bundle = englishBundle ?? .main
            return
        }
        bundle = localized
    }

    private static var englishBundle: Bundle? {
        Bundle.main.path(forResource: "en", ofType: "lproj").flatMap(Bundle.init(path:))
    }

    /// Only the primary system language decides System mode. This makes the
    /// fallback predictable: French, Korean, or any other unsupported primary
    /// language shows English even if a secondary preferred language happens
    /// to be supported.
    private static var systemResource: String {
        let primary = Locale.preferredLanguages.first?.lowercased() ?? "en"
        if primary.hasPrefix("ja") { return "ja" }
        if primary.hasPrefix("zh") {
            if primary.contains("hant") || primary.hasPrefix("zh-tw")
                || primary.hasPrefix("zh-hk") || primary.hasPrefix("zh-mo") {
                return "zh-Hant"
            }
            return "zh-Hans"
        }
        if primary.hasPrefix("en") { return "en" }
        return "en"
    }
}

/// Localized string lookup. Keys are stable identifiers rather than English
/// text, because several UI strings are assembled by concatenation and SwiftUI
/// only auto-localizes a bare literal.
func L(_ key: String) -> String {
    Localization.bundle.localizedString(forKey: key, value: nil, table: nil)
}
