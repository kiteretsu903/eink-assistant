#include "app/ApplicationController.h"
#include "platform/windows/WindowsPlatformServices.h"
#include "platform/windows/WindowsTrayIntegration.h"
#include "ui/Localization.h"
#include "ui/MainPanel.h"
#include "ui/BusyDialog.h"
#include "ui/TrayIcon.h"
#include "ui/UiStyle.h"
#include "ui/WelcomeDialog.h"

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QCoreApplication>
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QEventLoop>
#include <QLockFile>
#include <QMenu>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QTextStream>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cwchar>
#include <functional>
#include <thread>
#ifdef Q_OS_WIN
#include <windows.h>
#include <commctrl.h>
#endif

using namespace eink;

#ifdef Q_OS_WIN
static int runColorSelfTest(const QString &resultPath) {
    QFile reportFile(resultPath);
    if(!reportFile.open(QIODevice::WriteOnly|QIODevice::Text))return 20;
    QTextStream report(&reportFile);
    WindowsPlatformServices platform;
    const QVector<DisplayInfo> displays=platform.displays();
    if(displays.isEmpty()){report<<"FAIL: no active displays\n";return 21;}
    const auto it=std::find_if(displays.begin(),displays.end(),[](const DisplayInfo &d){return d.colorAdjustmentSupported;});
    if(it==displays.end()){report<<"FAIL: no supported color-profile display\n";return 22;}
    const DisplayInfo display=*it;
    const bool originalAcm=display.acmEnabled;
    const QString originalProfile=platform.defaultProfileForDiagnostics(display);
    report<<"Display="<<display.friendlyName<<"\nOriginalACM="<<originalAcm<<"\nOriginalProfile="<<originalProfile<<"\n";
    const ApplyResult applied=platform.applyColor(display,1.01,RgbBalance{});
    if(!applied.success){report<<"FAIL apply: "<<applied.error<<"\n";platform.shutdown();return 23;}
    const QString installed=platform.defaultProfileForDiagnostics(display);
    report<<"InstalledProfile="<<installed<<"\n";
    if(!installed.startsWith(QStringLiteral("EinkAssistant-"))){report<<"FAIL: generated profile did not become default\n";platform.restoreColor(display);platform.shutdown();return 24;}
    const ApplyResult restored=platform.restoreColor(display);
    if(!restored.success){report<<"FAIL restore: "<<restored.error<<"\n";platform.shutdown();return 25;}
    bool restoredAcm=false;QString restoredProfile;
    for(const DisplayInfo &current:platform.displays())if(current.stableId==display.stableId)restoredAcm=current.acmEnabled;
    restoredProfile=platform.defaultProfileForDiagnostics(display);
    report<<"RestoredACM="<<restoredAcm<<"\nRestoredProfile="<<restoredProfile<<"\n";
    if(restoredAcm!=originalAcm||restoredProfile!=originalProfile){report<<"FAIL: original display state was not restored\n";platform.shutdown();return 26;}
    report<<"PASS\n";platform.shutdown();return 0;
}

static int runNightLightSelfTest(const QString &resultPath) {
    QFile reportFile(resultPath);if(!reportFile.open(QIODevice::WriteOnly|QIODevice::Text))return 70;
    QTextStream report(&reportFile);WindowsPlatformServices platform;
    const ApplyResult recovered=platform.recoverInterruptedNightLightState();
    if(!recovered.success){report<<"FAIL recovery: "<<recovered.error<<'\n';return 71;}
    if(!platform.nightLightControlAvailable()){report<<"FAIL: guarded Night Light control is unavailable\n";return 72;}
    const bool originalEnabled=platform.nightLightEnabled();report<<"OriginalEnabled="<<originalEnabled<<'\n';
    const ApplyResult disabled=platform.setNightLightDisabled(true);
    if(!disabled.success){report<<"FAIL disable: "<<disabled.error<<'\n';platform.shutdown();return 73;}
    const bool disabledState=!platform.nightLightEnabled();report<<"Disabled="<<disabledState<<'\n';
    const ApplyResult restored=platform.restoreNightLightState();
    const bool restoredState=platform.nightLightEnabled()==originalEnabled;
    report<<"Restored="<<restoredState<<'\n';
    if(!restored.success){report<<"FAIL restore: "<<restored.error<<'\n';platform.shutdown();return 74;}
    report<<(disabledState&&restoredState?"PASS\n":"FAIL state verification\n");platform.shutdown();return disabledState&&restoredState?0:75;
}

