// Runtime-selected localization shared by the app and the tuning tools.
import Foundation

struct LocaleDefinition: Decodable {
    let code: String
    let name: String
    let dir: String
}

struct AppLanguage: RawRepresentable, Hashable, Identifiable, CaseIterable {
    let rawValue: String
    var id: String { rawValue }
    private init(code: String) { rawValue = code }

    static let system = AppLanguage(code: "system")
    static let english = AppLanguage(code: "en")
    static let definitions: [LocaleDefinition] = {
        guard let url = Bundle.main.url(forResource: "locales", withExtension: "json"),
              let data = try? Data(contentsOf: url),
              let locales = try? JSONDecoder().decode([LocaleDefinition].self, from: data),
              locales.contains(where: { $0.code == "en" }) else {
            return [LocaleDefinition(code: "en", name: "English", dir: "ltr")]
        }
        return locales
    }()
    static var allCases: [AppLanguage] { [.system] + definitions.map { AppLanguage(code: $0.code) } }

    init?(rawValue: String) {
        guard rawValue == "system" || Self.definitions.contains(where: { $0.code == rawValue }) else { return nil }
        self.rawValue = rawValue
    }

    /// Autonyms remain recognizable regardless of the selected UI language.
    var label: String {
        self == .system ? L("language.system") : Self.definitions.first(where: { $0.code == rawValue })?.name ?? rawValue
    }
}

enum Localization {
    private static let key = "app-language"
    static let didChange = Notification.Name("EinkAssistantLocalizationDidChange")
    private(set) static var bundle: Bundle = .main
    private(set) static var resource = "en"
    private static var previewLanguage: AppLanguage?

    static var current: AppLanguage {
        previewLanguage ?? AppLanguage(rawValue: UserDefaults.standard.string(forKey: key) ?? "") ?? .system
    }
    static var isRightToLeft: Bool {
        AppLanguage.definitions.first(where: { $0.code == resource })?.dir == "rtl"
    }

    static func set(_ language: AppLanguage) {
        if previewLanguage != nil { previewLanguage = language }
        else { UserDefaults.standard.set(language.rawValue, forKey: key) }
        refresh()
        NotificationCenter.default.post(name: didChange, object: nil)
    }

    /// Preview mode never writes the user's saved language preference.
    static func usePreviewLanguage(_ language: AppLanguage) {
        previewLanguage = language
        refresh()
    }

    static func refresh() {
        resource = current == .system ? resolve(Locale.preferredLanguages.first ?? "en") : current.rawValue
        bundle = localizedBundle(resource) ?? localizedBundle("en") ?? .main
    }

    private static func localizedBundle(_ code: String) -> Bundle? {
        Bundle.main.path(forResource: code, ofType: "lproj").flatMap(Bundle.init(path:))
    }

    /// Use only the primary system language. Region variants fall back to their
    /// supported base; an unsupported primary falls back to English, never a
    /// secondary preference. Explicit unsupported scripts do not cross scripts.
    static func resolve(_ identifier: String, supported: [String] = AppLanguage.definitions.map(\.code)) -> String {
        let value = identifier.replacingOccurrences(of: "_", with: "-").lowercased()
        // BCP 47 singleton subtags begin extensions/private use. Their values
        // (for example `u-nu-arab`) are not a requested writing script.
        let parts = Array(value.split(separator: "-").map(String.init).prefix { $0.count != 1 })
        let requestedBase = parts.first ?? "en"
        let base = ["no": "nb", "tl": "fil", "iw": "he", "in": "id"][requestedBase] ?? requestedBase
        func match(_ code: String) -> String? { supported.first { $0.lowercased() == code.lowercased() } }
        let core = ([base] + parts.dropFirst()).joined(separator: "-")
        if let exact = match(core) { return exact }
        let script = parts.count > 1 && parts[1].count == 4 ? parts[1] : nil
        if base == "zh" {
            if let script, !["hans", "hant"].contains(script) { return "en" }
            let traditional = parts.contains("hant") || (!parts.contains("hans") && parts.contains(where: { ["tw", "hk", "mo"].contains($0) }))
            return match(traditional ? "zh-Hant" : "zh-Hans") ?? "en"
        }
        if let script {
            if let scripted = match("\(base)-\(script)") { return scripted }
            // A base locale may use the requested script implicitly (sr-Cyrl,
            // az-Latn, etc.). Foundation's likely script supplies that default.
            let defaultScript = Locale.Language(identifier: base).script?.identifier.lowercased()
            if defaultScript != script { return "en" }
        }
        // Keep a supported region when later variant subtags are present.
        // This runs after script validation so sr-Latn cannot fall into sr.
        var regionCandidate = Array(parts.dropFirst(script == nil ? 1 : 2))
        while !regionCandidate.isEmpty {
            if let regional = match(([base] + regionCandidate).joined(separator: "-")) { return regional }
            regionCandidate.removeLast()
        }
        if let language = match(base) { return language }
        // Shared registry order chooses the intentional default when only
        // regional variants exist, e.g. Portuguese -> pt-BR.
        return supported.first { $0.lowercased().hasPrefix(base + "-") } ?? "en"
    }

    static func string(_ key: String, in selected: Bundle? = nil, english: Bundle? = nil) -> String {
        let fallback = (english ?? localizedBundle("en"))?.localizedString(forKey: key, value: key, table: nil) ?? key
        return (selected ?? bundle).localizedString(forKey: key, value: fallback, table: nil)
    }
}

func L(_ key: String) -> String { Localization.string(key) }
