#pragma once

#include "core/AppState.h"
#include "core/SettingsStore.h"
#include "platform/PlatformServices.h"

#include <QObject>
#include <QThreadPool>
#include <atomic>
#include <memory>

namespace eink {

class ApplicationController : public QObject {
    Q_OBJECT
public:
    ApplicationController(std::unique_ptr<PlatformServices> platform,
                          SettingsStore store = SettingsStore{}, QObject *parent = nullptr);
    ~ApplicationController() override;

    const QVector<DisplayInfo> &displays() const { return m_displays; }
    const AppSettings &settings() const { return m_settings; }
    AppSettings &settings() { return m_settings; }
    PlatformServices &platform() { return *m_platform; }
    const QString &lastError() const { return m_lastError; }
    DisplaySettings &settingsFor(const QString &stableId);

    void initialize();
    void refreshDisplays();
    void setEink(const QString &id, bool enabled);
    void setSaturation(const QString &id, double value, int preset = -1);
    void setRgb(const QString &id, const RgbBalance &rgb);
    void setTextLevel(const QString &id, TextLevel level);
    void setEnhanceLevel(const QString &id, EnhanceLevel level);
    void setAdvanced(const QString &id, bool enabled);
    void setCustomCurve(const QString &id, const ToneCurve &curve);
    void setReduceShaking(const QString &id, bool enabled);
    void setVisualEffects(bool enabled);
    void setWindowsLightMode(bool enabled);
    void setNightLightDisabled(bool disabled);
    void refreshNightLightState();
    void setAutoVisualEffects(bool enabled);
    void setLaunchAtLogin(bool enabled);
    void setLanguage(const QString &language);
    void setShowWelcome(bool enabled);
    void setTrayDiscoveryShown(bool shown, const QString &executablePath=QString(),
                               int version=kCurrentTrayDiscoveryVersion);
    void saveCurve(int slot, const ToneCurve &curve);
    void applySavedCurve(int slot, const QString &displayId);
    void renameCurve(int slot, const QString &name);
    void clearCurve(int slot);
    void reapplyAll();
    void waitForPendingOperations();
    void shutdown();
    void beginOperation();
    void endOperation();
    bool operationInProgress() const { return m_operationCount.load()>0; }

signals:
    void displaysChanged();
    void stateChanged();
    void errorChanged(const QString &message);
    void colorApplyFinished(const QString &displayId);
    void operationStarted();
    void operationFinished();
    void nightLightStateChanged(bool disabled);
    void visualEffectsStateChanged(bool reduced);
    void windowsLightModeStateChanged(bool enabled);

private:
    const DisplayInfo *displayById(const QString &id) const;
    void applyCurve(const QString &id);
    void applyColor(const QString &id);
    void queueColorApply(const QString &id);
    void waitForColorTasks();
    void persistAndNotify();
    void report(const ApplyResult &result);
    void beginQuietOperation();
    void endQuietOperation();

    std::unique_ptr<PlatformServices> m_platform;
    SettingsStore m_store;
    AppSettings m_settings;
    QVector<DisplayInfo> m_displays;
    bool m_initialized = false;
    bool m_initializing = false;
    std::atomic_bool m_shutDown {false};
    std::atomic_int m_operationCount {0};
    QString m_lastError;
    bool m_lastNightLightDisabled = false;
    bool m_nightLightStateKnown = false;
    bool m_windowsLightModeOwned = false;
    bool m_originalWindowsLightMode = false;
    QThreadPool m_colorPool;
    QThreadPool m_systemPool;
};

} // namespace eink