static int runNightLightCrashSeed(const QString &statePath) {
    WindowsPlatformServices platform;const ApplyResult recovered=platform.recoverInterruptedNightLightState();
    if(!recovered.success||!platform.nightLightControlAvailable())return 76;
    QSettings state(statePath,QSettings::IniFormat);state.clear();state.setValue(QStringLiteral("originalEnabled"),platform.nightLightEnabled());
    const ApplyResult disabled=platform.setNightLightDisabled(true);state.setValue(QStringLiteral("disableSuccess"),disabled.success);state.setValue(QStringLiteral("disableError"),disabled.error);state.setValue(QStringLiteral("disabled"),!platform.nightLightEnabled());state.sync();
    if(!disabled.success)return 77;
    TerminateProcess(GetCurrentProcess(),78);return 78;
}

static int runNightLightRecoveryTest(const QString &statePath) {
    QSettings state(statePath,QSettings::IniFormat);if(!state.value(QStringLiteral("disableSuccess")).toBool())return 79;
    const bool original=state.value(QStringLiteral("originalEnabled")).toBool();WindowsPlatformServices platform;
    const ApplyResult recovered=platform.recoverInterruptedNightLightState();const bool restored=recovered.success&&platform.nightLightEnabled()==original;
    state.setValue(QStringLiteral("recoverySuccess"),recovered.success);state.setValue(QStringLiteral("recoveryError"),recovered.error);state.setValue(QStringLiteral("restored"),restored);state.sync();platform.shutdown();return restored?0:80;
}

static int runColorCleanup(const QString &resultPath) {
    QFile reportFile(resultPath);
    if(!reportFile.open(QIODevice::WriteOnly|QIODevice::Text))return 30;
    QTextStream report(&reportFile);
    WindowsPlatformServices platform;
    const ApplyResult cleaned=platform.cleanupGeneratedColorForDiagnostics();
    if(!cleaned.success){report<<"FAIL: "<<cleaned.error<<"\n";platform.shutdown();return 31;}
    bool clean=true;
    for(const DisplayInfo &display:platform.displays()) {
        const QString profile=platform.defaultProfileForDiagnostics(display);
        report<<display.deviceName<<" ACM="<<display.acmEnabled<<" Profile="<<profile<<"\n";
        if(profile.startsWith(QStringLiteral("EinkAssistant-")) || !platform.generatedProfilesForDiagnostics(display).isEmpty())clean=false;
    }
    if(!platform.installedGeneratedProfilesForDiagnostics().isEmpty())clean=false;
    report<<(clean?"PASS\n":"FAIL: generated color state remains\n");
    platform.shutdown();return clean?0:32;
}

static int runLaunchAtLoginSelfTest(const QString &resultPath) {
    QFile reportFile(resultPath);
    if(!reportFile.open(QIODevice::WriteOnly|QIODevice::Text))return 33;
    QTextStream report(&reportFile);
    WindowsPlatformServices platform;
    const bool original=platform.launchAtLogin();
    const ApplyResult enabled=platform.setLaunchAtLogin(true);
    if(!enabled.success){report<<"FAIL enable: "<<enabled.error<<'\n';platform.shutdown();return 34;}
    const QString xml=platform.registeredLaunchTaskXmlForDiagnostics();
    const QString executable=QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const bool valid=platform.launchAtLogin()
        &&xml.contains(QStringLiteral("<LogonTrigger>"))
        &&xml.contains(QStringLiteral("<LogonType>InteractiveToken</LogonType>"))
        &&xml.contains(QStringLiteral("<RunLevel>HighestAvailable</RunLevel>"))
        &&xml.contains(QStringLiteral("<DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>"))
        &&xml.contains(QStringLiteral("<StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>"))
        &&xml.contains(QStringLiteral("<Arguments>--background</Arguments>"))
        &&xml.contains(executable,Qt::CaseInsensitive);
    report<<"OriginalEnabled="<<original<<'\n'<<"Registered="<<platform.launchAtLogin()<<'\n'
          <<"LogonTrigger="<<xml.contains(QStringLiteral("<LogonTrigger>"))<<'\n'
          <<"HighestRunLevel="<<xml.contains(QStringLiteral("<RunLevel>HighestAvailable</RunLevel>"))<<'\n'
          <<"AllowStartOnBattery="<<xml.contains(QStringLiteral("<DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>"))<<'\n'
          <<"ContinueOnBattery="<<xml.contains(QStringLiteral("<StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>"))<<'\n';
    if(!original) {
        const ApplyResult restored=platform.setLaunchAtLogin(false);
        if(!restored.success){report<<"FAIL restore: "<<restored.error<<'\n';platform.shutdown();return 35;}
    }
    report<<(valid?"PASS\n":"FAIL task definition\n");
    platform.shutdown();return valid?0:36;
}

