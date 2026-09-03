#pragma once

#include "ToneCurve.h"

#include <QString>
#include <QStringList>
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
    QString legacyStableId;
    QString deviceName;
    QString friendlyName;
    bool friendlyNameIsFallback = false;
    bool builtIn = false;
    bool ditheringControlSupported = false;
    bool colorAdjustmentSupported = false;
    bool usesWindows10Mhc2 = false;
    bool acmSupported = false;
    bool acmToggleSupported = false;
    bool acmEnabled = false;
    bool colorAdjustmentUpgradeMayHelp = false;
    bool matrixDdiSupported = false;
    int wddmVersion = 0;
    QString colorCapabilityFingerprint;
    GraphicsVendor graphicsVendor = GraphicsVendor::Unknown;
    QString graphicsAdapterName;
    bool gpuControlPanelAvailable = false;
    qint32 adapterHigh = 0;
    quint32 adapterLow = 0;
    quint32 sourceId = 0;
    quint32 targetId = 0;
    bool cloneMode = false;
    QString cloneGroupKey;
    QStringList clonePeerNames;
    QVector<quint32> cloneTargetIds;
};

struct DisplaySettings {
    QString stableId;
    bool isEink = false;
    bool reduceShaking = false;
    double saturation = 1.0;
    int saturationPreset = 2;
    RgbBalance rgb;
    bool experimentalColorEnabled = false;
    QString confirmedColorFingerprint;
    QString failedColorFingerprint;
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
    bool hideHardwareSetupNotice = false;
    bool trayDiscoveryShown = false;
    int trayDiscoveryVersion = 0;
    QString trayDiscoveryExecutablePath;
    QVector<DisplaySettings> displays;
    QVector<SavedCurve> savedCurves = QVector<SavedCurve>(5);

    DisplaySettings &forDisplay(const QString &stableId);
};

} // namespace eink
