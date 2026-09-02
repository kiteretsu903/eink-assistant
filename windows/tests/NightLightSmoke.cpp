#include "platform/windows/WindowsPlatformServices.h"

#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QTextStream>
#include <windows.h>

using namespace eink;

int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);QCoreApplication::setApplicationName(QStringLiteral("E-Ink Assistant Night Light Test"));QCoreApplication::setOrganizationName(QStringLiteral("EinkAssistant"));
    const QStringList arguments=app.arguments();if(arguments.size()<3)return 90;const QString mode=arguments[1],path=arguments[2];
    if(mode==QStringLiteral("normal")) {
        QFile file(path);if(!file.open(QIODevice::WriteOnly|QIODevice::Text))return 91;QTextStream report(&file);WindowsPlatformServices platform;
        const ApplyResult recovered=platform.recoverInterruptedNightLightState();if(!recovered.success){report<<"FAIL recovery: "<<recovered.error<<'\n';return 92;}
        if(!platform.nightLightControlAvailable()){report<<"FAIL unavailable\n";return 93;}const bool original=platform.nightLightEnabled();report<<"OriginalEnabled="<<original<<'\n';
        const ApplyResult disabled=platform.setNightLightDisabled(true);const bool isDisabled=!platform.nightLightEnabled();report<<"DisableSuccess="<<disabled.success<<"\nDisabled="<<isDisabled<<'\n';
        const ApplyResult enabled=platform.setNightLightDisabled(false);const bool isEnabled=platform.nightLightEnabled();report<<"EnableSuccess="<<enabled.success<<"\nEnabled="<<isEnabled<<'\n';
        const ApplyResult restored=platform.restoreNightLightState();const bool isRestored=platform.nightLightEnabled()==original;report<<"RestoreSuccess="<<restored.success<<"\nRestored="<<isRestored<<'\n';
        const bool pass=disabled.success&&isDisabled&&enabled.success&&isEnabled&&restored.success&&isRestored;report<<(pass?"PASS\n":"FAIL\n");platform.shutdown();return pass?0:94;
    }
    if(mode==QStringLiteral("crash")) {
        WindowsPlatformServices platform;const ApplyResult recovered=platform.recoverInterruptedNightLightState();if(!recovered.success||!platform.nightLightControlAvailable())return 95;
        QSettings state(path,QSettings::IniFormat);state.clear();state.setValue(QStringLiteral("originalEnabled"),platform.nightLightEnabled());const ApplyResult disabled=platform.setNightLightDisabled(true);state.setValue(QStringLiteral("disableSuccess"),disabled.success);state.setValue(QStringLiteral("disabled"),!platform.nightLightEnabled());state.sync();if(!disabled.success)return 96;TerminateProcess(GetCurrentProcess(),97);return 97;
    }
    if(mode==QStringLiteral("recover")) {
        QSettings state(path,QSettings::IniFormat);if(!state.value(QStringLiteral("disableSuccess")).toBool())return 98;const bool original=state.value(QStringLiteral("originalEnabled")).toBool();
        WindowsPlatformServices platform;const ApplyResult recovered=platform.recoverInterruptedNightLightState();const bool pass=recovered.success&&platform.nightLightEnabled()==original;state.setValue(QStringLiteral("recoverySuccess"),recovered.success);state.setValue(QStringLiteral("restored"),pass);state.sync();platform.shutdown();return pass?0:99;
    }
    return 100;
}