static int runColorVisualTest(const QString &resultPath,double saturation,int holdSeconds) {
    QFile reportFile(resultPath);
    if(!reportFile.open(QIODevice::WriteOnly|QIODevice::Text))return 40;
    QTextStream report(&reportFile);
    WindowsPlatformServices platform;
    const QVector<DisplayInfo> displays=platform.displays();
    const auto it=std::find_if(displays.begin(),displays.end(),[](const DisplayInfo &display){return !display.builtIn&&display.colorAdjustmentSupported;});
    if(it==displays.end()){report<<"FAIL: no external display with a supported color path\n";return 41;}
    const DisplayInfo display=*it;
    const bool originalAcm=display.acmEnabled;
    const QString originalProfile=platform.defaultProfileForDiagnostics(display);
    const ApplyResult applied=platform.applyColor(display,saturation,RgbBalance{});
    if(!applied.success){report<<"FAIL: "<<applied.error<<"\n";platform.shutdown();return 42;}
    report<<"ACTIVE Display="<<display.friendlyName<<" Saturation="<<qRound(saturation*100)<<"% Profile="<<platform.defaultProfileForDiagnostics(display)<<" HoldSeconds="<<holdSeconds<<"\n";
    report.flush();reportFile.flush();
    Sleep(static_cast<DWORD>(std::max(1,holdSeconds))*1000);
    const ApplyResult restored=platform.restoreColor(display);
    bool restoredAcm=false;for(const DisplayInfo &current:platform.displays())if(current.stableId==display.stableId)restoredAcm=current.acmEnabled;
    const QString restoredProfile=platform.defaultProfileForDiagnostics(display);
    const bool clean=restored.success&&restoredAcm==originalAcm&&restoredProfile==originalProfile;
    report<<(clean?"PASS restored\n":"FAIL restore\n");platform.shutdown();return clean?0:43;
}

static int runColorCrashSeed(const QString &statePath) {
    WindowsPlatformServices platform;
    const ApplyResult recovered=platform.recoverInterruptedColorState();
    if(!recovered.success)return 49;
    const QVector<DisplayInfo> displays=platform.displays();
    const auto it=std::find_if(displays.begin(),displays.end(),[](const DisplayInfo &display){return !display.builtIn&&display.colorAdjustmentSupported;});
    if(it==displays.end())return 50;
    const DisplayInfo display=*it;
    QSettings state(statePath,QSettings::IniFormat); state.clear();
    state.setValue(QStringLiteral("stableId"),display.stableId);
    state.setValue(QStringLiteral("originalAcm"),display.acmEnabled);
    state.setValue(QStringLiteral("originalProfile"),platform.defaultProfileForDiagnostics(display));
    const ApplyResult applied=platform.applyColor(display,1.37,RgbBalance{.91,1.0,1.08});
    state.setValue(QStringLiteral("applySuccess"),applied.success);
    state.setValue(QStringLiteral("applyError"),applied.error);
    state.setValue(QStringLiteral("appliedProfile"),platform.defaultProfileForDiagnostics(display));
    state.sync();
    if(!applied.success)return 51;
    TerminateProcess(GetCurrentProcess(),77);
    return 77;
}

