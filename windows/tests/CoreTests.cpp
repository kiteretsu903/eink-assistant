#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif
#include <windows.h>
#endif

#include "app/ApplicationController.h"
#include "core/IccProfile.h"
#include "core/NightLightStateCodec.h"
#include "core/SettingsStore.h"
#include "core/ToneCurve.h"
#include "ui/Localization.h"
#include "FakePlatform.h"
#ifdef Q_OS_WIN
#include "platform/windows/WindowsCompatibility.h"
#include "platform/windows/WindowsGpuControlPanel.h"
#endif

#include <QTemporaryDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QThread>
#include <QtTest>
#include <algorithm>
#include <atomic>
#include <thread>

using namespace eink;
using namespace eink::tests;

class CoreTests : public QObject {
    Q_OBJECT
private slots:
    void toneCurveEndpointsAndMonotonicity();
    void presetValuesMatchUpstream();
    void iccProfilesAreValidAndRoundTrip();
    void nightLightCodecMatchesReference();
    void settingsRoundTrip();
    void controllerEnforcesMutualExclusion();
    void localizationResourcesLoad();
    void colorChangesRunOffUiThread();
    void factoryKeepsIdentityProfileActive();
    void crashRecoveryRunsBeforeColorReapply();
    void savedCurveLifecycle();
    void nightLightSessionSyncAndRestore();
    void sessionLightModeRestoresAndBuiltInDisplayIsLast();
    void pendingNightLightToggleCancelsDuringWorkerShutdown();
    void nightLightCompletionIsEmittedOnFailure();
    void windowsCompatibilityDecisions();
    void gpuControlPanelResolutionRules();
    void liveDisplayGpuMappingProbe();
    void liveGpuPanelLaunchSmoke();
};

