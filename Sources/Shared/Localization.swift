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

    /// Shown in the picker. Deliberately in each language's own name, so the
    /// options stay recognisable whichever language is currently active.
    var label: String {
        switch self {
        case .system: return L("language.system")
        case .english: return "English"
        case .simplifiedChinese: return "简体中文"
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

    /// Resolves the stored preference to a bundle. Falls back to the system
    /// bundle if the requested .lproj is missing rather than showing raw keys.
    static func refresh() {
        guard current != .system,
              let path = Bundle.main.path(forResource: current.rawValue, ofType: "lproj"),
              let localized = Bundle(path: path)
        else {
            bundle = .main
            return
        }
        bundle = localized
    }
}

/// Localized string lookup. Keys are stable identifiers rather than English
/// text, because several UI strings are assembled by concatenation and SwiftUI
/// only auto-localizes a bare literal.
func L(_ key: String) -> String {
    Localization.bundle.localizedString(forKey: key, value: nil, table: nil)
}
