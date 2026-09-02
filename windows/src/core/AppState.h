#pragma once

#include "ToneCurve.h"

#include <QString>
#include <QVector>

namespace eink {

inline constexpr int kCurrentTrayDiscoveryVersion = 5;

struct RgbBalance {
    double red = 1.0;
    double green = 1.0;
    double blue = 1.0;
    bool isIdentity() const;
};

enum class GraphicsVendor {
    Unknown,
    Intel,
    Nvidia,
    Amd
};

struct DisplayInfo {
    QString stableId;
    QString deviceName;
    QString friendlyName;
    bool builtIn = false;
    bool ditheringControlSupported = false;
    bool colorAdjustmentSupported = false;
    bool usesWindows10Mhc2 = false;
    bool acmSupported = false;
    bool acmEnabled = false;
    bool colorAdjustmentUpgradeMayHelp = false;
    GraphicsVendor graphicsVendor = GraphicsVendor::Unknown;
    QString graphicsAdapterName;
    bool gpuControlPanelAvailable = false;
    qint32 adapterHigh = 0;
    quint32 adapterLow = 0;
    quint32 sourceId = 0;
    quint32 targetId = 0;
};

struct DisplaySettings {
    QString stableId;
    bool isEink = false;
    bool reduceShaking = false;
    double saturation = 1.0;
    int saturationPreset = 2;
    RgbBalance rgb;
    TextLevel textLevel = TextLevel::Off;
    EnhanceLevel enhanceLevel = EnhanceLevel::Off;
    bool advanced = false;
    ToneCurve customCurve {0.90, 3.00, 0.0, 1.0};

    ToneCurve effectiveCurve() const;
};

struct SavedCurve {
    QString name;
    bool occupied = false;
    ToneCurve curve;
};

struct AppSettings {
    QString language = QStringLiteral("system");
    bool launchAtLogin = false;
    bool reduceVisualEffects = false;
    bool autoVisualEffects = true;
    bool showWelcome = true;
    bool trayDiscoveryShown = false;
    int trayDiscoveryVersion = 0;
    QString trayDiscoveryExecutablePath;
    QVector<DisplaySettings> displays;
    QVector<SavedCurve> savedCurves = QVector<SavedCurve>(5);

    DisplaySettings &forDisplay(const QString &stableId);
};

} // namespace eink
