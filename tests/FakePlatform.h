#pragma once

#include "platform/PlatformServices.h"

#include <QHash>
#include <QThread>

namespace eink::tests {

class FakePlatform : public PlatformServices {
public:
    explicit FakePlatform(bool saturationAvailable=true,bool lightModeAvailable=true,bool directNightLightAvailable=true);
    ApplyResult recoverInterruptedColorState() override { ++recoveryCalls; operationLog<<QStringLiteral("color.recover"); return ApplyResult::ok(); }
    QVector<DisplayInfo> displays() override { return displayList; }
    ApplyResult applyToneCurve(const DisplayInfo &,const ToneCurve &) override;
    ApplyResult restoreToneCurve(const DisplayInfo &) override;
    ApplyResult applyColor(const DisplayInfo &,double,const RgbBalance &) override;
    ApplyResult restoreColor(const DisplayInfo &) override;
    ApplyResult setDitheringDisabled(const DisplayInfo &,bool) override;
    bool visualEffectsReduced() const override { return effectsReduced; }
    ApplyResult setVisualEffectsReduced(bool value) override { if(systemDelayMs>0)QThread::msleep(static_cast<unsigned long>(systemDelayMs));effectsReduced=value; ++effectsCalls; return ApplyResult::ok(); }
    bool windowsLightModeAvailable() const override { return lightModeAvailable; }
    bool windowsLightModeEnabled() const override { return lightMode; }
    ApplyResult setWindowsLightModeEnabled(bool value) override { if(systemDelayMs>0)QThread::msleep(static_cast<unsigned long>(systemDelayMs));lightMode=value; ++lightModeCalls; return ApplyResult::ok(); }
    ApplyResult setLaunchAtLogin(bool value) override { login=value; return ApplyResult::ok(); }
    bool launchAtLogin() const override { return login; }
    void openVisualEffectsSettings() override { ++openedSettings; }
    ApplyResult recoverInterruptedNightLightState() override { ++nightRecoveryCalls; operationLog<<QStringLiteral("night.recover"); return ApplyResult::ok(); }
    bool nightLightControlAvailable() const override { return directNightAvailable; }
    bool nightLightEnabled() const override { return !nightDisabled; }
    ApplyResult setNightLightDisabled(bool value) override { if(systemDelayMs>0)QThread::msleep(static_cast<unsigned long>(systemDelayMs));if(!nightOwned){originalNightDisabled=nightDisabled;nightOwned=true;} nightDisabled=value; ++nightDisableCalls; operationLog<<(value?QStringLiteral("night.disable"):QStringLiteral("night.enable")); return ApplyResult::ok(); }
    ApplyResult restoreNightLightState() override { if(nightOwned){nightDisabled=originalNightDisabled;nightOwned=false;++nightRestoreCalls;operationLog<<QStringLiteral("night.restore");} return ApplyResult::ok(); }
    bool nightLightAvailable() const override { return nightAvailable; }
    void openNightLightSettings() override { ++openedNightLight; }
    bool saturationPlatformAvailable() const override { return saturationAvailable; }
    QString platformDiagnostic() const override { return saturationAvailable?QStringLiteral("Fake Windows 11"):QStringLiteral("Fake Windows 7"); }
    void shutdown() override { restoreNightLightState();shutDown=true; }

    QVector<DisplayInfo> displayList;
    QHash<QString,ToneCurve> curves;
    QHash<QString,double> saturations;
    QHash<QString,RgbBalance> balances;
    int recoveryCalls=0,nightRecoveryCalls=0,nightDisableCalls=0,nightRestoreCalls=0,curveApplyCalls=0,curveRestoreCalls=0,colorApplyCalls=0,colorRestoreCalls=0,ditherCalls=0,effectsCalls=0,lightModeCalls=0;
    bool effectsReduced=false,lightMode=false,nightDisabled=false,originalNightDisabled=false,nightOwned=false,login=false,saturationAvailable=true,lightModeAvailable=true,nightAvailable=true,directNightAvailable=true,shutDown=false;
    int openedSettings=0,openedNightLight=0;
    int systemDelayMs=0;
    QStringList operationLog;
};

} // namespace eink::tests
