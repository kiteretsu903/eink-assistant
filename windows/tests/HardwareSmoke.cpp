#include "platform/windows/WindowsPlatformServices.h"

#include <QCoreApplication>
#include <QTextStream>

using namespace eink;

int main(int argc,char **argv){QCoreApplication app(argc,argv);WindowsPlatformServices platform;QTextStream out(stdout),err(stderr);
    const bool loginEnable=app.arguments().contains(QStringLiteral("--launch-at-login-enable"));
    const bool loginDisable=app.arguments().contains(QStringLiteral("--launch-at-login-disable"));
    const bool loginQuery=app.arguments().contains(QStringLiteral("--launch-at-login-query"));
    if(loginEnable||loginDisable||loginQuery){
        if(loginEnable||loginDisable){const ApplyResult changed=platform.setLaunchAtLogin(loginEnable);if(!changed.success){err<<changed.error<<Qt::endl;return 3;}}
        const bool actual=platform.launchAtLogin();out<<"Launch at login="<<actual<<Qt::endl;
        if((loginEnable&&!actual)||(loginDisable&&actual))return 4;
        return 0;
    }
    out<<platform.platformDiagnostic()<<Qt::endl;const auto displays=platform.displays();if(displays.isEmpty()){err<<"No active displays were enumerated."<<Qt::endl;return 2;}int failures=0;for(const DisplayInfo &d:displays){out<<d.friendlyName<<" | "<<d.deviceName<<" | source="<<d.sourceId<<" target="<<d.targetId<<" clone="<<d.cloneMode<<" peers="<<d.clonePeerNames.join(QStringLiteral(", "))<<" | Color path="<<d.colorAdjustmentSupported<<" Windows 10 MHC2="<<d.usesWindows10Mhc2<<" MatrixDDI="<<d.matrixDdiSupported<<" WDDM="<<d.wddmVersion<<" fingerprint="<<d.colorCapabilityFingerprint<<" | ACM supported="<<d.acmSupported<<" enabled="<<d.acmEnabled<<" | profile="<<platform.defaultProfileForDiagnostics(d)<<Qt::endl;const ApplyResult apply=platform.applyToneCurve(d,ToneCurve::identity());if(!apply.success){err<<apply.error<<Qt::endl;++failures;}else{const ApplyResult restore=platform.restoreToneCurve(d);if(!restore.success){err<<restore.error<<Qt::endl;++failures;}}}
    if(app.arguments().contains(QStringLiteral("--acm-roundtrip"))&&displays[0].acmSupported){const DisplayInfo d=displays[0];const bool original=d.acmEnabled;out<<"ACM round trip on "<<d.friendlyName<<" (original="<<original<<")"<<Qt::endl;if(!original){const ApplyResult enabled=platform.setAcmForDiagnostics(d,true);if(!enabled.success){err<<enabled.error<<Qt::endl;++failures;}}if(failures==0){const ApplyResult gamma=platform.applyToneCurve(d,ToneCurve::identity());if(!gamma.success){err<<gamma.error<<Qt::endl;++failures;}const ApplyResult restored=platform.restoreToneCurve(d);if(!restored.success){err<<restored.error<<Qt::endl;++failures;}}if(!original){const ApplyResult disabled=platform.setAcmForDiagnostics(d,false);if(!disabled.success){err<<disabled.error<<Qt::endl;++failures;}}const auto after=platform.displays();for(const DisplayInfo &check:after)if(check.stableId==d.stableId&&check.acmEnabled!=original){err<<"ACM state was not restored."<<Qt::endl;++failures;}}
    const bool reduce=app.arguments().contains(QStringLiteral("--visual-effects-reduce"));
    const bool enable=app.arguments().contains(QStringLiteral("--visual-effects-enable"));
    if(reduce||enable){const ApplyResult changed=platform.setVisualEffectsReduced(reduce);if(!changed.success){err<<changed.error<<Qt::endl;++failures;}const bool actual=platform.visualEffectsReduced();out<<"Visual effects reduced="<<actual<<Qt::endl;if(actual!=reduce){err<<"Visual effects state did not match the requested value."<<Qt::endl;++failures;}}
    const bool lightModeOn=app.arguments().contains(QStringLiteral("--light-mode-enable"));
    const bool lightModeOff=app.arguments().contains(QStringLiteral("--light-mode-disable"));
    if(lightModeOn||lightModeOff){const ApplyResult changed=platform.setWindowsLightModeEnabled(lightModeOn);if(!changed.success){err<<changed.error<<Qt::endl;++failures;}const bool actual=platform.windowsLightModeEnabled();out<<"Windows Light Mode enabled="<<actual<<Qt::endl;if(actual!=lightModeOn){err<<"Windows Light Mode state did not match the requested value."<<Qt::endl;++failures;}}
    if(app.arguments().contains(QStringLiteral("--light-mode-query")))out<<"Windows Light Mode available="<<platform.windowsLightModeAvailable()<<" enabled="<<platform.windowsLightModeEnabled()<<Qt::endl;
    platform.shutdown();return failures?1:0;}
