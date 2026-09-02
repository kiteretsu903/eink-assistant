#include "SettingsStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>

namespace eink {

SettingsStore::SettingsStore(QString path) {
    if (path.isEmpty()) {
        const QString root = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        m_path = QDir(root).filePath(QStringLiteral("settings.ini"));
    } else {
        m_path = std::move(path);
    }
}

AppSettings SettingsStore::load() const {
    AppSettings result;
    QSettings s(m_path, QSettings::IniFormat);
    result.language = s.value(QStringLiteral("general/language"), QStringLiteral("system")).toString();
    result.launchAtLogin = s.value(QStringLiteral("general/launchAtLogin"), false).toBool();
    result.reduceVisualEffects = s.value(QStringLiteral("general/reduceVisualEffects"), false).toBool();
    result.autoVisualEffects = s.value(QStringLiteral("general/autoVisualEffects"), true).toBool();
    result.showWelcome = s.value(QStringLiteral("general/showWelcome"), true).toBool();
    result.trayDiscoveryShown = s.value(QStringLiteral("general/trayDiscoveryShown"), false).toBool();
    result.trayDiscoveryVersion = s.value(QStringLiteral("general/trayDiscoveryVersion"), 0).toInt();
    result.trayDiscoveryExecutablePath = s.value(QStringLiteral("general/trayDiscoveryExecutablePath")).toString();

    const int count = s.beginReadArray(QStringLiteral("displays"));
    for (int i = 0; i < count; ++i) {
        s.setArrayIndex(i);
        DisplaySettings d;
        d.stableId = s.value(QStringLiteral("stableId")).toString();
        d.isEink = s.value(QStringLiteral("isEink"), false).toBool();
        d.reduceShaking = s.value(QStringLiteral("reduceShaking"), false).toBool();
        d.saturation = s.value(QStringLiteral("saturation"), 1.0).toDouble();
        d.saturationPreset = s.value(QStringLiteral("saturationPreset"), 2).toInt();
        d.rgb.red = s.value(QStringLiteral("red"), 1.0).toDouble();
        d.rgb.green = s.value(QStringLiteral("green"), 1.0).toDouble();
        d.rgb.blue = s.value(QStringLiteral("blue"), 1.0).toDouble();
        d.textLevel = static_cast<TextLevel>(s.value(QStringLiteral("textLevel"), 0).toInt());
        d.enhanceLevel = static_cast<EnhanceLevel>(s.value(QStringLiteral("enhanceLevel"), 0).toInt());
        d.advanced = s.value(QStringLiteral("advanced"), false).toBool();
        d.customCurve.knee = s.value(QStringLiteral("curveKnee"), 0.90).toDouble();
        d.customCurve.gamma = s.value(QStringLiteral("curveGamma"), 3.0).toDouble();
        d.customCurve.blackPoint = s.value(QStringLiteral("curveBlack"), 0.0).toDouble();
        d.customCurve.whitePoint = s.value(QStringLiteral("curveWhite"), 1.0).toDouble();
        if (!d.stableId.isEmpty())
            result.displays.push_back(d);
    }
    s.endArray();

    result.savedCurves.clear();
    const int savedCount = s.beginReadArray(QStringLiteral("savedCurves"));
    for (int i = 0; i < 5; ++i) {
        SavedCurve saved;
        if (i < savedCount) {
            s.setArrayIndex(i);
            saved.occupied = s.value(QStringLiteral("occupied"), false).toBool();
            saved.name = s.value(QStringLiteral("name")).toString();
            saved.curve.knee = s.value(QStringLiteral("knee"), 0.35).toDouble();
            saved.curve.gamma = s.value(QStringLiteral("gamma"), 1.0).toDouble();
            saved.curve.blackPoint = s.value(QStringLiteral("black"), 0.0).toDouble();
            saved.curve.whitePoint = s.value(QStringLiteral("white"), 1.0).toDouble();
        }
        result.savedCurves.push_back(saved);
    }
    s.endArray();
    return result;
}

void SettingsStore::save(const AppSettings &settings) const {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSettings s(m_path, QSettings::IniFormat);
    s.clear();
    s.setValue(QStringLiteral("general/language"), settings.language);
    s.setValue(QStringLiteral("general/launchAtLogin"), settings.launchAtLogin);
    s.setValue(QStringLiteral("general/reduceVisualEffects"), settings.reduceVisualEffects);
    s.setValue(QStringLiteral("general/autoVisualEffects"), settings.autoVisualEffects);
    s.setValue(QStringLiteral("general/showWelcome"), settings.showWelcome);
    s.setValue(QStringLiteral("general/trayDiscoveryShown"), settings.trayDiscoveryShown);
    s.setValue(QStringLiteral("general/trayDiscoveryVersion"), settings.trayDiscoveryVersion);
    s.setValue(QStringLiteral("general/trayDiscoveryExecutablePath"), settings.trayDiscoveryExecutablePath);

    s.beginWriteArray(QStringLiteral("displays"));
    for (int i = 0; i < settings.displays.size(); ++i) {
        s.setArrayIndex(i);
        const DisplaySettings &d = settings.displays[i];
        s.setValue(QStringLiteral("stableId"), d.stableId);
        s.setValue(QStringLiteral("isEink"), d.isEink);
        s.setValue(QStringLiteral("reduceShaking"), d.reduceShaking);
        s.setValue(QStringLiteral("saturation"), d.saturation);
        s.setValue(QStringLiteral("saturationPreset"), d.saturationPreset);
        s.setValue(QStringLiteral("red"), d.rgb.red);
        s.setValue(QStringLiteral("green"), d.rgb.green);
        s.setValue(QStringLiteral("blue"), d.rgb.blue);
        s.setValue(QStringLiteral("textLevel"), static_cast<int>(d.textLevel));
        s.setValue(QStringLiteral("enhanceLevel"), static_cast<int>(d.enhanceLevel));
        s.setValue(QStringLiteral("advanced"), d.advanced);
        s.setValue(QStringLiteral("curveKnee"), d.customCurve.knee);
        s.setValue(QStringLiteral("curveGamma"), d.customCurve.gamma);
        s.setValue(QStringLiteral("curveBlack"), d.customCurve.blackPoint);
        s.setValue(QStringLiteral("curveWhite"), d.customCurve.whitePoint);
    }
    s.endArray();

    s.beginWriteArray(QStringLiteral("savedCurves"));
    for (int i = 0; i < settings.savedCurves.size(); ++i) {
        s.setArrayIndex(i);
        const SavedCurve &saved = settings.savedCurves[i];
        s.setValue(QStringLiteral("occupied"), saved.occupied);
        s.setValue(QStringLiteral("name"), saved.name);
        s.setValue(QStringLiteral("knee"), saved.curve.knee);
        s.setValue(QStringLiteral("gamma"), saved.curve.gamma);
        s.setValue(QStringLiteral("black"), saved.curve.blackPoint);
        s.setValue(QStringLiteral("white"), saved.curve.whitePoint);
    }
    s.endArray();
    s.sync();
}

} // namespace eink
