import Foundation

@main
enum LocalizationTests {
    static func main() throws {
        let codes = AppLanguage.definitions.map(\.code)
        precondition(codes.count == 80, "Expected all 80 bundled locales")
        precondition(Set(codes).count == codes.count)
        precondition(AppLanguage.allCases.count == 81)
        for code in codes {
            precondition(AppLanguage(rawValue: code)?.rawValue == code)
            precondition(Localization.resolve(code) == code)
        }
        for (input, expected) in ["en-US": "en", "zh-TW": "zh-Hant", "zh-HK": "zh-Hant",
            "zh-Hans-HK": "zh-Hans", "zh-Hant-CN": "zh-Hant", "zh_CN": "zh-Hans",
            "ja-JP": "ja", "fr-CA": "fr", "pt": "pt-BR", "pt-AO": "pt-BR",
            "pt-PT": "pt-PT", "sr-Cyrl-RS": "sr", "sr-Latn-RS": "en", "xx-ZZ": "en",
            "az-Arab": "en", "az-Latn-AZ": "az", "no-NO": "nb", "tl-PH": "fil", "iw-IL": "he",
            "en-US-u-nu-arab": "en", "ar-u-nu-latn": "ar", "pt-PT-u-ca-gregory": "pt-PT",
            "pt-PT-fonipa": "pt-PT", "sr-Latn-RS-u-nu-latn": "en", "zh-TW-u-nu-latn": "zh-Hant", "zh-Latn": "en", "pt-Latn-PT": "pt-PT"] {
            precondition(Localization.resolve(input) == expected, "\(input) should resolve to \(expected), got \(Localization.resolve(input))")
        }
        precondition(Localization.resolve("fr-CA", supported: ["en", "ja"]) == "en")
        let saved = UserDefaults.standard.string(forKey: "app-language")
        for code in ["en", "ar", "he", "fa", "ur", "ja"] {
            Localization.usePreviewLanguage(AppLanguage(rawValue: code)!)
            precondition(Localization.isRightToLeft == ["ar", "he", "fa", "ur"].contains(code))
        }
        Localization.set(.english)
        precondition(UserDefaults.standard.string(forKey: "app-language") == saved, "Preview mutated saved preference")

        let temporary = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        defer { try? FileManager.default.removeItem(at: temporary) }
        let en = temporary.appendingPathComponent("en.lproj")
        let other = temporary.appendingPathComponent("other.lproj")
        for url in [en, other] { try FileManager.default.createDirectory(at: url, withIntermediateDirectories: true) }
        try "\"present\" = \"English\";\n\"missing\" = \"English fallback\";".write(to: en.appendingPathComponent("Localizable.strings"), atomically: true, encoding: .utf8)
        try "\"present\" = \"Translated\";".write(to: other.appendingPathComponent("Localizable.strings"), atomically: true, encoding: .utf8)
        precondition(Localization.string("present", in: Bundle(url: other)!, english: Bundle(url: en)!) == "Translated")
        precondition(Localization.string("missing", in: Bundle(url: other)!, english: Bundle(url: en)!) == "English fallback")
        precondition(Localization.string("unknown", in: Bundle(url: other)!, english: Bundle(url: en)!) == "unknown")
        if CommandLine.arguments.contains("--resources") {
            for code in codes {
                guard let path = Bundle.main.path(forResource: code, ofType: "lproj"), let bundle = Bundle(path: path) else { fatalError("Missing \(code) resource") }
                precondition(bundle.localizedString(forKey: "language.title", value: "MISSING", table: nil) != "MISSING")
            }
        }
        print("PASS: 80 locales, legacy IDs, primary/region/script resolution, RTL, English fallback, isolated preview preference")
    }
}