void CoreTests::toneCurveEndpointsAndMonotonicity(){const ToneCurve c{.45,.45,0,1};QCOMPARE(c.value(0),0.0);QCOMPARE(c.value(1),1.0);QVERIFY(c.value(.05)>.05);QCOMPARE(c.value(.45),.45);QVERIFY(c.isMonotonic());QCOMPARE(c.table().size(),256);}
void CoreTests::presetValuesMatchUpstream(){QCOMPARE(curveForTextLevel(TextLevel::Medium),ToneCurve({.65,2.10,0,1}));QCOMPARE(curveForTextLevel(TextLevel::Strong),ToneCurve({.80,2.70,0,1}));QCOMPARE(curveForTextLevel(TextLevel::Sharp),ToneCurve({1.00,5.00,.10,1.0}));QCOMPARE(curveForTextLevel(TextLevel::Solid),ToneCurve({1.00,6.00,.34,1.0}));QCOMPARE(curveForEnhanceLevel(EnhanceLevel::Medium),ToneCurve({.35,.60,0,1}));}
void CoreTests::iccProfilesAreValidAndRoundTrip(){for(double s:{0.0,0.5,1.0,1.3,2.0,3.0}){RgbBalance rgb{1.2,.9,1.05};const QByteArray profile=IccProfile::make(s,rgb,QStringLiteral("Test Saturation %1").arg(s));QString error;QVERIFY2(IccProfile::structurallyValid(profile,&error),qPrintable(error));QVERIFY(std::abs(IccProfile::matrixDeterminant(profile))>1e-10);QCOMPARE(IccProfile::description(profile),QStringLiteral("Test Saturation %1").arg(s));IccBaseProfile base;QVERIFY(IccProfile::parseBase(profile,&base));double mhc[9]{};QVERIFY(IccProfile::mhc2Matrix(profile,mhc));}const QByteArray identity=IccProfile::make(1.0,RgbBalance{},QStringLiteral("Identity"));double mhc[9]{};QVERIFY(IccProfile::mhc2Matrix(identity,mhc));for(int i=0;i<9;++i)QVERIFY(std::abs(mhc[i]-(i%4==0?1.0:0.0))<0.0001);}
void CoreTests::windowsCompatibilityDecisions(){
#ifdef Q_OS_WIN
    using eink::windows::ColorPipeline;
    using eink::windows::chooseColorPipeline;
    QCOMPARE(static_cast<int>(chooseColorPipeline(19044,false,false,false,false)),static_cast<int>(ColorPipeline::Unavailable));
    QCOMPARE(static_cast<int>(chooseColorPipeline(19044,true,false,false,false)),static_cast<int>(ColorPipeline::Windows10Mhc2));
    QCOMPARE(static_cast<int>(chooseColorPipeline(19044,true,true,true,false)),static_cast<int>(ColorPipeline::Windows10Mhc2));
    QCOMPARE(static_cast<int>(chooseColorPipeline(19044,true,true,false,false)),static_cast<int>(ColorPipeline::Unavailable));
    QCOMPARE(static_cast<int>(chooseColorPipeline(19040,true,false,false,false)),static_cast<int>(ColorPipeline::Unavailable));
    QCOMPARE(static_cast<int>(chooseColorPipeline(22631,true,false,false,true)),static_cast<int>(ColorPipeline::Unavailable));
    QCOMPARE(static_cast<int>(chooseColorPipeline(26100,true,false,false,true)),static_cast<int>(ColorPipeline::Windows11Acm));
    QCOMPARE(static_cast<int>(chooseColorPipeline(26100,true,false,false,false)),static_cast<int>(ColorPipeline::Unavailable));
    QCOMPARE(static_cast<int>(chooseColorPipeline(26100,false,false,false,true)),static_cast<int>(ColorPipeline::Unavailable));
    using eink::windows::NightLightControlPath;
    QCOMPARE(static_cast<int>(eink::windows::chooseNightLightControlPath(19040)),static_cast<int>(NightLightControlPath::Unavailable));
    QCOMPARE(static_cast<int>(eink::windows::chooseNightLightControlPath(19041)),static_cast<int>(NightLightControlPath::Windows10Registry));
    QCOMPARE(static_cast<int>(eink::windows::chooseNightLightControlPath(19045)),static_cast<int>(NightLightControlPath::Windows10Registry));
    QCOMPARE(static_cast<int>(eink::windows::chooseNightLightControlPath(22000)),static_cast<int>(NightLightControlPath::Windows11UiAutomation));
    QCOMPARE(static_cast<int>(eink::windows::chooseNightLightControlPath(26100)),static_cast<int>(NightLightControlPath::Windows11UiAutomation));
#else
    QSKIP("Windows compatibility decisions are built only on Windows.");
#endif
}
void CoreTests::gpuControlPanelResolutionRules(){
#ifdef Q_OS_WIN
    using namespace eink::windows;
    QCOMPARE(graphicsVendorFromPciVendorId(0x8086),GraphicsVendor::Intel);
    QCOMPARE(graphicsVendorFromPciVendorId(0x10de),GraphicsVendor::Nvidia);
    QCOMPARE(graphicsVendorFromPciVendorId(0x1002),GraphicsVendor::Amd);
    QCOMPARE(graphicsVendorFromPciVendorId(0x1414),GraphicsVendor::Unknown);
    QCOMPARE(graphicsVendorFromDeviceId(QStringLiteral("PCI\\VEN_10DE&DEV_1C82")),GraphicsVendor::Nvidia);
    QCOMPARE(graphicsVendorFromDeviceId(QStringLiteral("pci\\ven_8086&dev_5917")),GraphicsVendor::Intel);

    const auto nvidia=knownGpuControlPanelCandidates(GraphicsVendor::Nvidia,QStringLiteral("C:/PF"),QStringLiteral("C:/PF86"),QStringLiteral("C:/Windows"));
    QCOMPARE(nvidia.first().kind,GpuPanelLaunchKind::ShellApplication);
    QVERIFY(nvidia.first().target.contains(QStringLiteral("NVIDIAControlPanel")));
    const QString nvidiaLegacy=QStringLiteral("C:/PF/NVIDIA Corporation/Control Panel Client/nvcplui.exe");
    const auto selectedStore=selectGpuControlPanelCandidate(nvidia,{nvidia.first().target},{nvidiaLegacy});
    QCOMPARE(selectedStore.kind,GpuPanelLaunchKind::ShellApplication);
    const auto selectedLegacy=selectGpuControlPanelCandidate(nvidia,{}, {nvidiaLegacy.toUpper()});
    QCOMPARE(selectedLegacy.kind,GpuPanelLaunchKind::Executable);QCOMPARE(QDir::cleanPath(selectedLegacy.target),QDir::cleanPath(nvidiaLegacy));

    const auto intel=knownGpuControlPanelCandidates(GraphicsVendor::Intel,QStringLiteral("C:/PF"),QStringLiteral("C:/PF86"),QStringLiteral("C:/Windows"));
    QVERIFY(std::any_of(intel.cbegin(),intel.cend(),[](const GpuPanelLaunchCandidate &candidate){return candidate.target.endsWith(QStringLiteral("GfxUIEx.exe"));}));
    QVERIFY(std::any_of(intel.cbegin(),intel.cend(),[](const GpuPanelLaunchCandidate &candidate){return candidate.kind==GpuPanelLaunchKind::ControlPanelApplet&&candidate.target.endsWith(QStringLiteral("igfxcpl.cpl"));}));
    const auto amd=knownGpuControlPanelCandidates(GraphicsVendor::Amd,QStringLiteral("C:/PF"),QStringLiteral("C:/PF86"),QStringLiteral("C:/Windows"));
    QVERIFY(std::any_of(amd.cbegin(),amd.cend(),[](const GpuPanelLaunchCandidate &candidate){return candidate.target.endsWith(QStringLiteral("RadeonSoftware.exe"));}));
    QVERIFY(std::any_of(amd.cbegin(),amd.cend(),[](const GpuPanelLaunchCandidate &candidate){return candidate.target.endsWith(QStringLiteral("CCC.exe"));}));
    QVERIFY(!selectGpuControlPanelCandidate(amd,{},{}).isValid());
    QVERIFY(shellApplicationMatchesVendor(GraphicsVendor::Amd,{QStringLiteral("Localized name"),QStringLiteral("CNEventWindowClass")}));
    QVERIFY(shellApplicationMatchesVendor(GraphicsVendor::Intel,{QStringLiteral("Localized name"),QStringLiteral("AppUp.IntelGraphicsExperience_8j3eq9eme6ctt!App")}));
    QVERIFY(!shellApplicationMatchesVendor(GraphicsVendor::Nvidia,{QStringLiteral("NVIDIA"),QStringLiteral("com.nvidia.nvapp")}));
#else
    QSKIP("GPU control-panel resolution is built only on Windows.");
#endif
}
void CoreTests::liveDisplayGpuMappingProbe(){
#ifdef Q_OS_WIN
    UINT32 pathCount=0,modeCount=0;
    if(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&pathCount,&modeCount)!=ERROR_SUCCESS||!pathCount)QSKIP("No active DisplayConfig path is available.");
    QVector<DISPLAYCONFIG_PATH_INFO> paths(static_cast<int>(pathCount));QVector<DISPLAYCONFIG_MODE_INFO> modes(static_cast<int>(modeCount));
    if(QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&pathCount,paths.data(),&modeCount,modes.data(),nullptr)!=ERROR_SUCCESS)QSKIP("Active DisplayConfig paths could not be queried.");
    int dxgiMatches=0;
    for(UINT32 index=0;index<pathCount;++index) {
        const LUID luid=paths[static_cast<int>(index)].targetInfo.adapterId;
        const auto adapter=eink::windows::graphicsAdapterForLuid(luid.HighPart,luid.LowPart);
        if(adapter.name.isEmpty())continue;
        ++dxgiMatches;const auto panel=eink::windows::resolveGpuControlPanel(adapter.vendor);
        qInfo().noquote()<<QStringLiteral("Display path %1:%2 -> %3 (vendor 0x%4), panel=%5 target=%6")
            .arg(luid.HighPart).arg(luid.LowPart).arg(adapter.name).arg(adapter.pciVendorId,4,16,QLatin1Char('0'))
            .arg(static_cast<int>(panel.kind)).arg(panel.target);
        QCOMPARE(eink::windows::gpuControlPanelAvailable(adapter.vendor),panel.isValid());
        if(panel.kind==eink::windows::GpuPanelLaunchKind::Executable||panel.kind==eink::windows::GpuPanelLaunchKind::ControlPanelApplet)QVERIFY(QFileInfo(panel.target).isFile());
    }
    QVERIFY2(dxgiMatches>0,"No active DisplayConfig adapter LUID matched a DXGI adapter.");
