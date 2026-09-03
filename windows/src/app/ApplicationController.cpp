#include "ApplicationController.h"

#include <QMetaObject>
#include <QRunnable>
#include <QSet>
#include <QThread>
#include <algorithm>

namespace eink {

ApplicationController::ApplicationController(std::unique_ptr<PlatformServices> platform,
                                             SettingsStore store, QObject *parent)
    : QObject(parent), m_platform(std::move(platform)), m_store(std::move(store)) {
    m_colorPool.setMaxThreadCount(1);
    m_colorPool.setExpiryTimeout(5000);
    m_systemPool.setMaxThreadCount(1);m_systemPool.setExpiryTimeout(5000);
    m_colorSafetyTimer.setSingleShot(false);
    connect(&m_colorSafetyTimer,&QTimer::timeout,this,&ApplicationController::advanceColorSafetyTest);
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
    QVector<DisplayInfo> nextDisplays=m_platform->displays();
    const bool topologyChanged=m_displays.size()!=nextDisplays.size()||std::any_of(m_displays.cbegin(),m_displays.cend(),[&](const DisplayInfo &oldDisplay){
        const auto next=std::find_if(nextDisplays.cbegin(),nextDisplays.cend(),[&](const DisplayInfo &candidate){return candidate.stableId==oldDisplay.stableId;});
        return next==nextDisplays.cend()||next->adapterHigh!=oldDisplay.adapterHigh||next->adapterLow!=oldDisplay.adapterLow
            ||next->sourceId!=oldDisplay.sourceId||next->targetId!=oldDisplay.targetId
            ||next->cloneMode!=oldDisplay.cloneMode||next->cloneTargetIds!=oldDisplay.cloneTargetIds;
    });
    if(topologyChanged&&!m_displays.isEmpty()) {
        if(!m_colorSafetyDisplayId.isEmpty())finishColorSafetyRollback(m_colorSafetyDisplayId,false);
        waitForColorTasks();
        const QVector<DisplayInfo> oldDisplays=m_displays;
        for(const DisplayInfo &display:oldDisplays) {
            report(m_platform->restoreToneCurve(display));report(m_platform->restoreColor(display));
            if(display.ditheringControlSupported)report(m_platform->setDitheringDisabled(display,false));
        }
    }
    bool migratedDisplaySettings=false;
    for(const DisplayInfo &display:nextDisplays) {
        const auto canonical=std::find_if(m_settings.displays.cbegin(),m_settings.displays.cend(),[&](const DisplaySettings &settings){return settings.stableId==display.stableId;});
        if(canonical!=m_settings.displays.cend()||display.legacyStableId.isEmpty()||display.legacyStableId==display.stableId)continue;
        auto legacy=std::find_if(m_settings.displays.begin(),m_settings.displays.end(),[&](const DisplaySettings &settings){return settings.stableId==display.legacyStableId;});
        if(legacy!=m_settings.displays.end()){legacy->stableId=display.stableId;migratedDisplaySettings=true;}
    }
    m_displays=std::move(nextDisplays);
    std::stable_partition(m_displays.begin(),m_displays.end(),[](const DisplayInfo &display){return !display.builtIn;});
    QSet<QString> cloneOwners;bool normalizedCloneSettings=false;
    for(const DisplayInfo &display:m_displays) {
        DisplaySettings &settings=m_settings.forDisplay(display.stableId);
        if(!display.cloneMode||!settings.isEink)continue;
        if(cloneOwners.contains(display.cloneGroupKey)){settings.isEink=false;normalizedCloneSettings=true;}
        else cloneOwners.insert(display.cloneGroupKey);
    }
    if(migratedDisplaySettings||normalizedCloneSettings)m_store.save(m_settings);
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
    bool clearedClonePeer=false;
    if (d) {
        if (enabled) {
            if(d->cloneMode)for(const DisplayInfo &peer:m_displays) {
                if(peer.stableId==id||peer.cloneGroupKey!=d->cloneGroupKey)continue;
                DisplaySettings &peerSettings=settingsFor(peer.stableId);if(!peerSettings.isEink)continue;
                peerSettings.isEink=false;clearedClonePeer=true;report(m_platform->restoreToneCurve(peer));queueColorApply(peer.stableId);
                if(peer.ditheringControlSupported)report(m_platform->setDitheringDisabled(peer,false));
            }
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
    if(clearedClonePeer)emit displaysChanged();
    endOperation();
}

void ApplicationController::setSaturation(const QString &id, double value, int preset) {
    if(!colorControlsEnabled(id))return;
    DisplaySettings &s=settingsFor(id); s.saturation=std::max(0.0,std::min(value,3.0)); s.saturationPreset=preset;
    persistAndNotify(); queueColorApply(id);
}

void ApplicationController::setRgb(const QString &id, const RgbBalance &rgb) {
    if(!colorControlsEnabled(id))return;
    DisplaySettings &s=settingsFor(id);
    s.rgb.red=std::max(0.0,std::min(rgb.red,2.0));
    s.rgb.green=std::max(0.0,std::min(rgb.green,2.0));
    s.rgb.blue=std::max(0.0,std::min(rgb.blue,2.0));
    persistAndNotify(); queueColorApply(id);
}

bool ApplicationController::colorControlsEnabled(const QString &id) const {
    const DisplayInfo *display=displayById(id);
    if(!display||display->cloneMode||!m_platform->saturationPlatformAvailable()||!display->colorAdjustmentSupported)return false;
    if(!display->usesWindows10Mhc2)return true;
    const auto setting=std::find_if(m_settings.displays.cbegin(),m_settings.displays.cend(),[&](const DisplaySettings &value){return value.stableId==id;});
    return setting!=m_settings.displays.cend()&&setting->experimentalColorEnabled
        &&!display->colorCapabilityFingerprint.isEmpty()
        &&setting->confirmedColorFingerprint==display->colorCapabilityFingerprint
        &&setting->failedColorFingerprint!=display->colorCapabilityFingerprint;
}

bool ApplicationController::colorExperimentDenied(const QString &id) const {
    const DisplayInfo *display=displayById(id);if(!display||display->colorCapabilityFingerprint.isEmpty())return false;
    const auto setting=std::find_if(m_settings.displays.cbegin(),m_settings.displays.cend(),[&](const DisplaySettings &value){return value.stableId==id;});
    return setting!=m_settings.displays.cend()&&setting->failedColorFingerprint==display->colorCapabilityFingerprint;
}

bool ApplicationController::colorExperimentAvailable(const QString &id) const {
    const DisplayInfo *display=displayById(id);
    return display&&!display->cloneMode&&display->usesWindows10Mhc2&&display->colorAdjustmentSupported
        &&!display->colorCapabilityFingerprint.isEmpty()&&!colorExperimentDenied(id);
}

ColorSafetyPhase ApplicationController::colorSafetyPhase(const QString &id) const {
    return id==m_colorSafetyDisplayId?m_colorSafetyPhase:ColorSafetyPhase::Idle;
}

int ApplicationController::colorSafetySecondsRemaining(const QString &id) const {
    return id==m_colorSafetyDisplayId?m_colorSafetySecondsRemaining:0;
}

void ApplicationController::setColorSafetyTickIntervalForTests(int milliseconds) {
    m_colorSafetyTickIntervalMs=std::max(1,milliseconds);
}

void ApplicationController::setExperimentalColorEnabled(const QString &id,bool enabled) {
    const DisplayInfo *display=displayById(id);if(!display||!display->usesWindows10Mhc2)return;
    DisplaySettings &settings=settingsFor(id);
    if(!enabled) {
        if(colorSafetyPhase(id)!=ColorSafetyPhase::Idle)return;
        settings.experimentalColorEnabled=false;persistAndNotify();emit colorSafetyStateChanged(id,ColorSafetyPhase::Idle,0);queueColorApply(id);return;
    }
    if(!colorExperimentAvailable(id)||colorSafetyPhase(id)!=ColorSafetyPhase::Idle)return;
    if(settings.confirmedColorFingerprint==display->colorCapabilityFingerprint) {
        settings.experimentalColorEnabled=true;persistAndNotify();emit colorSafetyStateChanged(id,ColorSafetyPhase::Idle,0);queueColorApply(id);return;
    }
    if(!m_colorSafetyDisplayId.isEmpty())return;
    m_colorSafetyDisplayId=id;m_colorSafetyPhase=ColorSafetyPhase::Preparing;m_colorSafetySecondsRemaining=5;
    m_colorSafetyTimer.start(m_colorSafetyTickIntervalMs);
    emit colorSafetyStateChanged(id,m_colorSafetyPhase,m_colorSafetySecondsRemaining);
}

void ApplicationController::advanceColorSafetyTest() {
    if(m_colorSafetyDisplayId.isEmpty()){m_colorSafetyTimer.stop();return;}
    const QString id=m_colorSafetyDisplayId;
    if(--m_colorSafetySecondsRemaining>0) {
        emit colorSafetyStateChanged(id,m_colorSafetyPhase,m_colorSafetySecondsRemaining);return;
    }
    const DisplayInfo *display=displayById(id);
    if(!display){finishColorSafetyRollback(id,false);return;}
    if(m_colorSafetyPhase==ColorSafetyPhase::Preparing) {
        const ApplyResult result=m_platform->beginColorSafetyTest(*display,1.01,RgbBalance{},15);
        if(!result.success){report(result);finishColorSafetyRollback(id,true);return;}
        m_colorSafetyPhase=ColorSafetyPhase::AwaitingConfirmation;m_colorSafetySecondsRemaining=15;
        emit colorSafetyStateChanged(id,m_colorSafetyPhase,m_colorSafetySecondsRemaining);return;
    }
    finishColorSafetyRollback(id,false);
}

void ApplicationController::confirmExperimentalColor(const QString &id) {
    if(id!=m_colorSafetyDisplayId||m_colorSafetyPhase!=ColorSafetyPhase::AwaitingConfirmation)return;
    const QString stableId=id;const DisplayInfo *display=displayById(stableId);if(!display){finishColorSafetyRollback(stableId,false);return;}
    const ApplyResult result=m_platform->confirmColorSafetyTest(*display);
    if(!result.success){report(result);finishColorSafetyRollback(stableId,true);return;}
    m_colorSafetyTimer.stop();m_colorSafetyPhase=ColorSafetyPhase::Idle;m_colorSafetySecondsRemaining=0;m_colorSafetyDisplayId.clear();
    DisplaySettings &settings=settingsFor(stableId);settings.experimentalColorEnabled=true;
    settings.confirmedColorFingerprint=display->colorCapabilityFingerprint;settings.failedColorFingerprint.clear();
    settings.saturation=1.0;settings.saturationPreset=2;settings.rgb=RgbBalance{};
    persistAndNotify();emit colorSafetyStateChanged(stableId,ColorSafetyPhase::Idle,0);queueColorApply(stableId);
}

void ApplicationController::finishColorSafetyRollback(const QString &id,bool denyFingerprint) {
    const QString stableId=id;
    m_colorSafetyTimer.stop();
    m_colorSafetyPhase=ColorSafetyPhase::Idle;m_colorSafetySecondsRemaining=0;m_colorSafetyDisplayId.clear();
    if(const DisplayInfo *display=displayById(stableId)) {
        report(m_platform->rollbackColorSafetyTest(*display));
        DisplaySettings &settings=settingsFor(stableId);settings.experimentalColorEnabled=false;
        if(denyFingerprint&&!display->colorCapabilityFingerprint.isEmpty()) {
            settings.failedColorFingerprint=display->colorCapabilityFingerprint;
            settings.confirmedColorFingerprint.clear();
        }
        persistAndNotify();
    }
    emit colorSafetyStateChanged(stableId,ColorSafetyPhase::Idle,0);
}

void ApplicationController::rollbackExperimentalColor(const QString &id) {
    if(id==m_colorSafetyDisplayId&&m_colorSafetyPhase==ColorSafetyPhase::AwaitingConfirmation)
        finishColorSafetyRollback(id,false);
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

void ApplicationController::setHardwareSetupNoticeHidden(bool hidden) {
    m_settings.hideHardwareSetupNotice=hidden;persistAndNotify();
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
    const bool profileAvailable=colorControlsEnabled(id);
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
    const bool shouldRestore=!settings.isEink||!colorControlsEnabled(id);
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
    m_colorSafetyTimer.stop();
    if(!m_colorSafetyDisplayId.isEmpty()) {
        const QString id=m_colorSafetyDisplayId;
        if(const DisplayInfo *display=displayById(id))m_platform->rollbackColorSafetyTest(*display);
        m_colorSafetyDisplayId.clear();m_colorSafetyPhase=ColorSafetyPhase::Idle;m_colorSafetySecondsRemaining=0;
    }
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
