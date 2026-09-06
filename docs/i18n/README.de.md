# E-Ink Assistant

<!-- BEGIN README LANGUAGES -->
<p align="center">
  <a href="../../README.md" lang="en" dir="ltr">English</a> &nbsp;·&nbsp;
  <a href="README.zh-Hans.md" lang="zh-Hans" dir="ltr">简体中文</a> &nbsp;·&nbsp;
  <a href="README.zh-Hant.md" lang="zh-Hant" dir="ltr">繁體中文</a> &nbsp;·&nbsp;
  <a href="README.ja.md" lang="ja" dir="ltr">日本語</a> &nbsp;·&nbsp;
  <a href="README.ko.md" lang="ko" dir="ltr">한국어</a> &nbsp;·&nbsp;
  <a href="README.es.md" lang="es" dir="ltr">Español</a> &nbsp;·&nbsp;
  <a href="README.fr.md" lang="fr" dir="ltr">Français</a> &nbsp;·&nbsp;
  <b lang="de" dir="ltr">Deutsch</b> &nbsp;·&nbsp;
  <a href="README.pt-BR.md" lang="pt-BR" dir="ltr">Português (Brasil)</a> &nbsp;·&nbsp;
  <a href="README.ru.md" lang="ru" dir="ltr">Русский</a> &nbsp;·&nbsp;
  <a href="README.ar.md" lang="ar" dir="rtl">العربية</a> &nbsp;·&nbsp;
  <a href="README.hi.md" lang="hi" dir="ltr">हिन्दी</a>
</p>
<!-- END README LANGUAGES -->

<p align="center">
  <img src="../../Resources/AppIcon.png" alt="App-Symbol von E-Ink Assistant" width="128">
</p>

**Schwarzweiß- und Farb-E-Ink-Displays unter macOS und Windows anpassen.**