#else
    QSKIP("Live display-to-GPU mapping is Windows-only.");
#endif
}
void CoreTests::liveGpuPanelLaunchSmoke(){
#ifdef Q_OS_WIN
    const QString requested=qEnvironmentVariable("EINK_GPU_PANEL_LAUNCH_TEST").trimmed().toLower();
    if(requested.isEmpty())QSKIP("Set EINK_GPU_PANEL_LAUNCH_TEST to amd, intel, or nvidia for the opt-in launch smoke test.");
    const GraphicsVendor vendor=requested==QStringLiteral("amd")?GraphicsVendor::Amd
        :requested==QStringLiteral("intel")?GraphicsVendor::Intel
        :requested==QStringLiteral("nvidia")?GraphicsVendor::Nvidia:GraphicsVendor::Unknown;
    QVERIFY2(vendor!=GraphicsVendor::Unknown,"EINK_GPU_PANEL_LAUNCH_TEST must be amd, intel, or nvidia.");
    const auto candidate=eink::windows::resolveGpuControlPanel(vendor);QVERIFY2(candidate.isValid(),"No installed control panel was resolved for the requested GPU vendor.");
    const ApplyResult launched=eink::windows::launchGpuControlPanel(vendor);QVERIFY2(launched.success,qPrintable(launched.error));
    qInfo().noquote()<<QStringLiteral("Launched GPU panel target: %1").arg(candidate.target);
#else
    QSKIP("GPU control-panel launch smoke testing is Windows-only.");
#endif
}
void CoreTests::nightLightCodecMatchesReference(){const QByteArray disabled=QByteArray::fromHex("434201000A0201002A068995FCBE062A2B0E1343420100D00A02C614A9F6E2D3EFEAE6ED0100000000");const QByteArray enabled=QByteArray::fromHex("434201000A0201002A068995FCBE062A2B0E15434201001000D00A02C614A9F6E2D3EFEAE6ED0100000000");NightLightStateRecord d,e;QString error;QVERIFY2(NightLightStateCodec::decode(disabled,&d,&error),qPrintable(error));QVERIFY2(NightLightStateCodec::decode(enabled,&e,&error),qPrintable(error));QVERIFY(!d.enabled);QVERIFY(e.enabled);QCOMPARE(d.initialized,1);QCOMPARE(d.cloudTimestamp,quint64(1742670473));QCOMPARE(d.transitionFileTime,quint64(133871411809270569ULL));QCOMPARE(NightLightStateCodec::encode(d),disabled);QCOMPARE(NightLightStateCodec::encode(e),enabled);QByteArray malformed=enabled;malformed[24]=char(1);QVERIFY(!NightLightStateCodec::decode(malformed,&e,&error));}
void CoreTests::settingsRoundTrip(){QTemporaryDir dir;SettingsStore store(dir.filePath(QStringLiteral("settings.ini")));AppSettings s;s.language=QStringLiteral("ja");s.trayDiscoveryShown=true;s.trayDiscoveryVersion=kCurrentTrayDiscoveryVersion;s.trayDiscoveryExecutablePath=QStringLiteral("C:/Apps/EinkAssistant.exe");auto &d=s.forDisplay(QStringLiteral("abc"));d.isEink=true;d.saturation=2;d.rgb.red=1.2;s.savedCurves[0].occupied=true;s.savedCurves[0].name=QStringLiteral("Newsprint");store.save(s);const AppSettings loaded=store.load();QCOMPARE(loaded.language,QStringLiteral("ja"));QVERIFY(loaded.trayDiscoveryShown);QCOMPARE(loaded.trayDiscoveryVersion,kCurrentTrayDiscoveryVersion);QCOMPARE(loaded.trayDiscoveryExecutablePath,QStringLiteral("C:/Apps/EinkAssistant.exe"));QCOMPARE(loaded.displays.size(),1);QVERIFY(loaded.displays[0].isEink);QCOMPARE(loaded.displays[0].saturation,2.0);QCOMPARE(loaded.savedCurves[0].name,QStringLiteral("Newsprint"));}
void CoreTests::controllerEnforcesMutualExclusion(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("s.ini"))));c.initialize();c.setEink(QStringLiteral("fake-external"),true);c.setTextLevel(QStringLiteral("fake-external"),TextLevel::Solid);QCOMPARE(c.settingsFor(QStringLiteral("fake-external")).enhanceLevel,EnhanceLevel::Off);c.setEnhanceLevel(QStringLiteral("fake-external"),EnhanceLevel::Medium);QCOMPARE(c.settingsFor(QStringLiteral("fake-external")).textLevel,TextLevel::Off);QVERIFY(raw->curveApplyCalls>=2);c.shutdown();QVERIFY(raw->shutDown);}
void CoreTests::localizationResourcesLoad(){Localization::instance().setLanguage(QStringLiteral("en"));QCOMPARE(L("app.title"),QStringLiteral("E-Ink Assistant"));QVERIFY(L("saturation.mhc2.compat")!=QStringLiteral("saturation.mhc2.compat"));QVERIFY(L("saturation.unsupported.driver").contains(QStringLiteral("current GPU/driver")));Localization::instance().setLanguage(QStringLiteral("ja"));QVERIFY(L("app.title")!=QStringLiteral("app.title"));QCOMPARE(L("night.title"),QStringLiteral("夜間モード"));QVERIFY(L("saturation.mhc2.compat")!=QStringLiteral("saturation.mhc2.compat"));QVERIFY(L("saturation.open.intel")!=QStringLiteral("saturation.open.intel"));Localization::instance().setLanguage(QStringLiteral("zh-Hans"));QCOMPARE(L("welcome.windows.tray.title"),QStringLiteral("固定任务栏图标"));QVERIFY(L("welcome.windows.tray").contains(QStringLiteral("书页图标")));QVERIFY(L("system.lightMode.note").contains(QStringLiteral("墨水屏")));QVERIFY(!L("system.lightMode.note").contains(QStringLiteral("电子纸")));QVERIFY(L("saturation.mhc2.compat")!=QStringLiteral("saturation.mhc2.compat"));QVERIFY(L("saturation.unsupported.upgrade").contains(QStringLiteral("可能")));Localization::instance().setLanguage(QStringLiteral("zh-Hant"));QCOMPARE(L("night.open"),QStringLiteral("開啟夜間模式設定"));QVERIFY(L("welcome.windows.tray").contains(QStringLiteral("書頁圖示")));QVERIFY(L("saturation.mhc2.compat")!=QStringLiteral("saturation.mhc2.compat"));QVERIFY(L("saturation.connectedGpu")!=QStringLiteral("saturation.connectedGpu"));}
void CoreTests::colorChangesRunOffUiThread(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("s.ini"))));c.initialize();c.setEink(QStringLiteral("fake-external"),true);c.waitForPendingOperations();raw->operationLog.clear();c.setSaturation(QStringLiteral("fake-external"),1.3,3);c.waitForPendingOperations();QCOMPARE(raw->operationLog.size(),1);QCOMPARE(raw->operationLog[0],QStringLiteral("color.apply"));c.shutdown();}
void CoreTests::factoryKeepsIdentityProfileActive(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("s.ini"))));c.initialize();c.setEink(QStringLiteral("fake-external"),true);c.waitForPendingOperations();QVERIFY(raw->saturations.contains(QStringLiteral("fake-external")));QCOMPARE(raw->saturations.value(QStringLiteral("fake-external")),1.0);raw->operationLog.clear();c.setSaturation(QStringLiteral("fake-external"),1.0,2);c.waitForPendingOperations();QCOMPARE(raw->operationLog.size(),1);QCOMPARE(raw->operationLog[0],QStringLiteral("color.apply"));QVERIFY(raw->saturations.contains(QStringLiteral("fake-external")));c.shutdown();}
void CoreTests::crashRecoveryRunsBeforeColorReapply(){QTemporaryDir dir;const QString path=dir.filePath(QStringLiteral("s.ini"));AppSettings settings;auto &display=settings.forDisplay(QStringLiteral("fake-external"));display.isEink=true;SettingsStore(path).save(settings);auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();ApplicationController c(std::move(fake),SettingsStore(path));c.initialize();QCOMPARE(raw->recoveryCalls,1);QVERIFY(raw->operationLog.size()>=2);QCOMPARE(raw->operationLog.first(),QStringLiteral("color.recover"));QVERIFY(raw->operationLog.indexOf(QStringLiteral("color.apply"))>0);c.shutdown();}
void CoreTests::savedCurveLifecycle(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("s.ini"))));c.initialize();const ToneCurve newsprint{.72,2.4,.08,.96};c.saveCurve(0,newsprint);QVERIFY(c.settings().savedCurves[0].occupied);c.renameCurve(0,QStringLiteral("Newsprint"));QCOMPARE(c.settings().savedCurves[0].name,QStringLiteral("Newsprint"));c.setEink(QStringLiteral("fake-external"),true);c.applySavedCurve(0,QStringLiteral("fake-external"));QVERIFY(c.settingsFor(QStringLiteral("fake-external")).advanced);QCOMPARE(c.settingsFor(QStringLiteral("fake-external")).customCurve,newsprint);c.clearCurve(0);QVERIFY(!c.settings().savedCurves[0].occupied);c.shutdown();}
void CoreTests::nightLightSessionSyncAndRestore(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("night.ini"))));QSignalSpy changed(&c,&ApplicationController::nightLightStateChanged);c.initialize();QCOMPARE(raw->nightRecoveryCalls,1);QVERIFY(!raw->nightDisabled);QCOMPARE(raw->nightDisableCalls,0);c.setNightLightDisabled(true);QTRY_VERIFY(raw->nightDisabled);QTRY_COMPARE(changed.count(),1);QCOMPARE(raw->nightDisableCalls,1);QCOMPARE(changed.takeLast().at(0).toBool(),true);c.setNightLightDisabled(false);QTRY_VERIFY(!raw->nightDisabled);QTRY_COMPARE(changed.count(),1);QCOMPARE(raw->operationLog.last(),QStringLiteral("night.enable"));QCOMPARE(changed.takeLast().at(0).toBool(),false);raw->nightDisabled=true;c.refreshNightLightState();QCOMPARE(changed.takeLast().at(0).toBool(),true);c.shutdown();QVERIFY(!raw->nightDisabled);QCOMPARE(raw->nightRestoreCalls,1);}
void CoreTests::sessionLightModeRestoresAndBuiltInDisplayIsLast(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();std::reverse(raw->displayList.begin(),raw->displayList.end());raw->lightMode=true;ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("light.ini"))));c.initialize();QCOMPARE(raw->lightModeCalls,0);QVERIFY(!c.displays().first().builtIn);QVERIFY(c.displays().last().builtIn);c.setWindowsLightMode(false);QTRY_COMPARE(raw->lightModeCalls,1);QVERIFY(!raw->lightMode);c.shutdown();QVERIFY(raw->lightMode);QCOMPARE(raw->lightModeCalls,2);}
void CoreTests::pendingNightLightToggleCancelsDuringWorkerShutdown(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("shutdown.ini"))));c.initialize();c.shutdown();c.setNightLightDisabled(true);QCOMPARE(raw->nightDisableCalls,0);QVERIFY(raw->shutDown);}
void CoreTests::nightLightCompletionIsEmittedOnFailure(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();raw->nightLightSetResult=ApplyResult::fail(QStringLiteral("simulated Night Light failure"));ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("night-failure.ini"))));c.initialize();QSignalSpy completed(&c,&ApplicationController::nightLightOperationFinished);c.setNightLightDisabled(true);QTRY_COMPARE(completed.count(),1);QCOMPARE(c.lastError(),QStringLiteral("simulated Night Light failure"));QVERIFY(!raw->nightDisabled);c.shutdown();}

QTEST_GUILESS_MAIN(CoreTests)
#include "CoreTests.moc"
