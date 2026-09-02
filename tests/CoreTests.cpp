#include "app/ApplicationController.h"
#include "core/IccProfile.h"
#include "core/NightLightStateCodec.h"
#include "core/SettingsStore.h"
#include "core/ToneCurve.h"
#include "ui/Localization.h"
#include "FakePlatform.h"
#ifdef Q_OS_WIN
#include "platform/windows/WindowsCompatibility.h"
#include "platform/windows/WindowsTrayIntegration.h"
#endif

#include <QTemporaryDir>
#include <QEventLoop>
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
    void windowsCompatibilityDecisions();
    void classicTrayBlobValidation();
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
    QCOMPARE(static_cast<int>(eink::windows::trayPromotionStrategyForBuild(7601)),static_cast<int>(eink::windows::TrayPromotionStrategy::LegacyCom));
    QCOMPARE(static_cast<int>(eink::windows::trayPromotionStrategyForBuild(9200)),static_cast<int>(eink::windows::TrayPromotionStrategy::LegacyCom));
    QCOMPARE(static_cast<int>(eink::windows::trayPromotionStrategyForBuild(15063)),static_cast<int>(eink::windows::TrayPromotionStrategy::LegacyCom));
    QCOMPARE(static_cast<int>(eink::windows::trayPromotionStrategyForBuild(16299)),static_cast<int>(eink::windows::TrayPromotionStrategy::ClassicIconStreams));
    QCOMPARE(static_cast<int>(eink::windows::trayPromotionStrategyForBuild(19044)),static_cast<int>(eink::windows::TrayPromotionStrategy::ClassicIconStreams));
    QCOMPARE(static_cast<int>(eink::windows::trayPromotionStrategyForBuild(26100)),static_cast<int>(eink::windows::TrayPromotionStrategy::PerIconRegistry));
#else
    QSKIP("Windows compatibility decisions are built only on Windows.");