[Produktwebsite besuchen](https://kiteretsu903.github.io/eink-assistant/de/)

E-Ink Assistant passt Textkontrast, Schattendetails und Farben der von Ihnen
gewählten E-Ink-Displays an. Andere Displays bleiben unverändert. Die macOS-Ausgabe
läuft in der Menüleiste, die Windows-Ausgabe im Infobereich der Taskleiste.

[macOS 2.6 herunterladen](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-v2.6.dmg) ·
[Windows 1.2 herunterladen](https://github.com/kiteretsu903/eink-assistant/releases/download/macos-v2.6-windows-v1.2/E-Ink-Assistant-Windows-1.2-Setup.exe) ·
[Alle Versionen ansehen](https://github.com/kiteretsu903/eink-assistant/releases)

Kostenlos, quelloffen und unter der MIT-Lizenz verfügbar.

## Funktionen und Systemanforderungen

| Funktion | macOS | Windows |
|---|---|---|
| Unterstützte Systeme | **macOS 14 oder neuer**<br>Nur Apple Silicon | **Windows 7 SP1 bis Windows 11**<br>x64-Computer |
| Wo die App läuft | Menüleiste | Infobereich der Taskleiste |
| Bestimmte E-Ink-Displays auswählen | Ja. Andere Displays bleiben unverändert. | Wie unter macOS |
| Textkontrast | Vier Stufen: Mittel, Stark, Scharf, Satt | Wie unter macOS |
| Videoverbesserung | Drei Stufen: Dezent, Mittel, Stark | Wie unter macOS |
| Erweiterte Kurve und Voreinstellungen | Kurveneditor mit Live-Ansicht und fünf benannte Voreinstellungen | Wie unter macOS |
| Sättigung und RGB | Farbprofil pro Display; 0%–300% Sättigung und 0%–200% RGB | Auf geeigneten Systemen mit Windows 10 2004 und Windows 11 21H2+ verfügbar; das verfügbare Verfahren hängt von System und Hardware ab |
| Flimmern reduzieren | Auf unterstützten externen Displays verfügbar; wird aktiviert, sobald ein Display als E-Ink markiert wird | Nicht verfügbar. Windows bietet keine einheitliche öffentliche Dithering-Steuerung pro Display, und die meisten Windows-Systeme benötigen sie vermutlich nicht. |
| Transparenz und Bewegung reduzieren | Über einen einmalig vom Benutzer bestätigten Hilfskurzbefehl verfügbar | Ab Windows 7 SP1 über kompatible System-APIs verfügbar |
| Heller Systemmodus | Wird nicht verändert | Heller Windows-Modus nur für die Sitzung unter Windows 10 1903+ |
| Night Shift / Nachtmodus | Night Shift und True Tone für einzelne Displays ausschließen; erfordert Administratorbestätigung und erneutes Anschließen | Nachtmodus-Einstellungen ab Windows 10 1703+; direkte Steuerung „Nachtmodus deaktivieren“ unter Windows 11 24H2+ |
| Gespiegelte / duplizierte Displays | Gespiegelte physische Displays bleiben einzeln auswählbar | Tonwertkurven wirken auf die gemeinsame Quelle; Sättigung und RGB erfordern den Modus „Erweitern“ |
| Änderungen wiederherstellen | Temporäre Kurven, Farbprofile und Dithering werden beim Beenden wiederhergestellt; der Ausschluss von Night Shift / True Tone bleibt bestehen | Temporäre Gamma-, Farb-, Darstellungs- und Nachtmodus-Änderungen werden beim Beenden wiederhergestellt; Farbe und Nachtmodus werden auch nach einem unerwarteten Beenden wiederhergestellt |
| Bei Anmeldung starten | Unterstützt | Unterstützt |
| Oberflächensprachen | Englisch, vereinfachtes Chinesisch, traditionelles Chinesisch, Japanisch | Wie unter macOS |
| Administratorzugriff | Nur für den optionalen Ausschluss von Night Shift / True Tone | Für Installationsprogramm und App erforderlich |

[macOS-Details](../../macos/README.md) ·
[Windows-Kompatibilität und Einrichtung](../../WINDOWS.md) ·
[macOS-Versionsverlauf](../../CHANGELOG.md) ·
[Windows-Versionsverlauf](../../windows/CHANGELOG.md)

<p align="center">
  <img src="../../docs/en/app-main-v2-1.png" alt="E-Ink Assistant v2.1 auf Englisch" width="440">
</p>

## Bedienelemente

| Bedienelement | Einsatzzweck | Wirkung |
|---|---|---|
| Textkontrast | Lesen | Dunkelt schwachen Text mit den Stufen Mittel, Stark, Scharf und Satt ab. Stärkere Stufen erzeugen härtere Kanten auf Kosten von Graudetails. |
| Videoverbesserung | Fotos und Videos | Macht Schattendetails mit den Stufen Dezent, Mittel und Stark sichtbar. Zum Lesen ausschalten, da auch dunkler Text aufgehellt wird. |
| Sättigung und RGB | Farb-E-Ink | Bietet sechs Sättigungsvoreinstellungen, einen Sättigungsregler von 0%–300% und RGB-Korrektur von 0%–200%, sofern die Plattform dies unterstützt. |
| Flimmern reduzieren | Unterstützte macOS-Displays | Stoppt sichtbares Dithering-Flimmern und wird für als E-Ink markierte Displays automatisch aktiviert. |
| Night Shift und True Tone | Displays, die von Farbtemperaturänderungen betroffen sind | Schließt das gewählte macOS-Display aus. Dies erfordert Administratorbestätigung und erneutes Anschließen des Displays; die Einstellung bleibt nach dem Beenden bestehen. |
| Transparenz und Bewegung reduzieren | Displays mit langsamer Bildaktualisierung | Vereinfacht die Systemdarstellung. macOS verwendet einen einmalig vom Benutzer bestätigten Hilfskurzbefehl. |
| Erweiterte Kurven | Abstimmung auf einzelne Displays | Passt Knickpunkt, Gamma, Schwarzpunkt und Weißpunkt mit einer Live-Grafik und fünf benannten Voreinstellungen an. |

<p align="center">
  <img src="../../docs/en/text-contrast-editorial.png" alt="Veranschaulichender Vorher-Nachher-Vergleich des Textkontrasts" width="31%">
  <img src="../../docs/en/video-enhance-editorial.png" alt="Veranschaulichender Vorher-Nachher-Vergleich der Videoverbesserung" width="31%">
  <img src="../../docs/en/saturation-editorial.png" alt="Veranschaulichender Vorher-Nachher-Vergleich der Sättigung" width="31%">
</p>

> Diese Bilder veranschaulichen die Bedienelemente. Ergebnisse hängen vom Display
> und vom Ausgangsmaterial ab.

## Installation

### macOS 14+, Apple Silicon

1. Laden Sie die DMG-Datei für macOS 2.6 über den obigen Link herunter.
2. Öffnen Sie sie und ziehen Sie **E-Ink Assistant** in den Ordner **Programme**.
3. Versuchen Sie einmal, die App zu öffnen. Wenn macOS sie blockiert, öffnen Sie
   **Systemeinstellungen → Datenschutz & Sicherheit** und wählen Sie **Dennoch öffnen**.

Diese Software wird unabhängig entwickelt und ist derzeit nicht im App Store erhältlich.
macOS zeigt beim ersten Öffnen eine Warnung an, dass die App nicht überprüft werden kann.
Der Code ist vollständig quelloffen, sodass Sie ihn prüfen können, bevor Sie sich für
die Nutzung entscheiden.

Wenn **Dennoch öffnen** nicht erscheint, nachdem Sie die App in den Ordner Programme
verschoben haben, öffnen Sie Terminal und führen Sie Folgendes aus:

```
xattr -dr com.apple.quarantine "/Applications/E-Ink Assistant.app"
```

### Windows 7 SP1 bis Windows 11, x64

1. Laden Sie das Installationsprogramm für Windows 1.2 über den obigen Link herunter.
2. Führen Sie Setup aus und bestätigen Sie die Administratorabfrage.
3. Öffnen Sie E-Ink Assistant über das Startmenü oder den Infobereich der Taskleiste.

Die genaue Verfügbarkeit der Funktionen nach Windows-Version, GPU, Treiber und
Displayanschluss finden Sie in [WINDOWS.md](../../WINDOWS.md).

## Verwendung

<p align="center">
  <img src="../../docs/en/app-displays-v2-1.png" alt="Displays in E-Ink Assistant v2.1 markieren" width="440">
</p>

1. Öffnen Sie die App über die macOS-Menüleiste oder den Infobereich der Windows-Taskleiste.
2. Markieren Sie jedes Schwarzweiß- oder Farb-E-Ink-Display, das Sie anpassen möchten.
3. Stellen Sie zuerst im Menü des Monitors einen ausgewogenen Hardware-Kontrast ein.
4. Wählen Sie Textkontrast zum Lesen oder Videoverbesserung für Medien, nicht beides gleichzeitig.
5. Passen Sie bei Farb-E-Ink Sättigung und RGB an, sofern die Plattform dies unterstützt.

**Displayanpassungen werden beim Beenden wiederhergestellt** und beim Start erneut
angewendet. Aktivieren Sie **Bei Anmeldung starten** für den automatischen Start.

## Displayeinrichtung

Stellen Sie vor den Anpassungen in der App im Menü des Monitors einen ausgewogenen
Kontrast ein. Die mitgelieferten Voreinstellungen wurden nach Augenmaß auf einem
**Bigme B251 Pro** (R2 FW V2.0) mit **Web Mode, Hardware-Gamma Stufe 3,
Kontrast 50, Color Restore Mode aus** abgestimmt. Ein Schwarzweiß-Display oder ein
anderes Farbmodell benötigt eigene Werte. Der erweiterte Modus stellt die vollständige
Kurve bereit, und die Einstellungen werden für jedes Display separat gespeichert.

Flimmern reduzieren ist nur auf Apple Silicon verfügbar und wird ausgeblendet,
wenn es nicht unterstützt wird.

<details>
<summary>macOS-Hilfskurzbefehl für Transparenz und Bewegung reduzieren</summary>

Bei der ersten Verwendung müssen Sie **Kurzbefehl hinzufügen** in Apples App
Kurzbefehle bestätigen. Der mitgelieferte Hilfskurzbefehl akzeptiert ausschließlich
die exakten Textbefehle `on` und `off`, liefert keine Ausgabe und erscheint weder
im Teilen-Menü noch in Spotlight, Schnellaktionen oder auf dem Sperrbildschirm.
Er kann bei gesperrtem Mac ausgeführt werden. Die App listet Ihre anderen
Kurzbefehle weder auf noch untersucht sie diese.

Der automatische Modus schaltet beide Einstellungen ein, wenn ein markiertes
E-Ink-Display angeschlossen wird, und aus, wenn das letzte markierte Display getrennt
wird. Beim Beenden der App werden beide ebenfalls ausgeschaltet.

</details>

## Projektdokumentation

- [CHANGELOG.md](../../CHANGELOG.md): Änderungen in jeder Version
- [TECHNICAL.md](../../TECHNICAL.md): Implementierung, Messungen und Ansätze,
  die unter modernem macOS *nicht* funktionieren
- [mac-saturation](https://github.com/kiteretsu903/mac-saturation): Untersuchung
  des Farbmechanismus und eine CLI zum Profilexport

## Lizenz und Danksagungen

MIT, siehe [LICENSE](../../LICENSE).

**Flimmern reduzieren basiert auf [Stillcolor](https://github.com/aiaf/Stillcolor)
von Abdullah Arif** (MIT). Stillcolor entdeckte, dass sich Display-Dithering über
die I/O-Registry-Eigenschaft `enableDither` deaktivieren lässt. Dieses Projekt setzt
die Idee für einzelne Displays neu um; die Anerkennung für die Entdeckung gebührt
Stillcolor. Vielen Dank.

Vollständige Hinweise in [THIRD-PARTY-NOTICES.md](../../THIRD-PARTY-NOTICES.md).
