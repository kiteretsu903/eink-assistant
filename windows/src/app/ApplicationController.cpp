#include "ApplicationController.h"

#include <QMetaObject>
#include <QRunnable>
#include <QThread>
#include <algorithm>

namespace eink {

ApplicationController::ApplicationController(std::unique_ptr<PlatformServices> platform,
                                             SettingsStore store, QObject *parent)
    : QObject(parent), m_platform(std::move(platform)), m_store(std::move(store)) {
    m_colorPool.setMaxThreadCount(1);
    m_colorPool.setExpiryTimeout(5000);
    m_systemPool.setMaxThreadCount(1);m_systemPool.setExpiryTimeout(5000);
}

ApplicationController::~ApplicationController() { shutdown(); }

void ApplicationController::initialize() {
    if (m_initialized) return;
    m_settings = m_store.load();
    m_settings.launchAtLogin = m_platform->launchAtLogin();
    m_initialized = true;
    m_initializing = true;
    report(m_platform->recoverInterruptedColorState());
    report(m_platform->recoverInterruptedNightLightState());
    if(m_platform->nightLightControlAvailable()) {
        m_lastNightLightDisabled=!m_platform->nightLightEnabled();m_nightLightStateKnown=true;
    }
    refreshDisplays();
    reapplyAll();
    m_initializing = false;
}

DisplaySettings &ApplicationController::settingsFor(const QString &stableId) {
    return m_settings.forDisplay(stableId);
}

const DisplayInfo *ApplicationController::displayById(const QString &id) const {
    for (const DisplayInfo &d : m_displays) if (d.stableId == id) return &d;
    return nullptr;
}

void ApplicationController::refreshDisplays() {
    const bool hadEink = std::any_of(m_displays.begin(), m_displays.end(), [&](const DisplayInfo &d) {
        for (const DisplaySettings &s : m_settings.displays) if (s.stableId==d.stableId) return s.isEink;
        return false;
    });
    m_displays = m_platform->displays();
    std::stable_partition(m_displays.begin(),m_displays.end(),[](const DisplayInfo &display){return !display.builtIn;});
    const bool hasEink = std::any_of(m_displays.begin(), m_displays.end(), [&](const DisplayInfo &d) {
        return m_settings.forDisplay(d.stableId).isEink;
    });
    if (m_settings.autoVisualEffects && hadEink != hasEink)
        report(m_platform->setVisualEffectsReduced(hasEink));
    emit displaysChanged();
}

void ApplicationController::setEink(const QString &id, bool enabled) {
    beginOperation();
    DisplaySettings &s = settingsFor(id);
    s.isEink = enabled;
    const DisplayInfo *d = displayById(id);
    if (d) {
        if (enabled) {
            queueColorApply(id); applyCurve(id);
            if (s.reduceShaking && d->ditheringControlSupported)
                report(m_platform->setDitheringDisabled(*d, true));
        } else {
            report(m_platform->restoreToneCurve(*d));
            queueColorApply(id);
            if (d->ditheringControlSupported)
                report(m_platform->setDitheringDisabled(*d, false));
        }
    }
    if (m_settings.autoVisualEffects) {
        const bool any = std::any_of(m_displays.begin(), m_displays.end(), [&](const DisplayInfo &info) {
            return m_settings.forDisplay(info.stableId).isEink;
        });
        report(m_platform->setVisualEffectsReduced(any));
    }
    persistAndNotify();
    endOperation();
}

void ApplicationController::setSaturation(const QString &id, double value, int preset) {
    DisplaySettings &s=settingsFor(id); s.saturation=std::max(0.0,std::min(value,3.0)); s.saturationPreset=preset;
    persistAndNotify(); queueColorApply(id);
}

void ApplicationController::setRgb(const QString &id, const RgbBalance &rgb) {
    DisplaySettings &s=settingsFor(id);
    s.rgb.red=std::max(0.0,std::min(rgb.red,2.0));
    s.rgb.green=std::max(0.0,std::min(rgb.green,2.0));
    s.rgb.blue=std::max(0.0,std::min(rgb.blue,2.0));
    persistAndNotify(); queueColorApply(id);
}

void ApplicationController::setTextLevel(const QString &id, TextLevel level) {
    beginOperation();
    DisplaySettings &s=settingsFor(id); s.textLevel=level;
    if (level!=TextLevel::Off) s.enhanceLevel=EnhanceLevel::Off;
    applyCurve(id); persistAndNotify();endOperation();
}

void ApplicationController::setEnhanceLevel(const QString &id, EnhanceLevel level) {
    beginOperation();
    DisplaySettings &s=settingsFor(id); s.enhanceLevel=level;
    if (level!=EnhanceLevel::Off) s.textLevel=TextLevel::Off;
    applyCurve(id); persistAndNotify();endOperation();
}

void ApplicationController::setAdvanced(const QString &id, bool enabled) {
    beginOperation();settingsFor(id).advanced=enabled; applyCurve(id); persistAndNotify();endOperation();
}

void ApplicationController::setCustomCurve(const QString &id, const ToneCurve &curve) {
    beginOperation();settingsFor(id).customCurve=curve; applyCurve(id); persistAndNotify();endOperation();
}

void ApplicationController::setReduceShaking(const QString &id, bool enabled) {
    beginOperation();
    DisplaySettings &s=settingsFor(id); s.reduceShaking=enabled;
    if (const DisplayInfo *d=displayById(id); d && s.isEink && d->ditheringControlSupported)
        report(m_platform->setDitheringDisabled(*d,enabled));
    persistAndNotify();endOperation();
}

void ApplicationController::setVisualEffects(bool enabled) {
    beginQuietOperation();m_systemPool.start(QRunnable::create([this,enabled]{
        const ApplyResult result=m_platform->setVisualEffectsReduced(enabled);
        QMetaObject::invokeMethod(this,[this,enabled,result]{
            if(m_shutDown.load()){endQuietOperation();return;}report(result);
            if(result.success){m_settings.reduceVisualEffects=enabled;persistAndNotify();}else emit stateChanged();
            emit visualEffectsStateChanged(m_platform->visualEffectsReduced());endQuietOperation();
        },Qt::QueuedConnection);
    }));
}

void ApplicationController::setWindowsLightMode(bool enabled) {
    if(!m_windowsLightModeOwned&&m_platform->windowsLightModeAvailable()) {
        m_originalWindowsLightMode=m_platform->windowsLightModeEnabled();
        m_windowsLightModeOwned=true;
    }
    beginQuietOperation();m_systemPool.start(QRunnable::create([this,enabled]{
        const ApplyResult result=m_platform->setWindowsLightModeEnabled(enabled);
        QMetaObject::invokeMethod(this,[this,result]{
            if(m_shutDown.load()){endQuietOperation();return;}report(result);emit stateChanged();
            emit windowsLightModeStateChanged(m_platform->windowsLightModeEnabled());endQuietOperation();
        },Qt::QueuedConnection);
    }));
}

void ApplicationController::setNightLightDisabled(bool disabled) {
    if(m_shutDown.load())return;
    beginQuietOperation();m_systemPool.start(QRunnable::create([this,disabled]{
        const ApplyResult result=m_platform->setNightLightDisabled(disabled);
        QMetaObject::invokeMethod(this,[this,result]{
            if(m_shutDown.load()){endQuietOperation();return;}report(result);
            if(m_platform->nightLightControlAvailable()) {
                m_lastNightLightDisabled=!m_platform->nightLightEnabled();m_nightLightStateKnown=true;
                emit nightLightStateChanged(m_lastNightLightDisabled);
            }
            emit stateChanged();emit nightLightOperationFinished();endQuietOperation();
        },Qt::QueuedConnection);
    }));
}

void ApplicationController::refreshNightLightState() {
    if(m_shutDown.load()||operationInProgress()||!m_platform->nightLightControlAvailable())return;
    const bool disabled=!m_platform->nightLightEnabled();
    if(!m_nightLightStateKnown||disabled!=m_lastNightLightDisabled) {
        m_lastNightLightDisabled=disabled;m_nightLightStateKnown=true;emit nightLightStateChanged(disabled);
    }
}

void ApplicationController::setAutoVisualEffects(bool enabled) {
    m_settings.autoVisualEffects=enabled; persistAndNotify();
}

void ApplicationController::setLaunchAtLogin(bool enabled) {
    const ApplyResult result=m_platform->setLaunchAtLogin(enabled); report(result);
    if (result.success) m_settings.launchAtLogin=enabled;
    persistAndNotify();
}

void ApplicationController::openGpuControlPanel(const QString &displayId) {
    const DisplayInfo *display=displayById(displayId);
    if(!display)return;
    report(m_platform->openGpuControlPanel(*display));
}

void ApplicationController::setLanguage(const QString &language) {
    m_settings.language=language; persistAndNotify();
}

void ApplicationController::setShowWelcome(bool enabled) {
    m_settings.showWelcome=enabled; persistAndNotify();
}

void ApplicationController::setTrayDiscoveryShown(bool shown,const QString &executablePath,int version) {
    m_settings.trayDiscoveryShown=shown;
    m_settings.trayDiscoveryVersion=shown?version:0;
    if(!executablePath.isEmpty())m_settings.trayDiscoveryExecutablePath=executablePath;
    persistAndNotify();
}

void ApplicationController::saveCurve(int slot, const ToneCurve &curve) {
    if (slot<0 || slot>=m_settings.savedCurves.size()) return;
    m_settings.savedCurves[slot].occupied=true; m_settings.savedCurves[slot].curve=curve; persistAndNotify();
}

void ApplicationController::applySavedCurve(int slot, const QString &displayId) {
    if (slot<0 || slot>=m_settings.savedCurves.size() || !m_settings.savedCurves[slot].occupied) return;
    DisplaySettings &s=settingsFor(displayId); s.customCurve=m_settings.savedCurves[slot].curve; s.advanced=true;
    applyCurve(displayId); persistAndNotify();
}

void ApplicationController::renameCurve(int slot, const QString &name) {
    if (slot<0 || slot>=m_settings.savedCurves.size()) return;
    m_settings.savedCurves[slot].name=name.trimmed(); persistAndNotify();
}

void ApplicationController::clearCurve(int slot) {
    if (slot<0 || slot>=m_settings.savedCurves.size()) return;
    m_settings.savedCurves[slot]=SavedCurve{}; persistAndNotify();
}

void ApplicationController::applyCurve(const QString &id) {
    const DisplayInfo *found=displayById(id); if (!found) return;const DisplayInfo d=*found;
    const DisplaySettings &s=settingsFor(id);
    if (!s.isEink || s.effectiveCurve().isIdentity()) report(m_platform->restoreToneCurve(d));
    else report(m_platform->applyToneCurve(d,s.effectiveCurve()));
}

void ApplicationController::applyColor(const QString &id) {
    const DisplayInfo *found=displayById(id); if (!found) return;const DisplayInfo d=*found;
    const DisplaySettings &s=settingsFor(id);
    const bool profileAvailable=m_platform->saturationPlatformAvailable()&&d.colorAdjustmentSupported;
    if (!s.isEink||!profileAvailable) report(m_platform->restoreColor(d));
    else report(m_platform->applyColor(d,s.saturation,s.rgb));
}

void ApplicationController::queueColorApply(const QString &id) {
    const DisplayInfo *display=displayById(id); if(!display)return;
    beginOperation();
    const DisplaySettings &settings=settingsFor(id);
    const DisplayInfo displayCopy=*display;
    const double saturation=settings.saturation;
    const RgbBalance rgb=settings.rgb;
    const bool shouldRestore=!settings.isEink||!m_platform->saturationPlatformAvailable()||!displayCopy.colorAdjustmentSupported;
    m_colorPool.start(QRunnable::create([this,id,displayCopy,saturation,rgb,shouldRestore]{
        const ApplyResult result=shouldRestore?m_platform->restoreColor(displayCopy):m_platform->applyColor(displayCopy,saturation,rgb);
        QMetaObject::invokeMethod(this,[this,id,result]{
            if(m_shutDown.load()){endOperation();return;}
            report(result);
            applyCurve(id);
            emit colorApplyFinished(id);
            endOperation();
        },Qt::QueuedConnection);
    }));
}

void ApplicationController::waitForColorTasks() { m_colorPool.waitForDone(); }
void ApplicationController::waitForPendingOperations() { waitForColorTasks();m_systemPool.waitForDone(); }
void ApplicationController::beginOperation(){m_operationCount.fetch_add(1);emit operationStarted();}
void ApplicationController::endOperation(){const int previous=m_operationCount.fetch_sub(1);if(previous<=0)m_operationCount.store(0);emit operationFinished();}
void ApplicationController::beginQuietOperation(){m_operationCount.fetch_add(1);}
void ApplicationController::endQuietOperation(){const int previous=m_operationCount.fetch_sub(1);if(previous<=0)m_operationCount.store(0);}

void ApplicationController::reapplyAll() {
    const bool mayQueue=!m_initializing&&QThread::currentThread()==thread();
    if(!mayQueue)waitForColorTasks();
    const QVector<DisplayInfo> displaysSnapshot=m_displays;
    for (const DisplayInfo &d:displaysSnapshot) {
        const QString stableId=d.stableId;const DisplaySettings &s=settingsFor(stableId); if (!s.isEink) continue;
        if(mayQueue)queueColorApply(stableId);else applyColor(stableId);
        applyCurve(stableId);
        if (s.reduceShaking && d.ditheringControlSupported) report(m_platform->setDitheringDisabled(d,true));
    }
    if (m_settings.autoVisualEffects) {
        const bool any=std::any_of(m_displays.begin(),m_displays.end(),[&](const DisplayInfo &d){return settingsFor(d.stableId).isEink;});
        if(any)report(m_platform->setVisualEffectsReduced(true));
    }
}

void ApplicationController::shutdown() {
    if (m_shutDown.exchange(true) || !m_platform) return;
    waitForColorTasks();
    m_systemPool.waitForDone();
    const QVector<DisplayInfo> displaysSnapshot=m_displays;
    for (const DisplayInfo &d:displaysSnapshot) {
        m_platform->restoreToneCurve(d); m_platform->restoreColor(d);
        if (d.ditheringControlSupported) m_platform->setDitheringDisabled(d,false);
    }
    m_platform->setVisualEffectsReduced(false);
    if(m_windowsLightModeOwned)
        m_platform->setWindowsLightModeEnabled(m_originalWindowsLightMode);
    m_platform->shutdown();
}

void ApplicationController::persistAndNotify() { m_store.save(m_settings); emit stateChanged(); }
void ApplicationController::report(const ApplyResult &result) {
    if (!result.success) {
        m_lastError=result.error;
        emit errorChanged(result.error);
    }
}

} // namespace eink