static int runColorRecoveryTest(const QString &statePath) {
    QSettings state(statePath,QSettings::IniFormat);
    const QString stableId=state.value(QStringLiteral("stableId")).toString();
    const bool originalAcm=state.value(QStringLiteral("originalAcm")).toBool();
    const QString originalProfile=state.value(QStringLiteral("originalProfile")).toString();
    if(stableId.isEmpty() || !state.value(QStringLiteral("applySuccess")).toBool())return 60;
    WindowsPlatformServices platform; const ApplyResult recovered=platform.recoverInterruptedColorState();
    if(!recovered.success)return 61;
    const QVector<DisplayInfo> displays=platform.displays();
    const auto it=std::find_if(displays.begin(),displays.end(),[&](const DisplayInfo &display){return display.stableId==stableId;});
    if(it==displays.end())return 62;
    const QStringList associated=platform.generatedProfilesForDiagnostics(*it);
    const QStringList installed=platform.installedGeneratedProfilesForDiagnostics();
    const QString restoredProfile=platform.defaultProfileForDiagnostics(*it);
    state.setValue(QStringLiteral("restoredAcm"),it->acmEnabled);
    state.setValue(QStringLiteral("restoredProfile"),restoredProfile);
    state.setValue(QStringLiteral("generatedAssociations"),associated);
    state.setValue(QStringLiteral("installedGeneratedProfiles"),installed);
    const bool clean=it->acmEnabled==originalAcm && restoredProfile==originalProfile && associated.isEmpty() && installed.isEmpty();
    state.setValue(QStringLiteral("recoveryPass"),clean); state.sync(); platform.shutdown();
    return clean?0:63;
}
#endif

#ifdef Q_OS_WIN
class DisplayEventFilter final : public QObject, public QAbstractNativeEventFilter {
public:
    explicit DisplayEventFilter(ApplicationController *controller):m_controller(controller){m_refresh.setSingleShot(true);m_refresh.setInterval(800);QObject::connect(&m_refresh,&QTimer::timeout,this,[this]{if(m_controller->operationInProgress()){m_refresh.start();return;}m_controller->beginOperation();m_controller->refreshDisplays();m_controller->reapplyAll();m_controller->endOperation();});}
    void stop(){m_refresh.stop();m_enabled=false;}
    bool nativeEventFilter(const QByteArray &,void *message,long *) override {
        const auto *msg=static_cast<MSG*>(message);
        if(msg && (msg->message==WM_QUERYENDSESSION || msg->message==WM_ENDSESSION)) {
            m_controller->shutdown();
        }
        if(m_enabled&&msg && (msg->message==WM_DISPLAYCHANGE || msg->message==WM_DEVICECHANGE || msg->message==WM_POWERBROADCAST))m_refresh.start();
        return false;
    }
private: ApplicationController *m_controller;QTimer m_refresh;bool m_enabled=true;
};
#endif

int main(int argc,char **argv) {
    QCoreApplication::setApplicationName(QStringLiteral("E-Ink Assistant"));
    QCoreApplication::setOrganizationName(QStringLiteral("EinkAssistant"));
#ifdef Q_OS_WIN
    for(int index=1;index+1<argc;++index) {
        if(std::strcmp(argv[index],"--color-broker")==0) {
            QCoreApplication brokerApp(argc,argv);
            return WindowsPlatformServices::runColorBroker(QString::fromLocal8Bit(argv[index+1]));
        }
        if(std::strcmp(argv[index],"--color-self-test")==0) {
            QCoreApplication selfTestApp(argc,argv);
            return runColorSelfTest(QString::fromLocal8Bit(argv[index+1]));
        }
        if(std::strcmp(argv[index],"--night-light-self-test")==0) {
            QCoreApplication selfTestApp(argc,argv);return runNightLightSelfTest(QString::fromLocal8Bit(argv[index+1]));
        }
        if(std::strcmp(argv[index],"--night-light-crash-seed")==0) {
            QCoreApplication crashApp(argc,argv);return runNightLightCrashSeed(QString::fromLocal8Bit(argv[index+1]));
        }
        if(std::strcmp(argv[index],"--night-light-recovery-test")==0) {
            QCoreApplication recoveryApp(argc,argv);return runNightLightRecoveryTest(QString::fromLocal8Bit(argv[index+1]));
        }
        if(std::strcmp(argv[index],"--color-cleanup")==0) {
            QCoreApplication cleanupApp(argc,argv);
            return runColorCleanup(QString::fromLocal8Bit(argv[index+1]));
        }
        if(std::strcmp(argv[index],"--launch-at-login-self-test")==0) {
            QCoreApplication loginTestApp(argc,argv);
            return runLaunchAtLoginSelfTest(QString::fromLocal8Bit(argv[index+1]));
        }
        if(std::strcmp(argv[index],"--color-visual-test")==0&&index+3<argc) {
            QCoreApplication visualTestApp(argc,argv);
            return runColorVisualTest(QString::fromLocal8Bit(argv[index+1]),QString::fromLocal8Bit(argv[index+2]).toDouble()/100.0,QString::fromLocal8Bit(argv[index+3]).toInt());
        }
        if(std::strcmp(argv[index],"--color-crash-seed")==0) {
            QCoreApplication crashApp(argc,argv);
            return runColorCrashSeed(QString::fromLocal8Bit(argv[index+1]));
        }
        if(std::strcmp(argv[index],"--color-recovery-test")==0) {
            QCoreApplication recoveryApp(argc,argv);
            return runColorRecoveryTest(QString::fromLocal8Bit(argv[index+1]));
        }
    }
#endif
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);app.setWindowIcon(QIcon(QStringLiteral(":/app-icon.png")));
    QString baseStyle=app.style()->objectName();if(baseStyle.isEmpty())baseStyle=QStringLiteral("windowsvista");app.setStyle(new ui::SmoothProxyStyle(baseStyle));
    const QStringList arguments=app.arguments();
