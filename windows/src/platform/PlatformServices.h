#pragma once

#include "core/AppState.h"

#include <QString>
#include <QVector>

namespace eink {

struct ApplyResult {
    bool success = true;
    QString error;
    static ApplyResult ok() { return {}; }
    static ApplyResult fail(const QString &message) { return {false, message}; }
};

class PlatformServices {
public:
    virtual ~PlatformServices() = default;
    virtual ApplyResult recoverInterruptedColorState() = 0;
    virtual QVector<DisplayInfo> displays() = 0;
    virtual ApplyResult applyToneCurve(const DisplayInfo &, const ToneCurve &) = 0;
    virtual ApplyResult restoreToneCurve(const DisplayInfo &) = 0;
    virtual ApplyResult applyColor(const DisplayInfo &, double saturation, const RgbBalance &) = 0;
    virtual ApplyResult restoreColor(const DisplayInfo &) = 0;
    virtual ApplyResult beginColorSafetyTest(const DisplayInfo &, double saturation,
                                             const RgbBalance &, int timeoutSeconds) = 0;
    virtual ApplyResult confirmColorSafetyTest(const DisplayInfo &) = 0;
    virtual ApplyResult rollbackColorSafetyTest(const DisplayInfo &) = 0;
    virtual ApplyResult setDitheringDisabled(const DisplayInfo &, bool disabled) = 0;
    virtual bool visualEffectsReduced() const = 0;
    virtual ApplyResult setVisualEffectsReduced(bool reduced) = 0;
    virtual bool windowsLightModeAvailable() const = 0;
    virtual bool windowsLightModeEnabled() const = 0;
    virtual ApplyResult setWindowsLightModeEnabled(bool enabled) = 0;
    virtual ApplyResult setLaunchAtLogin(bool enabled) = 0;
    virtual bool launchAtLogin() const = 0;
    virtual void openVisualEffectsSettings() = 0;
    virtual ApplyResult recoverInterruptedNightLightState() = 0;
    virtual bool nightLightControlAvailable() const = 0;
    virtual bool nightLightEnabled() const = 0;
    virtual ApplyResult setNightLightDisabled(bool disabled) = 0;
    virtual ApplyResult restoreNightLightState() = 0;
    virtual bool nightLightAvailable() const = 0;
    virtual void openNightLightSettings() = 0;
    virtual bool saturationPlatformAvailable() const = 0;
    virtual ApplyResult openGpuControlPanel(const DisplayInfo &) = 0;
    virtual QString platformDiagnostic() const = 0;
    virtual void shutdown() = 0;
};

} // namespace eink
