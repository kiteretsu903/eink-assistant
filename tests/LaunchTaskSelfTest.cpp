#include "platform/windows/WindowsPlatformServices.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>

using namespace eink;

int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    QFile report(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("launch-task-self-test.txt")));
    if(!report.open(QIODevice::WriteOnly|QIODevice::Text))return 2;
    QTextStream out(&report);
    WindowsPlatformServices platform;
    const bool original=platform.launchAtLogin();
    const ApplyResult enabled=platform.setLaunchAtLogin(true);
    if(!enabled.success){out<<"FAIL enable: "<<enabled.error<<'\n';return 3;}
    const QString xml=platform.registeredLaunchTaskXmlForDiagnostics();
    const bool valid=platform.launchAtLogin()
        &&xml.contains(QStringLiteral("<LogonTrigger>"))
        &&xml.contains(QStringLiteral("<LogonType>InteractiveToken</LogonType>"))
        &&xml.contains(QStringLiteral("<RunLevel>HighestAvailable</RunLevel>"))
        &&xml.contains(QStringLiteral("<DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>"))
        &&xml.contains(QStringLiteral("<StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>"))
        &&xml.contains(QStringLiteral("<Arguments>--background</Arguments>"));
    out<<"OriginalEnabled="<<original<<'\n'<<"Registered="<<platform.launchAtLogin()<<'\n'
       <<"LogonTrigger="<<xml.contains(QStringLiteral("<LogonTrigger>"))<<'\n'
       <<"HighestRunLevel="<<xml.contains(QStringLiteral("<RunLevel>HighestAvailable</RunLevel>"))<<'\n'
       <<"AllowStartOnBattery="<<xml.contains(QStringLiteral("<DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>"))<<'\n'
       <<"ContinueOnBattery="<<xml.contains(QStringLiteral("<StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>"))<<'\n';
    if(!original) {
        const ApplyResult restored=platform.setLaunchAtLogin(false);
        if(!restored.success){out<<"FAIL restore: "<<restored.error<<'\n';return 4;}
    }
    out<<(valid?"PASS\n":"FAIL task definition\n");
    platform.shutdown();
    return valid?0:5;
}