#ifdef Q_OS_WIN
    const int trayAnchorTestIndex=arguments.indexOf(QStringLiteral("--tray-anchor-test"));
    if(trayAnchorTestIndex>=0&&trayAnchorTestIndex+1<arguments.size()) {
        QSystemTrayIcon diagnosticTray(ui::bookPagesTrayIcon(ui::systemTrayUsesLightBackground()));diagnosticTray.setToolTip(QStringLiteral("E-Ink Assistant tray anchor test"));diagnosticTray.show();
        QTimer::singleShot(500,&diagnosticTray,[]{windows::promoteOwnTrayIcon();windows::ownPromotedTrayIconRects();});
        QTimer::singleShot(1200,&diagnosticTray,[&]{
            QFile output(arguments[trayAnchorTestIndex+1]);
            if(output.open(QIODevice::WriteOnly|QIODevice::Text)) {
                QTextStream stream(&output);const QVector<QRect> native=windows::ownPromotedTrayIconRects();
                for(int index=0;index<native.size();++index)stream<<"private"<<index<<'='<<native[index].x()<<','<<native[index].y()<<','<<native[index].width()<<','<<native[index].height()<<'\n';
                for(QScreen *screen:QGuiApplication::screens())stream<<"screen="<<screen->name()<<','<<screen->geometry().x()<<','<<screen->geometry().y()<<','<<screen->geometry().width()<<','<<screen->geometry().height()<<'\n';
                const QRect qt=diagnosticTray.geometry();stream<<"qt="<<qt.x()<<','<<qt.y()<<','<<qt.width()<<','<<qt.height()<<'\n';
            }
            diagnosticTray.hide();app.quit();
        });
        return app.exec();
    }
