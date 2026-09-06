// Check CoreText's real system-font cascade and shaping result, rather than
// assuming a font supports a script from its filename or Unicode ranges.
import Foundation
import CoreText

struct FontIssue: Codable {
    let locale: String
    let key: String
    let utf16Index: Int
    let unicode: String
    let font: String
    let reason: String
}
struct FontReport: Codable {
    let checkedLocales: Int
    let checkedStrings: Int
    let checkedAutonyms: Int
    let requestedFont: String
    let resolvedFonts: [String]
    let issues: [FontIssue]
}

@main
enum LocalizationFontTests {
    static func main() throws {
        let arguments = CommandLine.arguments
        let root = URL(fileURLWithPath: arguments.count > 1 ? arguments[1] : FileManager.default.currentDirectoryPath)
        let output = URL(fileURLWithPath: arguments.count > 2 ? arguments[2] : "/tmp/eink-localization-fonts.json")
        let definitions = try JSONSerialization.jsonObject(with: Data(contentsOf: root.appendingPathComponent("localization/locales.json"))) as! [[String: Any]]
        let font = CTFontCreateUIFontForLanguage(.system, 15, nil)!
        var issues: [FontIssue] = []
        var resolvedFonts = Set<String>()
        var locales = 0
        var strings = 0
        for definition in definitions {
            let code = definition["code"] as! String
            let url = root.appendingPathComponent("Resources/\(code).lproj/Localizable.strings")
            var catalog: [String: String] = [:]
            if FileManager.default.fileExists(atPath: url.path) {
                catalog = try PropertyListSerialization.propertyList(from: Data(contentsOf: url), options: [], format: nil) as! [String: String]
                locales += 1
            }
            catalog["locale.autonym"] = definition["name"] as? String
            for (key, value) in catalog.sorted(by: { $0.key < $1.key }) {
                strings += 1
                let attributes: [NSAttributedString.Key: Any] = [
                    NSAttributedString.Key(kCTFontAttributeName as String): font,
                    NSAttributedString.Key(kCTLanguageAttributeName as String): code
                ]
                let line = CTLineCreateWithAttributedString(NSAttributedString(string: value, attributes: attributes))
                let runs = CTLineGetGlyphRuns(line) as! [CTRun]
                for run in runs {
                    let count = CTRunGetGlyphCount(run)
                    var glyphs = [CGGlyph](repeating: 0, count: count)
                    var indices = [CFIndex](repeating: 0, count: count)
                    CTRunGetGlyphs(run, CFRange(location: 0, length: 0), &glyphs)
                    CTRunGetStringIndices(run, CFRange(location: 0, length: 0), &indices)
                    let runAttributes = CTRunGetAttributes(run) as NSDictionary
                    let runFont = runAttributes[kCTFontAttributeName] as! CTFont
                    let name = CTFontCopyPostScriptName(runFont) as String
                    resolvedFonts.insert(name)
                    for position in glyphs.indices where glyphs[position] == 0 || name.lowercased().contains("lastresort") {
                        let index = indices[position]
                        let units = Array(value.utf16)
                        guard index >= 0, index < units.count else { continue }
                        var scalar = UInt32(units[index])
                        if (0xD800...0xDBFF).contains(units[index]), index + 1 < units.count,
                           (0xDC00...0xDFFF).contains(units[index + 1]) {
                            scalar = 0x10000 + (UInt32(units[index]) - 0xD800) * 0x400 + UInt32(units[index + 1]) - 0xDC00
                        }
                        // Directional formatting, joiners, variation selectors,
                        // and whitespace do not require visible glyphs.
                        if let character = UnicodeScalar(scalar), CharacterSet.whitespacesAndNewlines.contains(character) { continue }
                        if scalar < 0x20 || (0x7F...0x9F).contains(scalar)
                            || (0x200B...0x200F).contains(scalar) || (0x202A...0x202E).contains(scalar)
                            || (0x2060...0x206F).contains(scalar) || (0xFE00...0xFE0F).contains(scalar)
                            || (0xE0100...0xE01EF).contains(scalar) || scalar == 0xFEFF { continue }
                        issues.append(FontIssue(locale: code, key: key, utf16Index: index,
                            unicode: String(format: "U+%04X", scalar), font: name,
                            reason: glyphs[position] == 0 ? "missing glyph" : "LastResort font"))
                    }
                }
            }
        }
        let report = FontReport(checkedLocales: locales, checkedStrings: strings,
                                checkedAutonyms: definitions.count, requestedFont: CTFontCopyPostScriptName(font) as String,
                                resolvedFonts: resolvedFonts.sorted(), issues: issues)
        let encoder = JSONEncoder(); encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        try encoder.encode(report).write(to: output)
        print("CoreText: \(locales)/80 locales, \(strings) strings/autonyms, \(resolvedFonts.count) resolved fonts, \(issues.count) glyph issues; \(output.path)")
        if !issues.isEmpty { exit(1) }
    }
}