#endif
}
void CoreTests::classicTrayBlobValidation(){
#ifdef Q_OS_WIN
    constexpr int headerSize=20,recordSize=1640,preferenceOffset=528;
    auto putDword=[](QByteArray &data,int offset,quint32 value){for(int i=0;i<4;++i)data[offset+i]=static_cast<char>((value>>(i*8))&0xff);};
    auto rot13=[](ushort value){if(value>='A'&&value<='Z')return static_cast<ushort>((value-'A'+13)%26+'A');if(value>='a'&&value<='z')return static_cast<ushort>((value-'a'+13)%26+'a');return value;};
    const QString executable=QStringLiteral("C:/Apps/EinkAssistant.exe");
    QByteArray blob(headerSize+recordSize,0);putDword(blob,0,headerSize);putDword(blob,12,1);
    const ushort *utf16=executable.utf16();for(int i=0;i<executable.size();++i){const ushort encoded=rot13(utf16[i]);blob[headerSize+i*2]=static_cast<char>(encoded&0xff);blob[headerSize+i*2+1]=static_cast<char>(encoded>>8);}
    bool matched=false,changed=false;QVERIFY(windows::patchClassicIconStreamsBlobForExecutable(&blob,executable,&matched,&changed));QVERIFY(matched);QVERIFY(changed);QCOMPARE(static_cast<uchar>(blob[headerSize+preferenceOffset]),static_cast<uchar>(2));
    changed=true;QVERIFY(windows::patchClassicIconStreamsBlobForExecutable(&blob,executable,&matched,&changed));QVERIFY(matched);QVERIFY(!changed);
    QByteArray malformed=blob;malformed.chop(1);const QByteArray before=malformed;QVERIFY(!windows::patchClassicIconStreamsBlobForExecutable(&malformed,executable,&matched,&changed));QCOMPARE(malformed,before);
    QByteArray unrelated=blob;QVERIFY(windows::patchClassicIconStreamsBlobForExecutable(&unrelated,QStringLiteral("C:/Other.exe"),&matched,&changed));QVERIFY(!matched);QVERIFY(!changed);
#else
    QSKIP("Windows tray blob validation is built only on Windows.");
#endif
}
void CoreTests::nightLightCodecMatchesReference(){const QByteArray disabled=QByteArray::fromHex("434201000A0201002A068995FCBE062A2B0E1343420100D00A02C614A9F6E2D3EFEAE6ED0100000000");const QByteArray enabled=QByteArray::fromHex("434201000A0201002A068995FCBE062A2B0E15434201001000D00A02C614A9F6E2D3EFEAE6ED0100000000");NightLightStateRecord d,e;QString error;QVERIFY2(NightLightStateCodec::decode(disabled,&d,&error),qPrintable(error));QVERIFY2(NightLightStateCodec::decode(enabled,&e,&error),qPrintable(error));QVERIFY(!d.enabled);QVERIFY(e.enabled);QCOMPARE(d.initialized,1);QCOMPARE(d.cloudTimestamp,quint64(1742670473));QCOMPARE(d.transitionFileTime,quint64(133871411809270569ULL));QCOMPARE(NightLightStateCodec::encode(d),disabled);QCOMPARE(NightLightStateCodec::encode(e),enabled);QByteArray malformed=enabled;malformed[24]=char(1);QVERIFY(!NightLightStateCodec::decode(malformed,&e,&error));}
void CoreTests::settingsRoundTrip(){QTemporaryDir dir;SettingsStore store(dir.filePath(QStringLiteral("settings.ini")));AppSettings s;s.language=QStringLiteral("ja");s.trayDiscoveryShown=true;s.trayDiscoveryVersion=kCurrentTrayDiscoveryVersion;s.trayDiscoveryExecutablePath=QStringLiteral("C:/Apps/EinkAssistant.exe");auto &d=s.forDisplay(QStringLiteral("abc"));d.isEink=true;d.saturation=2;d.rgb.red=1.2;s.savedCurves[0].occupied=true;s.savedCurves[0].name=QStringLiteral("Newsprint");store.save(s);const AppSettings loaded=store.load();QCOMPARE(loaded.language,QStringLiteral("ja"));QVERIFY(loaded.trayDiscoveryShown);QCOMPARE(loaded.trayDiscoveryVersion,kCurrentTrayDiscoveryVersion);QCOMPARE(loaded.trayDiscoveryExecutablePath,QStringLiteral("C:/Apps/EinkAssistant.exe"));QCOMPARE(loaded.displays.size(),1);QVERIFY(loaded.displays[0].isEink);QCOMPARE(loaded.displays[0].saturation,2.0);QCOMPARE(loaded.savedCurves[0].name,QStringLiteral("Newsprint"));}
void CoreTests::controllerEnforcesMutualExclusion(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("s.ini"))));c.initialize();c.setEink(QStringLiteral("fake-external"),true);c.setTextLevel(QStringLiteral("fake-external"),TextLevel::Solid);QCOMPARE(c.settingsFor(QStringLiteral("fake-external")).enhanceLevel,EnhanceLevel::Off);c.setEnhanceLevel(QStringLiteral("fake-external"),EnhanceLevel::Medium);QCOMPARE(c.settingsFor(QStringLiteral("fake-external")).textLevel,TextLevel::Off);QVERIFY(raw->curveApplyCalls>=2);c.shutdown();QVERIFY(raw->shutDown);}
void CoreTests::localizationResourcesLoad(){Localization::instance().setLanguage(QStringLiteral("en"));QCOMPARE(L("app.title"),QStringLiteral("E-Ink Assistant"));QVERIFY(L("saturation.mhc2.compat")!=QStringLiteral("saturation.mhc2.compat"));Localization::instance().setLanguage(QStringLiteral("ja"));QVERIFY(L("app.title")!=QStringLiteral("app.title"));QCOMPARE(L("night.title"),QStringLiteral("夜間モード"));QVERIFY(L("saturation.mhc2.compat")!=QStringLiteral("saturation.mhc2.compat"));Localization::instance().setLanguage(QStringLiteral("zh-Hans"));QCOMPARE(L("welcome.windows.tray.title"),QStringLiteral("系统托盘"));QVERIFY(L("system.lightMode.note").contains(QStringLiteral("墨水屏")));QVERIFY(!L("system.lightMode.note").contains(QStringLiteral("电子纸")));QVERIFY(L("saturation.mhc2.compat")!=QStringLiteral("saturation.mhc2.compat"));Localization::instance().setLanguage(QStringLiteral("zh-Hant"));QCOMPARE(L("night.open"),QStringLiteral("開啟夜間模式設定"));QVERIFY(L("saturation.mhc2.compat")!=QStringLiteral("saturation.mhc2.compat"));}
void CoreTests::colorChangesRunOffUiThread(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("s.ini"))));c.initialize();c.setEink(QStringLiteral("fake-external"),true);c.waitForPendingOperations();raw->operationLog.clear();c.setSaturation(QStringLiteral("fake-external"),1.3,3);c.waitForPendingOperations();QCOMPARE(raw->operationLog.size(),1);QCOMPARE(raw->operationLog[0],QStringLiteral("color.apply"));c.shutdown();}
void CoreTests::factoryKeepsIdentityProfileActive(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("s.ini"))));c.initialize();c.setEink(QStringLiteral("fake-external"),true);c.waitForPendingOperations();QVERIFY(raw->saturations.contains(QStringLiteral("fake-external")));QCOMPARE(raw->saturations.value(QStringLiteral("fake-external")),1.0);raw->operationLog.clear();c.setSaturation(QStringLiteral("fake-external"),1.0,2);c.waitForPendingOperations();QCOMPARE(raw->operationLog.size(),1);QCOMPARE(raw->operationLog[0],QStringLiteral("color.apply"));QVERIFY(raw->saturations.contains(QStringLiteral("fake-external")));c.shutdown();}
void CoreTests::crashRecoveryRunsBeforeColorReapply(){QTemporaryDir dir;const QString path=dir.filePath(QStringLiteral("s.ini"));AppSettings settings;auto &display=settings.forDisplay(QStringLiteral("fake-external"));display.isEink=true;SettingsStore(path).save(settings);auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();ApplicationController c(std::move(fake),SettingsStore(path));c.initialize();QCOMPARE(raw->recoveryCalls,1);QVERIFY(raw->operationLog.size()>=2);QCOMPARE(raw->operationLog.first(),QStringLiteral("color.recover"));QVERIFY(raw->operationLog.indexOf(QStringLiteral("color.apply"))>0);c.shutdown();}
void CoreTests::savedCurveLifecycle(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("s.ini"))));c.initialize();const ToneCurve newsprint{.72,2.4,.08,.96};c.saveCurve(0,newsprint);QVERIFY(c.settings().savedCurves[0].occupied);c.renameCurve(0,QStringLiteral("Newsprint"));QCOMPARE(c.settings().savedCurves[0].name,QStringLiteral("Newsprint"));c.setEink(QStringLiteral("fake-external"),true);c.applySavedCurve(0,QStringLiteral("fake-external"));QVERIFY(c.settingsFor(QStringLiteral("fake-external")).advanced);QCOMPARE(c.settingsFor(QStringLiteral("fake-external")).customCurve,newsprint);c.clearCurve(0);QVERIFY(!c.settings().savedCurves[0].occupied);c.shutdown();}
void CoreTests::nightLightSessionSyncAndRestore(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("night.ini"))));QSignalSpy changed(&c,&ApplicationController::nightLightStateChanged);c.initialize();QCOMPARE(raw->nightRecoveryCalls,1);QVERIFY(!raw->nightDisabled);QCOMPARE(raw->nightDisableCalls,0);c.setNightLightDisabled(true);QTRY_VERIFY(raw->nightDisabled);QTRY_COMPARE(changed.count(),1);QCOMPARE(raw->nightDisableCalls,1);QCOMPARE(changed.takeLast().at(0).toBool(),true);c.setNightLightDisabled(false);QTRY_VERIFY(!raw->nightDisabled);QTRY_COMPARE(changed.count(),1);QCOMPARE(raw->operationLog.last(),QStringLiteral("night.enable"));QCOMPARE(changed.takeLast().at(0).toBool(),false);raw->nightDisabled=true;c.refreshNightLightState();QCOMPARE(changed.takeLast().at(0).toBool(),true);c.shutdown();QVERIFY(!raw->nightDisabled);QCOMPARE(raw->nightRestoreCalls,1);}
void CoreTests::sessionLightModeRestoresAndBuiltInDisplayIsLast(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();std::reverse(raw->displayList.begin(),raw->displayList.end());raw->lightMode=true;ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("light.ini"))));c.initialize();QCOMPARE(raw->lightModeCalls,0);QVERIFY(!c.displays().first().builtIn);QVERIFY(c.displays().last().builtIn);c.setWindowsLightMode(false);QTRY_COMPARE(raw->lightModeCalls,1);QVERIFY(!raw->lightMode);c.shutdown();QVERIFY(raw->lightMode);QCOMPARE(raw->lightModeCalls,2);}
void CoreTests::pendingNightLightToggleCancelsDuringWorkerShutdown(){QTemporaryDir dir;auto fake=std::make_unique<FakePlatform>();FakePlatform *raw=fake.get();ApplicationController c(std::move(fake),SettingsStore(dir.filePath(QStringLiteral("shutdown.ini"))));c.initialize();c.shutdown();c.setNightLightDisabled(true);QCOMPARE(raw->nightDisableCalls,0);QVERIFY(raw->shutDown);}

QTEST_GUILESS_MAIN(CoreTests)
#include "CoreTests.moc"