#endif
    QLockFile lock(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(QStringLiteral("EinkAssistant.lock")));lock.setStaleLockTime(5000);
    bool locked=lock.tryLock(100);
    if(!locked&&lock.removeStaleLockFile())locked=lock.tryLock(100);
    if(!locked) {
#ifdef Q_OS_WIN
        if(HWND existing=FindWindowW(nullptr,L"E-Ink Assistant")) {ShowWindow(existing,SW_SHOWNORMAL);SetForegroundWindow(existing);}
#endif
        return 0;
    }
    SettingsStore startupStore;Localization::instance().setLanguage(startupStore.load().language);BusyDialog busy;
    const bool backgroundLaunch=arguments.contains(QStringLiteral("--background"));if(!backgroundLaunch){busy.setMessage(L("busy.starting"));busy.showCentered();app.processEvents();}
    auto services=std::make_unique<WindowsPlatformServices>();ApplicationController controller(std::move(services));std::atomic_bool initialized{false};std::thread initializer([&]{controller.initialize();initialized.store(true);});
    while(!initialized.load()){app.processEvents(QEventLoop::AllEvents,20);std::this_thread::sleep_for(std::chrono::milliseconds(16));}initializer.join();busy.hide();
    const QString currentExecutable=QDir::cleanPath(QCoreApplication::applicationFilePath());
    const bool showTrayDiscovery=!controller.settings().trayDiscoveryShown
        ||controller.settings().trayDiscoveryVersion<kCurrentTrayDiscoveryVersion
        || QString::compare(QDir::cleanPath(controller.settings().trayDiscoveryExecutablePath),currentExecutable,Qt::CaseInsensitive)!=0;
    Localization::instance().setLanguage(controller.settings().language);MainPanel panel(&controller);WelcomeDialog welcome(&controller);
    bool trayLightBackground=ui::systemTrayUsesLightBackground();QSystemTrayIcon tray(ui::bookPagesTrayIcon(trayLightBackground));tray.setToolTip(L("app.title"));QMenu menu;auto *open=menu.addAction(L("app.title"));menu.addSeparator();auto *quit=menu.addAction(L("quit"));tray.setContextMenu(&menu);
    QTimer trayThemeTimer;trayThemeTimer.setInterval(2000);QObject::connect(&trayThemeTimer,&QTimer::timeout,&tray,[&]{const bool light=ui::systemTrayUsesLightBackground();if(light!=trayLightBackground){trayLightBackground=light;tray.setIcon(ui::bookPagesTrayIcon(light));}});trayThemeTimer.start();
    QTimer nightLightMaintenance;nightLightMaintenance.setInterval(2000);QObject::connect(&nightLightMaintenance,&QTimer::timeout,&controller,&ApplicationController::refreshNightLightState);nightLightMaintenance.start();
    int configuringDepth=0;QObject::connect(&controller,&ApplicationController::operationStarted,&panel,[&]{if(++configuringDepth==1)panel.setConfigurationBusy(true,L("busy.configuring"),250);});QObject::connect(&controller,&ApplicationController::operationFinished,&panel,[&]{configuringDepth=qMax(0,configuringDepth-1);if(configuringDepth==0)panel.setConfigurationBusy(false);});
#ifdef Q_OS_WIN
    DisplayEventFilter displayEvents(&controller);app.installNativeEventFilter(&displayEvents);
#endif
    bool quitting=false;auto requestQuit=[&]{if(quitting)return;quitting=true;
#ifdef Q_OS_WIN
        displayEvents.stop();nightLightMaintenance.stop();
#endif
        welcome.hide();tray.hide();if(!panel.isVisible())panel.showPanel();panel.setConfigurationBusy(true,L("busy.quitting"));app.processEvents(QEventLoop::ExcludeUserInputEvents);std::atomic_bool restored{false};std::thread restorer([&]{controller.shutdown();restored.store(true);});while(!restored.load()){app.processEvents(QEventLoop::ExcludeUserInputEvents,20);std::this_thread::sleep_for(std::chrono::milliseconds(16));}restorer.join();panel.setConfigurationBusy(false);panel.hide();app.quit();};
    auto show=[&]{welcome.hide();if(panel.isVisible())panel.hide();else panel.showPanel();};QObject::connect(open,&QAction::triggered,&panel,[&]{welcome.hide();panel.showPanel();});QObject::connect(&tray,&QSystemTrayIcon::activated,&panel,[&](QSystemTrayIcon::ActivationReason reason){if(reason==QSystemTrayIcon::Trigger)show();});QObject::connect(quit,&QAction::triggered,&app,requestQuit);QObject::connect(&panel,&MainPanel::quitRequested,&app,requestQuit);QObject::connect(&app,&QCoreApplication::aboutToQuit,&controller,&ApplicationController::shutdown);
    QObject::connect(&tray,&QSystemTrayIcon::messageClicked,&panel,[&]{welcome.hide();panel.showPanel();});
    QObject::connect(&controller,&ApplicationController::stateChanged,&tray,[&]{tray.setToolTip(L("app.title"));open->setText(L("app.title"));quit->setText(L("quit"));});
    windows::TrayPromotionWatcher trayPromotionWatcher;
    const bool trayWatcherArmed=showTrayDiscovery&&trayPromotionWatcher.arm(true);
    tray.show();
    std::function<void(int)> showWelcomeAtTray;
    showWelcomeAtTray=[&](int retries){
        const QVector<QRect> anchors=windows::ownPromotedTrayIconRects();
        const QRect qtTrayGeometry=tray.geometry();
        if(anchors.isEmpty()&&!qtTrayGeometry.isValid()&&retries>0){QTimer::singleShot(100,&tray,[&,retries]{showWelcomeAtTray(retries-1);});return;}
        QRect anchor;
        QScreen *preferred=QGuiApplication::screenAt(QCursor::pos());
        if(preferred)for(const QRect &candidate:anchors)if(preferred->geometry().contains(candidate.center())){anchor=candidate;break;}
        if(!anchor.isValid()&&!anchors.isEmpty())anchor=anchors.first();
        if(!anchor.isValid())anchor=qtTrayGeometry;
        welcome.setTrayAnchorRect(anchor);welcome.show();
    };
    QObject::connect(&welcome,&QDialog::finished,&panel,[&](int){panel.showPanel();});
    const bool shouldShowWelcome=!backgroundLaunch&&controller.settings().showWelcome
        &&!arguments.contains(QStringLiteral("--skip-welcome"));
    bool welcomeRequested=false;
    auto showWelcomeOnce=[&]{
        if(!shouldShowWelcome||welcomeRequested)return;
        welcomeRequested=true;showWelcomeAtTray(10);
    };
    std::function<void(int)> promoteThenShow;
    if(showTrayDiscovery) {
        constexpr int kTrayPromotionAttempts=300; // Explorer can publish the per-icon key late.
        bool catalogNotificationSent=false;
        promoteThenShow=[&](int attempt){
            windows::TrayPromotionResult result=windows::promoteOwnTrayIcon();
            if(trayWatcherArmed) {
                const windows::TrayPromotionResult watched=trayPromotionWatcher.result();
                if(watched==windows::TrayPromotionResult::Promoted
                    ||watched==windows::TrayPromotionResult::Overflow)result=watched;
                else if(result==windows::TrayPromotionResult::Overflow&&attempt<kTrayPromotionAttempts)
                    result=windows::TrayPromotionResult::Pending;
            }
            if(result==windows::TrayPromotionResult::Pending&&trayWatcherArmed
                &&!catalogNotificationSent&&attempt>=5) {
                // NIF_INFO forces lazy Windows 11 shells to catalog the icon,
                // giving the still-armed registry watcher an identity to promote.
                catalogNotificationSent=true;
                tray.showMessage(L("welcome.windows.tray.title"),L("welcome.windows.tray"),
                    QSystemTrayIcon::Information,10000);
            }
            if(result==windows::TrayPromotionResult::Pending&&attempt<kTrayPromotionAttempts) {
                QTimer::singleShot(100,&tray,[&,attempt]{promoteThenShow(attempt+1);});
            } else if(result==windows::TrayPromotionResult::Promoted) {
                trayPromotionWatcher.stop();
                QTimer::singleShot(450,&tray,[&]{showWelcomeOnce();controller.setTrayDiscoveryShown(true,currentExecutable);});
            } else if(result==windows::TrayPromotionResult::Overflow) {
                trayPromotionWatcher.stop();
                tray.showMessage(L("welcome.windows.tray.title"),L("welcome.windows.tray"),QSystemTrayIcon::Information,10000);
                QTimer::singleShot(500,&tray,[&]{showWelcomeOnce();});
            } else {trayPromotionWatcher.stop();showWelcomeOnce();}
        };
        if(shouldShowWelcome)QTimer::singleShot(250,&tray,[&]{showWelcomeOnce();});
        else if(!backgroundLaunch)panel.showPanel();
        QTimer::singleShot(200,&tray,[&]{promoteThenShow(0);});
    } else if(!arguments.contains(QStringLiteral("--background"))) {
        if(shouldShowWelcome)QTimer::singleShot(250,&tray,[&]{showWelcomeOnce();});else panel.showPanel();
    }
    const int exitIndex=arguments.indexOf(QStringLiteral("--exit-after-ms"));
    if(exitIndex>=0 && exitIndex+1<arguments.size())QTimer::singleShot(arguments[exitIndex+1].toInt(),&app,&QCoreApplication::quit);
    return app.exec();
}
