#include "WindowsPlatformServices.h"
#include "WindowsNative.h"
#include "WindowsCompatibility.h"
#include "core/IccProfile.h"
#include "core/NightLightStateCodec.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QSettings>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QtEndian>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <thread>
#include <icm.h>
#include <sddl.h>
#include <shellapi.h>
#include <taskschd.h>
#include <winternl.h>

namespace eink {
namespace {

struct GammaRamp { WORD channel[3][256]; };

template<typename T> T mscmsProc(const char *name) {
    HMODULE module = LoadLibraryW(L"mscms.dll");
    return module ? reinterpret_cast<T>(GetProcAddress(module, name)) : nullptr;
}

QString luidText(const LUID &luid) {
    return QStringLiteral("%1:%2").arg(luid.HighPart).arg(luid.LowPart);
}

bool parseBoolRegistry(HKEY root, const wchar_t *path, const wchar_t *name, DWORD *value) {
    DWORD size=sizeof(DWORD), type=0;
    return RegGetValueW(root,path,name,RRF_RT_REG_DWORD,&type,value,&size)==ERROR_SUCCESS;
}

struct ChildProcessResult {
    bool launched = false;
    bool timedOut = false;
    DWORD error = ERROR_SUCCESS;
    DWORD exitCode = static_cast<DWORD>(-1);
};

bool currentProcessElevated() {
    HANDLE token=nullptr;if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return false;
    TOKEN_ELEVATION elevation{};DWORD bytes=0;
    const bool elevated=GetTokenInformation(token,TokenElevation,&elevation,sizeof(elevation),&bytes)&&elevation.TokenIsElevated;
    CloseHandle(token);return elevated;
}

QString quoteCommandLineArgument(const QString &argument) {
    QString quoted=argument;quoted.replace(QLatin1Char('"'),QStringLiteral("\\\""));
    return QLatin1Char('"')+quoted+QLatin1Char('"');
}

ChildProcessResult runNightLightHelperProcess(const QString &helper,const QStringList &arguments) {
    ChildProcessResult result;
    QString command=quoteCommandLineArgument(helper);
    for(const QString &argument:arguments)command+=QLatin1Char(' ')+quoteCommandLineArgument(argument);
    QVector<wchar_t> mutableCommand(command.size()+1);command.toWCharArray(mutableCommand.data());mutableCommand[command.size()]=0;
    const QString workingDirectory=QFileInfo(helper).absolutePath();
    STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;
    PROCESS_INFORMATION process{};BOOL launched=FALSE;

    if(currentProcessElevated()) {
        DWORD shellPid=0;const HWND shell=GetShellWindow();if(shell)GetWindowThreadProcessId(shell,&shellPid);
        HANDLE shellProcess=shellPid?OpenProcess(PROCESS_CREATE_PROCESS,FALSE,shellPid):nullptr;
        if(shellProcess) {
            SIZE_T attributeBytes=0;
            InitializeProcThreadAttributeList(nullptr,1,0,&attributeBytes);
            QByteArray attributeStorage(static_cast<int>(attributeBytes),Qt::Uninitialized);
            auto *attributes=reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(attributeStorage.data());
            STARTUPINFOEXW extended{};extended.StartupInfo=startup;extended.StartupInfo.cb=sizeof(extended);
            if(InitializeProcThreadAttributeList(attributes,1,0,&attributeBytes)) {
                extended.lpAttributeList=attributes;
                if(UpdateProcThreadAttribute(attributes,0,PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
                    &shellProcess,sizeof(shellProcess),nullptr,nullptr)) {
                    launched=CreateProcessW(reinterpret_cast<LPCWSTR>(helper.utf16()),mutableCommand.data(),
                        nullptr,nullptr,FALSE,CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,
                        reinterpret_cast<LPCWSTR>(workingDirectory.utf16()),&extended.StartupInfo,&process);
                }
                if(!launched)result.error=GetLastError();
                DeleteProcThreadAttributeList(attributes);
            } else result.error=GetLastError();
        }
        else result.error=GetLastError();
        if(shellProcess)CloseHandle(shellProcess);
    } else {
        launched=CreateProcessW(reinterpret_cast<LPCWSTR>(helper.utf16()),mutableCommand.data(),nullptr,nullptr,FALSE,
            CREATE_NO_WINDOW,nullptr,reinterpret_cast<LPCWSTR>(workingDirectory.utf16()),&startup,&process);
        if(!launched)result.error=GetLastError();
    }

    if(!launched)return result;
    result.launched=true;
    const DWORD wait=WaitForSingleObject(process.hProcess,15000);
    if(wait==WAIT_TIMEOUT){result.timedOut=true;TerminateProcess(process.hProcess,69);WaitForSingleObject(process.hProcess,1000);}
    else if(wait!=WAIT_OBJECT_0)result.error=GetLastError();
    GetExitCodeProcess(process.hProcess,&result.exitCode);
    CloseHandle(process.hThread);CloseHandle(process.hProcess);return result;
}

constexpr wchar_t kLaunchTaskName[] = L"E-Ink Assistant";
constexpr wchar_t kLegacyRunPath[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr DWORD kDirectNightLightMinimumBuild = 15063;
constexpr wchar_t kNightLightStatePath[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\DefaultAccount\\Current\\default$windows.data.bluelightreduction.bluelightreductionstate\\windows.data.bluelightreduction.bluelightreductionstate";
constexpr wchar_t kNightLightStateValue[] = L"Data";
const QByteArray kNightLightRecoveryMagicV1("EINKNL1\n",8);
const QByteArray kNightLightRecoveryMagicV2("EINKNL2\n",8);

void updateNightLightTimestamps(NightLightStateRecord *record) {
    record->cloudTimestamp=static_cast<quint64>(QDateTime::currentSecsSinceEpoch());
    record->transitionFileTime=static_cast<quint64>(QDateTime::currentMSecsSinceEpoch())*10000ULL+116444736000000000ULL;
}

template<typename T>
class ComObject {
public:
    ComObject() = default;
    ~ComObject() { if (m_value) m_value->Release(); }
    ComObject(const ComObject &) = delete;
    ComObject &operator=(const ComObject &) = delete;
    T *get() const { return m_value; }
    T **put() { return &m_value; }
private:
    T *m_value = nullptr;
};

class ComApartment {
public:
    ComApartment() {
        const HRESULT result=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
        m_ready=SUCCEEDED(result)||result==RPC_E_CHANGED_MODE;
        m_uninitialize=SUCCEEDED(result);
        m_result=result;
    }
    ~ComApartment() { if(m_uninitialize)CoUninitialize(); }
    bool ready() const { return m_ready; }
    HRESULT result() const { return m_result; }
private:
    bool m_ready=false;
    bool m_uninitialize=false;
    HRESULT m_result=E_FAIL;
};

class ScopedBstr {
public:
    explicit ScopedBstr(const QString &value):m_value(SysAllocStringLen(
        reinterpret_cast<const OLECHAR*>(value.utf16()),static_cast<UINT>(value.size()))) {}
    ~ScopedBstr() { SysFreeString(m_value); }
    BSTR get() const { return m_value; }
private:
    BSTR m_value=nullptr;
};

HRESULT openTaskFolder(ITaskFolder **folder) {
    ComObject<ITaskService> service;
    HRESULT result=CoCreateInstance(CLSID_TaskScheduler,nullptr,CLSCTX_INPROC_SERVER,
                                    IID_ITaskService,reinterpret_cast<void**>(service.put()));
    if(FAILED(result))return result;
    VARIANT empty;VariantInit(&empty);
    result=service.get()->Connect(empty,empty,empty,empty);
    if(FAILED(result))return result;
    ScopedBstr root(QStringLiteral("\\"));
    return service.get()->GetFolder(root.get(),folder);
}

QString currentUserSid() {
    HANDLE token=nullptr;
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return {};
    DWORD bytes=0;
    GetTokenInformation(token,TokenUser,nullptr,0,&bytes);
    QByteArray buffer(static_cast<int>(bytes),Qt::Uninitialized);
    if(!bytes||!GetTokenInformation(token,TokenUser,buffer.data(),bytes,&bytes)) {
        CloseHandle(token);
        return {};
    }
    CloseHandle(token);
    LPWSTR sidText=nullptr;
    const auto *user=reinterpret_cast<const TOKEN_USER*>(buffer.constData());
    if(!ConvertSidToStringSidW(user->User.Sid,&sidText))return {};
    const QString result=QString::fromWCharArray(sidText);
    LocalFree(sidText);
    return result;
}

bool legacyRunEnabled() {
    HKEY key=nullptr;
    if(RegOpenKeyExW(HKEY_CURRENT_USER,kLegacyRunPath,0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS)return false;
    const LONG result=RegQueryValueExW(key,kLaunchTaskName,nullptr,nullptr,nullptr,nullptr);
    RegCloseKey(key);
    return result==ERROR_SUCCESS;
}

LONG removeLegacyRunEntry() {
    HKEY key=nullptr;
    const LONG opened=RegOpenKeyExW(HKEY_CURRENT_USER,kLegacyRunPath,0,KEY_SET_VALUE,&key);
    if(opened==ERROR_FILE_NOT_FOUND)return ERROR_SUCCESS;
    if(opened!=ERROR_SUCCESS)return opened;
    LONG result=RegDeleteValueW(key,kLaunchTaskName);
    RegCloseKey(key);
    if(result==ERROR_FILE_NOT_FOUND)result=ERROR_SUCCESS;
    return result;
}

QString launchTaskXml(const QString &userSid) {
    const QString command=QCoreApplication::applicationFilePath().toHtmlEscaped();
    const QString working=QFileInfo(QCoreApplication::applicationFilePath()).absolutePath().toHtmlEscaped();
    const QString sid=userSid.toHtmlEscaped();
    return QStringLiteral(
        "<Task version=\"1.2\" xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">"
        "<RegistrationInfo><Description>Starts E-Ink Assistant when this user logs in.</Description></RegistrationInfo>"
        "<Triggers><LogonTrigger><Enabled>true</Enabled><UserId>%1</UserId></LogonTrigger></Triggers>"
        "<Principals><Principal id=\"Author\"><UserId>%1</UserId><LogonType>InteractiveToken</LogonType>"
        "<RunLevel>HighestAvailable</RunLevel></Principal></Principals>"
        "<Settings><MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>"
        "<DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>"
        "<StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>"
        "<AllowHardTerminate>true</AllowHardTerminate><StartWhenAvailable>true</StartWhenAvailable>"
        "<RunOnlyIfNetworkAvailable>false</RunOnlyIfNetworkAvailable>"
        "<IdleSettings><StopOnIdleEnd>false</StopOnIdleEnd><RestartOnIdle>false</RestartOnIdle></IdleSettings>"
        "<AllowStartOnDemand>true</AllowStartOnDemand><Enabled>true</Enabled><Hidden>false</Hidden>"
        "<RunOnlyIfIdle>false</RunOnlyIfIdle><WakeToRun>false</WakeToRun>"
        "<ExecutionTimeLimit>PT0S</ExecutionTimeLimit><Priority>7</Priority></Settings>"
        "<Actions Context=\"Author\"><Exec><Command>%2</Command><Arguments>--background</Arguments>"
        "<WorkingDirectory>%3</WorkingDirectory></Exec></Actions></Task>").arg(sid,command,working);
}

QString monitorModelName(const QString &deviceId) {
    QString normalized=deviceId; normalized.replace(QLatin1Char('\\'),QLatin1Char('#'));
    const QStringList parts=normalized.split(QLatin1Char('#'),Qt::SkipEmptyParts);
    for(int index=0;index+1<parts.size();++index) {
        if(parts[index].compare(QStringLiteral("DISPLAY"),Qt::CaseInsensitive)!=0
            && parts[index].compare(QStringLiteral("MONITOR"),Qt::CaseInsensitive)!=0)continue;
        const QString code=parts[index+1].trimmed().toUpper();
        if(code.size()>3)return code.left(3)+QLatin1Char(' ')+code.mid(3);
        return code;
    }
    return {};
}

} // namespace

WindowsPlatformServices::WindowsPlatformServices()
    : m_profileTemp(new QTemporaryDir(QDir::tempPath()+QStringLiteral("/EinkAssistant-XXXXXX"))),
      m_profileSessionId(QString::number(QRandomGenerator::global()->generate64(),16)) {
    const bool scheduled=launchAtLogin();
    if(legacyRunEnabled()&&!scheduled)setLaunchAtLogin(true);
    else if(scheduled&&QFileInfo(QCoreApplication::applicationFilePath()).fileName().compare(QStringLiteral("EinkAssistant.exe"),Qt::CaseInsensitive)==0) {
        QString xml=registeredLaunchTaskXmlForDiagnostics();xml.replace(QLatin1Char('\\'),QLatin1Char('/'));
        QString executable=QDir::cleanPath(QCoreApplication::applicationFilePath());executable.replace(QLatin1Char('\\'),QLatin1Char('/'));
        if(!xml.contains(executable,Qt::CaseInsensitive))setLaunchAtLogin(true);
    }
}

WindowsPlatformServices::~WindowsPlatformServices() { shutdown(); }

quint32 WindowsPlatformServices::windowsBuild() {
    using RtlGetVersionFn=LONG (WINAPI*)(PRTL_OSVERSIONINFOW);
    const HMODULE ntdll=GetModuleHandleW(L"ntdll.dll");
    const auto fn=reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll,"RtlGetVersion"));
    RTL_OSVERSIONINFOW v{}; v.dwOSVersionInfoSize=sizeof(v);
    return fn && fn(&v)==0 ? v.dwBuildNumber : 0;
}

QString WindowsPlatformServices::errorMessage(const QString &operation, DWORD code) {
    wchar_t *buffer=nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr,code,0,reinterpret_cast<wchar_t*>(&buffer),0,nullptr);
    const QString detail=buffer?QString::fromWCharArray(buffer).trimmed():QStringLiteral("error %1").arg(code);
    if(buffer) LocalFree(buffer);
    return operation+QStringLiteral(": ")+detail;
}

bool WindowsPlatformServices::queryDisplayPath(const QString &deviceName, LUID *adapter,
                                                UINT32 *sourceId, UINT32 *targetId, bool *builtIn,
                                                QString *friendlyName) {
    UINT32 pathCount=0,modeCount=0;
    LONG result=GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&pathCount,&modeCount);
    if(result!=ERROR_SUCCESS) return false;
    QVector<DISPLAYCONFIG_PATH_INFO> paths(static_cast<int>(pathCount));
    QVector<DISPLAYCONFIG_MODE_INFO> modes(static_cast<int>(modeCount));
    result=QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&pathCount,paths.data(),&modeCount,modes.data(),nullptr);
    if(result!=ERROR_SUCCESS) return false;
    for(UINT32 i=0;i<pathCount;++i) {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};
        source.header.type=DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        source.header.size=sizeof(source);
        source.header.adapterId=paths[static_cast<int>(i)].sourceInfo.adapterId;
        source.header.id=paths[static_cast<int>(i)].sourceInfo.id;
        if(DisplayConfigGetDeviceInfo(&source.header)!=ERROR_SUCCESS) continue;
        if(QString::fromWCharArray(source.viewGdiDeviceName).compare(deviceName,Qt::CaseInsensitive)!=0) continue;
        if(adapter) *adapter=paths[static_cast<int>(i)].targetInfo.adapterId;
        if(sourceId) *sourceId=paths[static_cast<int>(i)].sourceInfo.id;
        if(targetId) *targetId=paths[static_cast<int>(i)].targetInfo.id;
        if(builtIn) {
            const auto tech=paths[static_cast<int>(i)].targetInfo.outputTechnology;
            *builtIn=tech==DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL
                || tech==DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED
                || tech==DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED;
        }
        if(friendlyName) {
            DISPLAYCONFIG_TARGET_DEVICE_NAME target{};
            target.header.type=DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
            target.header.size=sizeof(target);
            target.header.adapterId=paths[static_cast<int>(i)].targetInfo.adapterId;
            target.header.id=paths[static_cast<int>(i)].targetInfo.id;
            if(DisplayConfigGetDeviceInfo(&target.header)==ERROR_SUCCESS)
                *friendlyName=QString::fromWCharArray(target.monitorFriendlyDeviceName).trimmed();
        }
        return true;
    }
    return false;
}

QVector<DisplayInfo> WindowsPlatformServices::displays() {
    QVector<DisplayInfo> result;
    DISPLAY_DEVICEW adapterDevice{}; adapterDevice.cb=sizeof(adapterDevice);
    for(DWORD index=0;EnumDisplayDevicesW(nullptr,index,&adapterDevice,0);++index) {
        if(!(adapterDevice.StateFlags&DISPLAY_DEVICE_ACTIVE) || (adapterDevice.StateFlags&DISPLAY_DEVICE_MIRRORING_DRIVER)) {
            adapterDevice={}; adapterDevice.cb=sizeof(adapterDevice); continue;
        }
        DISPLAY_DEVICEW monitor{}; monitor.cb=sizeof(monitor);
        EnumDisplayDevicesW(adapterDevice.DeviceName,0,&monitor,EDD_GET_DEVICE_INTERFACE_NAME);
        DisplayInfo info;
        info.deviceName=QString::fromWCharArray(adapterDevice.DeviceName);
        info.friendlyName=QString::fromWCharArray(monitor.DeviceString);
        if(info.friendlyName.trimmed().isEmpty()) info.friendlyName=QString::fromWCharArray(adapterDevice.DeviceString);
        info.stableId=QString::fromWCharArray(monitor.DeviceID);
        if(info.stableId.isEmpty()) info.stableId=info.deviceName;
        LUID luid{}; UINT32 source=0,target=0; bool internal=false; QString targetFriendlyName;
        if(queryDisplayPath(info.deviceName,&luid,&source,&target,&internal,&targetFriendlyName)) {
            info.adapterHigh=luid.HighPart; info.adapterLow=luid.LowPart; info.sourceId=source; info.targetId=target; info.builtIn=internal;
            if(!targetFriendlyName.isEmpty())info.friendlyName=targetFriendlyName;
            else {
                const QString modelName=monitorModelName(info.stableId);
                if(!modelName.isEmpty())info.friendlyName=modelName;
            }
            bool supported=false,enabled=false;
            if(queryAcm(info,&supported,&enabled)) { info.acmSupported=supported; info.acmEnabled=enabled; }
            bool mhc2Supported=false,mhc2Verified=false;
            queryWindows10Mhc2(info,&mhc2Supported,&mhc2Verified);
            const windows::ColorPipeline pipeline=windows::chooseColorPipeline(
                windowsBuild(),modernColorProfileApisAvailable(),
                mscmsProc<ColorProfileGetDeviceCapabilitiesFn>("ColorProfileGetDeviceCapabilities")!=nullptr,
                mhc2Supported,info.acmSupported);
            info.colorAdjustmentSupported=pipeline!=windows::ColorPipeline::Unavailable;
            info.usesWindows10Mhc2=pipeline==windows::ColorPipeline::Windows10Mhc2;
        }
        info.ditheringControlSupported=false;
        result.push_back(info);
        adapterDevice={}; adapterDevice.cb=sizeof(adapterDevice);
    }
    return result;
}

bool WindowsPlatformServices::queryAcm(const DisplayInfo &d, bool *supported, bool *enabled, QString *error) {
    if(windowsBuild()<26100) { if(supported)*supported=false; if(enabled)*enabled=false; return true; }
    DisplayConfigAdvancedColorInfo2 packet{};
    packet.header.type=static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(DisplayConfigGetAdvancedColorInfo2);
    packet.header.size=sizeof(packet); packet.header.adapterId.HighPart=d.adapterHigh;
    packet.header.adapterId.LowPart=d.adapterLow; packet.header.id=d.targetId;
    const LONG result=DisplayConfigGetDeviceInfo(&packet.header);
    if(result!=ERROR_SUCCESS) { if(error)*error=errorMessage(QStringLiteral("Query Auto Color Management"),result); return false; }
    if(supported)*supported=packet.wideColorSupported(); if(enabled)*enabled=packet.wideColorEnabled(); return true;
}

bool WindowsPlatformServices::modernColorProfileApisAvailable() {
    return mscmsProc<ColorProfileAddDisplayAssociationFn>("ColorProfileAddDisplayAssociation")
        &&mscmsProc<ColorProfileRemoveDisplayAssociationFn>("ColorProfileRemoveDisplayAssociation")
        &&mscmsProc<ColorProfileSetDisplayDefaultAssociationFn>("ColorProfileSetDisplayDefaultAssociation")
        &&mscmsProc<ColorProfileGetDisplayDefaultFn>("ColorProfileGetDisplayDefault")
        &&mscmsProc<ColorProfileGetDisplayUserScopeFn>("ColorProfileGetDisplayUserScope")
        &&mscmsProc<ColorProfileGetDisplayListFn>("ColorProfileGetDisplayList");
}

bool WindowsPlatformServices::queryWindows10Mhc2(const DisplayInfo &d, bool *supported, bool *verified, QString *error) {
    if(supported)*supported=false;
    if(verified)*verified=false;
    const quint32 build=windowsBuild();
    if(build<19041||build>=22000)return true;
    if(!modernColorProfileApisAvailable()) {
        if(error)*error=QStringLiteral("The Windows 10 MHC2 compatibility path requires modern display profile APIs that are missing from mscms.dll.");
        return false;
    }
    const auto capability=mscmsProc<ColorProfileGetDeviceCapabilitiesFn>("ColorProfileGetDeviceCapabilities");
    if(!capability) {
        if(supported)*supported=true;
        return true;
    }
    LUID luid{};
    luid.HighPart=d.adapterHigh;
    luid.LowPart=d.adapterLow;
    WcsDeviceMhc2Capabilities caps{};
    caps.size=sizeof(caps);
    const HRESULT hr=capability(profileScope(d),luid,d.sourceId,
        static_cast<INT>(WcsDeviceCapabilitiesType::MicrosoftHardwareColorV2),&caps);
    if(FAILED(hr)) {
        if(error)*error=QStringLiteral("Query Windows 10 MHC2 capability failed (0x%1).")
            .arg(static_cast<quint32>(hr),8,16,QLatin1Char('0'));
        return false;
    }
    if(supported)*supported=caps.supportsMhc2!=FALSE;
    if(verified)*verified=true;
    return true;
}

ApplyResult WindowsPlatformServices::setAcm(const DisplayInfo &d, bool enabled) {
    if(windowsBuild()<26100) return ApplyResult::fail(QStringLiteral("Auto Color Management requires Windows 11 24H2 or above."));
    DisplayConfigSetWcg packet{};
    packet.header.type=static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(DisplayConfigSetWcgState);
    packet.header.size=sizeof(packet); packet.header.adapterId.HighPart=d.adapterHigh;
    packet.header.adapterId.LowPart=d.adapterLow; packet.header.id=d.targetId; packet.value=enabled?1u:0u;
    const LONG result=DisplayConfigSetDeviceInfo(&packet.header);
    return result==ERROR_SUCCESS?ApplyResult::ok():ApplyResult::fail(errorMessage(QStringLiteral("Change Auto Color Management"),result));
}

ApplyResult WindowsPlatformServices::applyToneCurve(const DisplayInfo &d, const ToneCurve &curve) {
    HDC dc=CreateDCW(L"DISPLAY",reinterpret_cast<LPCWSTR>(d.deviceName.utf16()),nullptr,nullptr);
    if(!dc) return ApplyResult::fail(errorMessage(QStringLiteral("Open %1").arg(d.friendlyName)));
    if(!m_originalGamma.contains(d.stableId)) {
        GammaRamp original{};
        if(GetDeviceGammaRamp(dc,&original)) m_originalGamma.insert(d.stableId,QByteArray(reinterpret_cast<char*>(&original),sizeof(original)));
    }
    GammaRamp ramp{}; const QVector<quint16> table=curve.table(256);
    for(int c=0;c<3;++c) for(int i=0;i<256;++i) ramp.channel[c][i]=table[i];
    const BOOL ok=SetDeviceGammaRamp(dc,&ramp); const DWORD code=ok?ERROR_SUCCESS:GetLastError(); DeleteDC(dc);
    return ok?ApplyResult::ok():ApplyResult::fail(errorMessage(QStringLiteral("Apply tone curve to %1").arg(d.friendlyName),code));
}

ApplyResult WindowsPlatformServices::restoreToneCurve(const DisplayInfo &d) {
    if(!m_originalGamma.contains(d.stableId)) return ApplyResult::ok();
    HDC dc=CreateDCW(L"DISPLAY",reinterpret_cast<LPCWSTR>(d.deviceName.utf16()),nullptr,nullptr);
    if(!dc) return ApplyResult::fail(errorMessage(QStringLiteral("Open %1").arg(d.friendlyName)));
    QByteArray original=m_originalGamma.take(d.stableId);
    const BOOL ok=SetDeviceGammaRamp(dc,original.data()); const DWORD code=ok?ERROR_SUCCESS:GetLastError(); DeleteDC(dc);
    return ok?ApplyResult::ok():ApplyResult::fail(errorMessage(QStringLiteral("Restore tone curve on %1").arg(d.friendlyName),code));
}

QString WindowsPlatformServices::colorDirectory() {
    DWORD size=0; GetColorDirectoryW(nullptr,nullptr,&size); if(!size) return {};
    QVector<wchar_t> buffer(static_cast<int>(size)+1);
    return GetColorDirectoryW(nullptr,buffer.data(),&size)?QString::fromWCharArray(buffer.data()):QString{};
}

QString WindowsPlatformServices::recoveryFilePath() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
        .filePath(QStringLiteral("color-recovery.ini"));
}

QString WindowsPlatformServices::nightLightRecoveryFilePath() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
        .filePath(QStringLiteral("night-light-recovery.bin"));
}

ApplyResult WindowsPlatformServices::readNightLightState(QByteArray *data) {
    if(!data)return ApplyResult::fail(QStringLiteral("Night Light state output is missing."));
    HKEY key=nullptr;
    LONG code=RegOpenKeyExW(HKEY_CURRENT_USER,kNightLightStatePath,0,KEY_QUERY_VALUE,&key);
    if(code!=ERROR_SUCCESS)return ApplyResult::fail(errorMessage(QStringLiteral("Open Night Light state"),static_cast<DWORD>(code)));
    DWORD type=0,size=0;
    code=RegQueryValueExW(key,kNightLightStateValue,nullptr,&type,nullptr,&size);
    if(code==ERROR_SUCCESS&&type!=REG_BINARY)code=ERROR_DATATYPE_MISMATCH;
    if(code==ERROR_SUCCESS&&(size<16||size>8192))code=ERROR_INVALID_DATA;
    QByteArray value(static_cast<int>(size),Qt::Uninitialized);
    if(code==ERROR_SUCCESS)code=RegQueryValueExW(key,kNightLightStateValue,nullptr,&type,reinterpret_cast<BYTE*>(value.data()),&size);
    RegCloseKey(key);
    if(code!=ERROR_SUCCESS)return ApplyResult::fail(errorMessage(QStringLiteral("Read Night Light state"),static_cast<DWORD>(code)));
    value.resize(static_cast<int>(size));*data=value;return ApplyResult::ok();
}

ApplyResult WindowsPlatformServices::writeNightLightState(const QByteArray &data) {
    HKEY key=nullptr;
    LONG code=RegOpenKeyExW(HKEY_CURRENT_USER,kNightLightStatePath,0,KEY_SET_VALUE,&key);
    if(code!=ERROR_SUCCESS)return ApplyResult::fail(errorMessage(QStringLiteral("Open Night Light state for writing"),static_cast<DWORD>(code)));
    code=RegSetValueExW(key,kNightLightStateValue,0,REG_BINARY,reinterpret_cast<const BYTE*>(data.constData()),static_cast<DWORD>(data.size()));
    RegCloseKey(key);
    return code==ERROR_SUCCESS?ApplyResult::ok():ApplyResult::fail(errorMessage(QStringLiteral("Write Night Light state"),static_cast<DWORD>(code)));
}

ApplyResult WindowsPlatformServices::queryNativeNightLight(bool *enabled, bool *openedSettings) {
    if(!enabled)return ApplyResult::fail(QStringLiteral("Night Light query output is missing."));
    const QString helper=QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("EinkNightLightControl.exe"));
    if(!QFileInfo::exists(helper))return ApplyResult::fail(QStringLiteral("The Windows Night Light control helper is missing."));
    const ChildProcessResult child=runNightLightHelperProcess(helper,{QStringLiteral("query"),QStringLiteral("--quiet")});
    if(!child.launched)return ApplyResult::fail(errorMessage(QStringLiteral("Start the Windows Night Light control helper"),child.error));
    if(child.timedOut)return ApplyResult::fail(QStringLiteral("Windows Night Light did not expose its native switch in time."));
    if(child.exitCode<10||child.exitCode>13)
        return ApplyResult::fail(QStringLiteral("Windows Night Light native-state query failed (code %1).").arg(child.exitCode));
    *enabled=child.exitCode>=12;
    if(openedSettings)*openedSettings=(child.exitCode%2)==1;
    return ApplyResult::ok();
}

ApplyResult WindowsPlatformServices::setNativeNightLight(bool enabled, bool closeSettings) {
    const QString helper=QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("EinkNightLightControl.exe"));
    if(!QFileInfo::exists(helper))return ApplyResult::fail(QStringLiteral("The Windows Night Light control helper is missing."));
    QStringList arguments{enabled?QStringLiteral("on"):QStringLiteral("off"),QStringLiteral("--quiet")};
    if(closeSettings)arguments<<QStringLiteral("--owned-settings")<<QStringLiteral("--close");
    const ChildProcessResult child=runNightLightHelperProcess(helper,arguments);
    if(!child.launched)return ApplyResult::fail(errorMessage(QStringLiteral("Start the Windows Night Light control helper"),child.error));
    if(child.timedOut)return ApplyResult::fail(QStringLiteral("Windows Night Light did not apply the requested state in time."));
    return child.exitCode==0?ApplyResult::ok()
        :ApplyResult::fail(QStringLiteral("Windows Night Light native control failed (code %1).").arg(child.exitCode));
}

bool WindowsPlatformServices::readNightLightRecovery(QByteArray *data, bool *enabled) {
    QFile file(nightLightRecoveryFilePath());if(!file.open(QIODevice::ReadOnly))return false;
    const QByteArray contents=file.readAll();
    bool originalEnabled=false;QByteArray original;
    if(contents.startsWith(kNightLightRecoveryMagicV2)&&contents.size()>kNightLightRecoveryMagicV2.size()) {
        const char state=contents.at(kNightLightRecoveryMagicV2.size());if(state!=0&&state!=1)return false;
        originalEnabled=state==1;original=contents.mid(kNightLightRecoveryMagicV2.size()+1);
    } else if(contents.startsWith(kNightLightRecoveryMagicV1)) {
        original=contents.mid(kNightLightRecoveryMagicV1.size());
    } else return false;
    NightLightStateRecord record;QString error;
    if(!NightLightStateCodec::decode(original,&record,&error))return false;
    if(contents.startsWith(kNightLightRecoveryMagicV1))originalEnabled=record.enabled;
    if(data)*data=original;if(enabled)*enabled=originalEnabled;return true;
}

ApplyResult WindowsPlatformServices::writeNightLightRecovery(const QByteArray &data, bool enabled) {
    const QString path=nightLightRecoveryFilePath();QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly))return ApplyResult::fail(QStringLiteral("Could not open the Night Light crash-recovery journal."));
    const char state=enabled?1:0;
    if(file.write(kNightLightRecoveryMagicV2)!=kNightLightRecoveryMagicV2.size()||file.write(&state,1)!=1
        ||file.write(data)!=data.size()||!file.commit())
        return ApplyResult::fail(QStringLiteral("Could not save the Night Light crash-recovery journal."));
    return ApplyResult::ok();
}

void WindowsPlatformServices::clearNightLightRecovery() { QFile::remove(nightLightRecoveryFilePath()); }

QString WindowsPlatformServices::recoveryGroup(const DisplayInfo &d) {
    return QString::fromLatin1(QCryptographicHash::hash(d.stableId.toUtf8(),QCryptographicHash::Sha256).toHex());
}

QString WindowsPlatformServices::profilePrefix(const DisplayInfo &d) {
    const QString id=QString::fromLatin1(QCryptographicHash::hash(d.stableId.toUtf8(),QCryptographicHash::Sha1).toHex().left(10));
    return QStringLiteral("EinkAssistant-%1-").arg(id);
}

bool WindowsPlatformServices::isGeneratedProfile(const QString &profile) {
    return profile.startsWith(QStringLiteral("EinkAssistant-"),Qt::CaseInsensitive)
        && profile.endsWith(QStringLiteral(".icm"),Qt::CaseInsensitive);
}

QStringList WindowsPlatformServices::associatedProfiles(const DisplayInfo &d, int scope, QString *error) {
    const auto fn=mscmsProc<ColorProfileGetDisplayListFn>("ColorProfileGetDisplayList");
    if(!fn) { if(error)*error=QStringLiteral("Modern Windows color profile list API is unavailable."); return {}; }
    LUID luid{}; luid.HighPart=d.adapterHigh; luid.LowPart=d.adapterLow;
    PWSTR *names=nullptr; DWORD count=0;
    const HRESULT hr=fn(scope,luid,d.sourceId,&names,&count);
    if(FAILED(hr)) {
        if(error)*error=QStringLiteral("List color profile associations failed (0x%1).").arg(static_cast<quint32>(hr),8,16,QLatin1Char('0'));
        return {};
    }
    QStringList result;
    for(DWORD index=0;index<count;++index) if(names[index]) result.push_back(QString::fromWCharArray(names[index]));
    if(names) LocalFree(names);
    return result;
}

ApplyResult WindowsPlatformServices::writeRecoveryRecord(const DisplayInfo &d, const ColorState &state) {
    const QString path=recoveryFilePath(); QDir().mkpath(QFileInfo(path).absolutePath());
    QSettings settings(path,QSettings::IniFormat); settings.beginGroup(recoveryGroup(d));
    settings.setValue(QStringLiteral("stableId"),d.stableId);
    settings.setValue(QStringLiteral("previousProfile"),state.previousProfile);
    settings.setValue(QStringLiteral("acmOriginallyEnabled"),state.acmOriginallyEnabled);
    settings.setValue(QStringLiteral("scope"),state.scope);
    settings.endGroup(); settings.sync();
    return settings.status()==QSettings::NoError?ApplyResult::ok()
        :ApplyResult::fail(QStringLiteral("Could not save the color crash-recovery record."));
}

bool WindowsPlatformServices::readRecoveryRecord(const DisplayInfo &d, ColorState *state) const {
    QSettings settings(recoveryFilePath(),QSettings::IniFormat); const QString group=recoveryGroup(d);
    if(!settings.childGroups().contains(group))return false;
    settings.beginGroup(group);
    if(settings.value(QStringLiteral("stableId")).toString()!=d.stableId) { settings.endGroup(); return false; }
    state->previousProfile=settings.value(QStringLiteral("previousProfile")).toString();
    state->acmOriginallyEnabled=settings.value(QStringLiteral("acmOriginallyEnabled"),false).toBool();
    state->scope=settings.value(QStringLiteral("scope"),WcsScopeCurrentUser).toInt();
    state->captured=true; settings.endGroup(); return true;
}

void WindowsPlatformServices::clearRecoveryRecord(const DisplayInfo &d) {
    QSettings settings(recoveryFilePath(),QSettings::IniFormat);
    settings.remove(recoveryGroup(d)); settings.sync();
}

QString WindowsPlatformServices::defaultProfile(const DisplayInfo &d) {
    const auto fn=mscmsProc<ColorProfileGetDisplayDefaultFn>("ColorProfileGetDisplayDefault");
    if(!fn) return {};
    LUID luid{}; luid.HighPart=d.adapterHigh; luid.LowPart=d.adapterLow; PWSTR name=nullptr;
    const int preferred=profileScope(d);
    HRESULT hr=fn(preferred,luid,d.sourceId,ColorProfileTypeIcc,ColorProfileSubtypeStandard,&name);
    if(FAILED(hr)) hr=fn(preferred==WcsScopeCurrentUser?WcsScopeSystemWide:WcsScopeCurrentUser,luid,d.sourceId,ColorProfileTypeIcc,ColorProfileSubtypeStandard,&name);
    QString result;
    if(SUCCEEDED(hr)&&name) { result=QString::fromWCharArray(name); LocalFree(name); }
    return result;
}

int WindowsPlatformServices::profileScope(const DisplayInfo &d) {
    const auto fn=mscmsProc<ColorProfileGetDisplayUserScopeFn>("ColorProfileGetDisplayUserScope");
    if(!fn)return WcsScopeCurrentUser;
    LUID luid{};luid.HighPart=d.adapterHigh;luid.LowPart=d.adapterLow;int scope=WcsScopeCurrentUser;
    return SUCCEEDED(fn(luid,d.sourceId,&scope))?scope:WcsScopeCurrentUser;
}

IccBaseProfile WindowsPlatformServices::baseProfileFor(const DisplayInfo &d, const ColorState &state) const {
    if(m_baseProfiles.contains(d.stableId)) return m_baseProfiles.value(d.stableId);
    IccBaseProfile base; const QString profile=state.previousProfile;
    if(!profile.isEmpty()) {
        QFile f(QDir(colorDirectory()).filePath(profile));
        if(f.open(QIODevice::ReadOnly)) IccProfile::parseBase(f.readAll(),&base);
    }
    return base;
}

ApplyResult WindowsPlatformServices::ensureBroker() {
    if(m_pipe!=INVALID_HANDLE_VALUE) return ApplyResult::ok();
    m_pipeName=QStringLiteral("\\\\.\\pipe\\EinkAssistantColor-%1-%2")
        .arg(GetCurrentProcessId()).arg(QRandomGenerator::global()->generate64());
    m_pipe=CreateNamedPipeW(reinterpret_cast<LPCWSTR>(m_pipeName.utf16()),PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT,1,8192,8192,30000,nullptr);
    if(m_pipe==INVALID_HANDLE_VALUE) return ApplyResult::fail(errorMessage(QStringLiteral("Create color helper channel")));
    const QString args=QStringLiteral("--color-broker \"")+m_pipeName+QStringLiteral("\"");
    SHELLEXECUTEINFOW sei{}; sei.cbSize=sizeof(sei); sei.fMask=SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb=L"runas"; sei.lpFile=reinterpret_cast<LPCWSTR>(QCoreApplication::applicationFilePath().utf16());
    sei.lpParameters=reinterpret_cast<LPCWSTR>(args.utf16()); sei.nShow=SW_HIDE;
    if(!ShellExecuteExW(&sei)) { const DWORD code=GetLastError(); CloseHandle(m_pipe); m_pipe=INVALID_HANDLE_VALUE; return ApplyResult::fail(errorMessage(QStringLiteral("Start color profile helper"),code)); }
    m_brokerProcess=sei.hProcess;
    std::atomic<int> connectionState{0};
    const HANDLE pendingPipe=m_pipe;
    std::thread connector([pendingPipe,&connectionState]{
        const BOOL connected=ConnectNamedPipe(pendingPipe,nullptr)?TRUE:(GetLastError()==ERROR_PIPE_CONNECTED);
        connectionState.store(connected?1:-1);
    });
    for(int attempt=0;attempt<600&&connectionState.load()==0;++attempt) {
        if(WaitForSingleObject(m_brokerProcess,0)==WAIT_OBJECT_0)break;
        Sleep(100);
    }
    if(connectionState.load()==0) {
        CloseHandle(m_pipe);
        m_pipe=INVALID_HANDLE_VALUE;
    }
    connector.join();
    if(connectionState.load()!=1) {
        const DWORD code=GetLastError(); closeBroker();
        return ApplyResult::fail(errorMessage(QStringLiteral("Connect color profile helper"),code?code:ERROR_TIMEOUT));
    }
    return ApplyResult::ok();
}

ApplyResult WindowsPlatformServices::brokerCommand(const QString &command, QString *response) {
    const ApplyResult ready=ensureBroker(); if(!ready.success) return ready;
    const QByteArray bytes=(command+QLatin1Char('\n')).toUtf8(); DWORD written=0;
    if(!WriteFile(m_pipe,bytes.constData(),static_cast<DWORD>(bytes.size()),&written,nullptr))
        return ApplyResult::fail(errorMessage(QStringLiteral("Send color helper command")));
    QByteArray reply(4096,'\0'); DWORD read=0;
    if(!ReadFile(m_pipe,reply.data(),static_cast<DWORD>(reply.size()-1),&read,nullptr))
        return ApplyResult::fail(errorMessage(QStringLiteral("Read color helper response")));
    reply.resize(static_cast<int>(read)); const QString text=QString::fromUtf8(reply).trimmed(); if(response)*response=text;
    return text.startsWith(QStringLiteral("OK"))?ApplyResult::ok():ApplyResult::fail(text.mid(4));
}

ApplyResult WindowsPlatformServices::removeGeneratedAssociations(const DisplayInfo &d, QStringList *removed) {
    ApplyResult result=ApplyResult::ok(); QSet<QString> found;
    for(const int scope:{WcsScopeCurrentUser,WcsScopeSystemWide}) {
        QString listError; const QStringList profiles=associatedProfiles(d,scope,&listError);
        if(!listError.isEmpty()) { if(result.success)result=ApplyResult::fail(listError); continue; }
        QSet<QString> seenInScope;
        for(const QString &profile:profiles) {
            if(!isGeneratedProfile(profile) || seenInScope.contains(profile))continue;
            seenInScope.insert(profile);
            const ApplyResult removal=removeProfileAssociation(d,profile,scope);
            if(removal.success)found.insert(profile); else if(result.success)result=removal;
        }
    }
    if(removed)*removed=found.values();
    return result;
}

ApplyResult WindowsPlatformServices::uninstallGeneratedProfiles(const QStringList &profiles) {
    ApplyResult result=ApplyResult::ok(); QSet<QString> unique;
    for(const QString &profile:profiles) {
        if(!isGeneratedProfile(profile) || unique.contains(profile))continue;
        unique.insert(profile);
        const ApplyResult removed=brokerCommand(QStringLiteral("UNINSTALL\t%1").arg(profile));
        if(!removed.success && result.success)result=removed;
    }
    return result;
}

ApplyResult WindowsPlatformServices::recoverInterruptedColorState() {
    if(m_recoveryComplete)return ApplyResult::ok();
    if(!saturationPlatformAvailable()) { m_recoveryComplete=true; return ApplyResult::ok(); }
    const QStringList installed=QDir(colorDirectory()).entryList({QStringLiteral("EinkAssistant-*.icm")},QDir::Files);
    ApplyResult result=ApplyResult::ok(); bool associationsClean=true; QStringList generated=installed;
    const QVector<DisplayInfo> current=displays();
    for(const DisplayInfo &display:current) {
        ColorState saved; const bool hasRecord=readRecoveryRecord(display,&saved);
        const QString defaultBefore=defaultProfile(display);
        const bool legacyGeneratedDefault=isGeneratedProfile(defaultBefore)
            || (hasRecord && isGeneratedProfile(saved.previousProfile));
        if(legacyGeneratedDefault) { saved.previousProfile.clear(); saved.acmOriginallyEnabled=false; }
        ApplyResult displayResult=ApplyResult::ok();
        if(hasRecord && !saved.previousProfile.isEmpty() && !isGeneratedProfile(saved.previousProfile)) {
            const ApplyResult restored=setDefaultProfile(display,saved.previousProfile,saved.scope);
            if(!restored.success)displayResult=restored;
        }
        QStringList removed; const ApplyResult cleaned=removeGeneratedAssociations(display,&removed);
        generated.append(removed);
        if(!cleaned.success) { associationsClean=false; if(displayResult.success)displayResult=cleaned; }
        if((hasRecord || legacyGeneratedDefault) && display.acmSupported) {
            bool supported=false,enabled=false; QString queryError;
            if(!queryAcm(display,&supported,&enabled,&queryError)) {
                if(displayResult.success)displayResult=ApplyResult::fail(queryError);
            } else if(enabled!=saved.acmOriginallyEnabled) {
                const ApplyResult restored=setAcm(display,saved.acmOriginallyEnabled);
                if(!restored.success && displayResult.success)displayResult=restored;
            }
        }
        if(displayResult.success && hasRecord)clearRecoveryRecord(display);
        if(!displayResult.success && result.success)result=displayResult;
    }
    const ApplyResult uninstalled=uninstallGeneratedProfiles(generated);
    if(!uninstalled.success && result.success)result=uninstalled;
    m_recoveryComplete=associationsClean;
    return result;
}

ApplyResult WindowsPlatformServices::applyColor(const DisplayInfo &d, double saturation, const RgbBalance &balance) {
    if(!saturationPlatformAvailable() || !d.colorAdjustmentSupported)
        return ApplyResult::fail(QStringLiteral("Saturation is unavailable for the active color pipeline on %1.").arg(d.friendlyName));
    ColorState pendingRecovery;
    if(!m_colorStates.contains(d.stableId)
        && (readRecoveryRecord(d,&pendingRecovery) || isGeneratedProfile(defaultProfile(d))))
        m_recoveryComplete=false;
    if(!m_recoveryComplete) {
        const ApplyResult recovered=recoverInterruptedColorState();
        if(!m_recoveryComplete)return recovered;
    }
    ColorState &state=m_colorStates[d.stableId];
    if(!state.captured) {
        state.previousProfile=defaultProfile(d); state.scope=profileScope(d);
        if(isGeneratedProfile(state.previousProfile))state.previousProfile.clear();
        if(d.acmSupported) {
            bool supported=false,enabled=false; QString error;
            if(!queryAcm(d,&supported,&enabled,&error)) return ApplyResult::fail(error);
            state.acmOriginallyEnabled=enabled;
        }
        const IccBaseProfile base=baseProfileFor(d,state); m_baseProfiles.insert(d.stableId,base);
        const ApplyResult journal=writeRecoveryRecord(d,state); if(!journal.success)return journal;
        state.captured=true;
    }
    if(d.acmSupported&&!state.acmOriginallyEnabled&&!state.acmChanged) {
        const ApplyResult acm=setAcm(d,true); if(!acm.success) return acm; state.acmChanged=true;
    }
    const QString fileName=QStringLiteral("%1%2-%3.icm").arg(profilePrefix(d),m_profileSessionId).arg(++m_profileSequence);
    if(!m_profileTemp||!m_profileTemp->isValid())return ApplyResult::fail(QStringLiteral("Could not create the temporary ICC profile directory."));
    const QString path=QDir::toNativeSeparators(QFileInfo(QDir(m_profileTemp->path()).filePath(fileName)).absoluteFilePath());
    const QString label=QStringLiteral("%1 - Saturation %2% - RGB %3/%4/%5%")
        .arg(d.friendlyName).arg(qRound(saturation*100)).arg(qRound(balance.red*100)).arg(qRound(balance.green*100)).arg(qRound(balance.blue*100));
    const QByteArray profile=IccProfile::make(saturation,balance,label,m_baseProfiles.value(d.stableId));
    QString profileError;
    if(!IccProfile::structurallyValid(profile,&profileError)) return ApplyResult::fail(QStringLiteral("Generated ICC profile is invalid: ")+profileError);
    QFile out(path); if(!out.open(QIODevice::WriteOnly)||out.write(profile)!=profile.size()) return ApplyResult::fail(QStringLiteral("Could not write the temporary ICC profile.")); out.close();
    LUID luid{}; luid.HighPart=d.adapterHigh; luid.LowPart=d.adapterLow;
    const QString command=QStringLiteral("APPLY\t%1\t%2\t%3\t%4\t%5\t%6")
        .arg(path,fileName).arg(luid.HighPart).arg(luid.LowPart).arg(d.sourceId).arg(state.scope);
    const ApplyResult applied=brokerCommand(command); if(!applied.success) return applied;
    if(!state.currentProfile.isEmpty() && state.currentProfile!=fileName) {
        removeProfileAssociation(d,state.currentProfile,state.scope);
        brokerCommand(QStringLiteral("UNINSTALL\t%1").arg(state.currentProfile));
    }
    state.currentProfile=fileName; return ApplyResult::ok();
}

ApplyResult WindowsPlatformServices::removeProfileAssociation(const DisplayInfo &d, const QString &profile, int scope) {
    const int requestedScope=scope<0?profileScope(d):scope;
    if(requestedScope==WcsScopeSystemWide) {
        return brokerCommand(QStringLiteral("REMOVE\t%1\t%2\t%3\t%4\t%5")
            .arg(profile).arg(d.adapterHigh).arg(d.adapterLow).arg(d.sourceId).arg(requestedScope));
    }
    const auto fn=mscmsProc<ColorProfileRemoveDisplayAssociationFn>("ColorProfileRemoveDisplayAssociation");
    if(!fn) return ApplyResult::fail(QStringLiteral("Modern Windows color profile APIs are unavailable."));
    LUID luid{}; luid.HighPart=d.adapterHigh; luid.LowPart=d.adapterLow;
    const HRESULT hr=fn(requestedScope,reinterpret_cast<LPCWSTR>(profile.utf16()),luid,d.sourceId,FALSE);
    return SUCCEEDED(hr)?ApplyResult::ok():ApplyResult::fail(QStringLiteral("Remove color profile association failed (0x%1).").arg(static_cast<quint32>(hr),8,16,QLatin1Char('0')));
}

ApplyResult WindowsPlatformServices::setDefaultProfile(const DisplayInfo &d, const QString &profile, int scope) {
    if(profile.isEmpty()) return ApplyResult::ok();
    const int requestedScope=scope<0?profileScope(d):scope;
    if(requestedScope==WcsScopeSystemWide) {
        return brokerCommand(QStringLiteral("SETDEFAULT\t%1\t%2\t%3\t%4\t%5")
            .arg(profile).arg(d.adapterHigh).arg(d.adapterLow).arg(d.sourceId).arg(requestedScope));
    }
    const auto fn=mscmsProc<ColorProfileSetDisplayDefaultAssociationFn>("ColorProfileSetDisplayDefaultAssociation");
    if(!fn) return ApplyResult::fail(QStringLiteral("Modern Windows color profile APIs are unavailable."));
    LUID luid{}; luid.HighPart=d.adapterHigh; luid.LowPart=d.adapterLow;
    const HRESULT hr=fn(requestedScope,reinterpret_cast<LPCWSTR>(profile.utf16()),ColorProfileTypeIcc,
                        ColorProfileSubtypeStandard,luid,d.sourceId);
    return SUCCEEDED(hr)?ApplyResult::ok():ApplyResult::fail(QStringLiteral("Restore default color profile failed (0x%1).").arg(static_cast<quint32>(hr),8,16,QLatin1Char('0')));
}

ApplyResult WindowsPlatformServices::restoreColor(const DisplayInfo &d) {
    if(!m_colorStates.contains(d.stableId)) return ApplyResult::ok();
    const ColorState state=m_colorStates.value(d.stableId); ApplyResult result=ApplyResult::ok();
    if(!state.previousProfile.isEmpty() && !isGeneratedProfile(state.previousProfile)) {
        const ApplyResult previous=setDefaultProfile(d,state.previousProfile,state.scope); if(!previous.success)result=previous;
    }
    QStringList generated; const ApplyResult removed=removeGeneratedAssociations(d,&generated);
    if(!removed.success && result.success)result=removed;
    const QString prefix=profilePrefix(d);
    generated.append(QDir(colorDirectory()).entryList({prefix+QStringLiteral("*.icm")},QDir::Files));
    const ApplyResult uninstalled=uninstallGeneratedProfiles(generated);
    if(!uninstalled.success && result.success)result=uninstalled;
    if(state.acmChanged) { const ApplyResult acm=setAcm(d,false); if(!acm.success) result=acm; }
    if(result.success) { clearRecoveryRecord(d); m_colorStates.remove(d.stableId); m_baseProfiles.remove(d.stableId); }
    return result;
}

ApplyResult WindowsPlatformServices::setDitheringDisabled(const DisplayInfo &, bool) {
    return ApplyResult::fail(QStringLiteral("This GPU does not expose a safe public per-display dithering control on Windows."));
}

bool WindowsPlatformServices::visualEffectsReduced() const {
    BOOL clientAnimation=TRUE,uiEffects=TRUE;
    ANIMATIONINFO windowAnimation{};windowAnimation.cbSize=sizeof(windowAnimation);windowAnimation.iMinAnimate=TRUE;
    const bool clientKnown=SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&clientAnimation,0);
    const bool uiKnown=SystemParametersInfoW(SPI_GETUIEFFECTS,0,&uiEffects,0);
    const bool windowKnown=SystemParametersInfoW(SPI_GETANIMATION,sizeof(windowAnimation),&windowAnimation,0);
    if(!clientKnown||!uiKnown||!windowKnown)return false;
    const bool animationEnabled=clientAnimation||uiEffects||windowAnimation.iMinAnimate!=0;
    DWORD transparency=1;
    if(windowsBuild()>=10240) {
        parseBoolRegistry(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",L"EnableTransparency",&transparency);
        return !animationEnabled && transparency==0;
    }
    transparency=0;
    parseBoolRegistry(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\DWM",L"ColorizationOpaqueBlend",&transparency);
    return !animationEnabled && transparency==1;
}

ApplyResult WindowsPlatformServices::setVisualEffectsReduced(bool reduced) {
    const BOOL animationEnabled=reduced?FALSE:TRUE;
    PVOID animationValue=reinterpret_cast<PVOID>(static_cast<ULONG_PTR>(animationEnabled));
    const UINT notify=SPIF_UPDATEINIFILE|SPIF_SENDCHANGE;
    DWORD animationError=ERROR_SUCCESS;
    if(!SystemParametersInfoW(SPI_SETUIEFFECTS,0,animationValue,notify))animationError=GetLastError();
    if(!SystemParametersInfoW(SPI_SETCLIENTAREAANIMATION,0,animationValue,notify)&&animationError==ERROR_SUCCESS)animationError=GetLastError();
    ANIMATIONINFO windowAnimation{};windowAnimation.cbSize=sizeof(windowAnimation);windowAnimation.iMinAnimate=animationEnabled;
    if(!SystemParametersInfoW(SPI_SETANIMATION,sizeof(windowAnimation),&windowAnimation,notify)&&animationError==ERROR_SUCCESS)animationError=GetLastError();
    HKEY key=nullptr;
    const bool modern=windowsBuild()>=10240;
    const wchar_t *path=modern?L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize":L"Software\\Microsoft\\Windows\\DWM";
    const wchar_t *name=modern?L"EnableTransparency":L"ColorizationOpaqueBlend";
    if(RegCreateKeyExW(HKEY_CURRENT_USER,path,0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)==ERROR_SUCCESS) {
        const DWORD transparency=modern?(reduced?0u:1u):(reduced?1u:0u);
        RegSetValueExW(key,name,0,REG_DWORD,reinterpret_cast<const BYTE*>(&transparency),sizeof(transparency)); RegCloseKey(key);
    }
    SendMessageTimeoutW(HWND_BROADCAST,WM_SETTINGCHANGE,0,reinterpret_cast<LPARAM>(L"ImmersiveColorSet"),SMTO_ABORTIFHUNG,1000,nullptr);
    return animationError==ERROR_SUCCESS?ApplyResult::ok()
        :ApplyResult::fail(errorMessage(QStringLiteral("Change animation effects"),animationError));
}

bool WindowsPlatformServices::windowsLightModeAvailable() const { return windowsBuild()>=18362; }

bool WindowsPlatformServices::windowsLightModeEnabled() const {
    if(!windowsLightModeAvailable())return false;
    constexpr wchar_t path[]=L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    DWORD system=0;
    return parseBoolRegistry(HKEY_CURRENT_USER,path,L"SystemUsesLightTheme",&system) && system==1;
}

ApplyResult WindowsPlatformServices::setWindowsLightModeEnabled(bool enabled) {
    if(!windowsLightModeAvailable())
        return ApplyResult::fail(QStringLiteral("Windows Light Mode requires Windows 10 May 2019 Update or above."));
    constexpr wchar_t path[]=L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    HKEY key=nullptr;
    const LONG opened=RegCreateKeyExW(HKEY_CURRENT_USER,path,0,nullptr,0,KEY_QUERY_VALUE|KEY_SET_VALUE,nullptr,&key,nullptr);
    if(opened!=ERROR_SUCCESS)return ApplyResult::fail(errorMessage(QStringLiteral("Open Windows color-mode setting"),opened));
    const DWORD value=enabled?1u:0u;
    const LONG system=RegSetValueExW(key,L"SystemUsesLightTheme",0,REG_DWORD,reinterpret_cast<const BYTE *>(&value),sizeof(value));
    RegCloseKey(key);
    if(system!=ERROR_SUCCESS)return ApplyResult::fail(errorMessage(QStringLiteral("Change Windows color mode"),system));
    SendMessageTimeoutW(HWND_BROADCAST,WM_SETTINGCHANGE,0,reinterpret_cast<LPARAM>(L"ImmersiveColorSet"),SMTO_ABORTIFHUNG,1000,nullptr);
    return ApplyResult::ok();
}

ApplyResult WindowsPlatformServices::setLaunchAtLogin(bool enabled) {
    ComApartment apartment;
    if(!apartment.ready())return ApplyResult::fail(errorMessage(QStringLiteral("Initialize Task Scheduler"),static_cast<DWORD>(apartment.result())));
    ComObject<ITaskFolder> folder;
    HRESULT result=openTaskFolder(folder.put());
    if(FAILED(result))return ApplyResult::fail(errorMessage(QStringLiteral("Open Task Scheduler"),static_cast<DWORD>(result)));
    ScopedBstr taskName(QString::fromWCharArray(kLaunchTaskName));
    if(enabled) {
        const QString sid=currentUserSid();
        if(sid.isEmpty())return ApplyResult::fail(errorMessage(QStringLiteral("Read current user identity")));
        ScopedBstr xml(launchTaskXml(sid));
        VARIANT user,password,sddl;
        VariantInit(&user);VariantInit(&password);VariantInit(&sddl);
        user.vt=VT_BSTR;user.bstrVal=SysAllocStringLen(reinterpret_cast<const OLECHAR*>(sid.utf16()),static_cast<UINT>(sid.size()));
        ComObject<IRegisteredTask> task;
        result=folder.get()->RegisterTask(taskName.get(),xml.get(),
            TASK_CREATE_OR_UPDATE|TASK_IGNORE_REGISTRATION_TRIGGERS,user,password,
            TASK_LOGON_INTERACTIVE_TOKEN,sddl,task.put());
        VariantClear(&user);
        if(FAILED(result))return ApplyResult::fail(errorMessage(QStringLiteral("Create login task"),static_cast<DWORD>(result)));
        const LONG legacyResult=removeLegacyRunEntry();
        if(legacyResult!=ERROR_SUCCESS)return ApplyResult::fail(errorMessage(QStringLiteral("Remove old startup entry"),legacyResult));
        return ApplyResult::ok();
    }
    result=folder.get()->DeleteTask(taskName.get(),0);
    if(result==HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))result=S_OK;
    if(FAILED(result))return ApplyResult::fail(errorMessage(QStringLiteral("Remove login task"),static_cast<DWORD>(result)));
    const LONG legacyResult=removeLegacyRunEntry();
    return legacyResult==ERROR_SUCCESS?ApplyResult::ok()
        :ApplyResult::fail(errorMessage(QStringLiteral("Remove old startup entry"),legacyResult));
}

bool WindowsPlatformServices::launchAtLogin() const {
    ComApartment apartment;
    if(!apartment.ready())return false;
    ComObject<ITaskFolder> folder;
    if(FAILED(openTaskFolder(folder.put())))return false;
    ScopedBstr taskName(QString::fromWCharArray(kLaunchTaskName));
    ComObject<IRegisteredTask> task;
    if(FAILED(folder.get()->GetTask(taskName.get(),task.put())))return false;
    VARIANT_BOOL enabled=VARIANT_FALSE;
    return SUCCEEDED(task.get()->get_Enabled(&enabled))&&enabled==VARIANT_TRUE;
}

QString WindowsPlatformServices::registeredLaunchTaskXmlForDiagnostics() const {
    ComApartment apartment;
    if(!apartment.ready())return {};
    ComObject<ITaskFolder> folder;
    if(FAILED(openTaskFolder(folder.put())))return {};
    ScopedBstr taskName(QString::fromWCharArray(kLaunchTaskName));
    ComObject<IRegisteredTask> task;
    if(FAILED(folder.get()->GetTask(taskName.get(),task.put())))return {};
    BSTR xml=nullptr;
    if(FAILED(task.get()->get_Xml(&xml))||!xml)return {};
    const QString result=QString::fromWCharArray(xml,static_cast<int>(SysStringLen(xml)));
    SysFreeString(xml);
    return result;
}

void WindowsPlatformServices::openVisualEffectsSettings() {
    if(windowsBuild()>=10240) ShellExecuteW(nullptr,L"open",L"ms-settings:easeofaccess-visualeffects",nullptr,nullptr,SW_SHOWNORMAL);
    else ShellExecuteW(nullptr,L"open",L"control.exe",L"/name Microsoft.EaseOfAccessCenter",nullptr,SW_SHOWNORMAL);
}

ApplyResult WindowsPlatformServices::recoverInterruptedNightLightState() {
    const QString journal=nightLightRecoveryFilePath();
    if(!QFileInfo::exists(journal))return ApplyResult::ok();
    QByteArray original;bool originalEnabled=false;
    if(!readNightLightRecovery(&original,&originalEnabled))
        return ApplyResult::fail(QStringLiteral("The Night Light crash-recovery journal is invalid; no registry value was changed."));
    NightLightStateRecord originalRecord;QString parseError;
    if(!NightLightStateCodec::decode(original,&originalRecord,&parseError))
        return ApplyResult::fail(QStringLiteral("The Night Light crash-recovery state is invalid: ")+parseError);
    ApplyResult restored=setNativeNightLight(originalEnabled);if(!restored.success)return restored;
    originalRecord.enabled=originalEnabled;updateNightLightTimestamps(&originalRecord);restored=writeNightLightState(NightLightStateCodec::encode(originalRecord));if(!restored.success)return restored;
    QByteArray verified;const ApplyResult readBack=readNightLightState(&verified);if(!readBack.success)return readBack;NightLightStateRecord verifiedRecord;
    if(!NightLightStateCodec::decode(verified,&verifiedRecord,&parseError)||verifiedRecord.enabled!=originalEnabled)
        return ApplyResult::fail(QStringLiteral("Night Light crash recovery could not be verified."));
    clearNightLightRecovery();m_originalNightLightState.clear();m_originalNightLightEnabled=originalEnabled;
    m_lastKnownNightLightEnabled=originalEnabled;m_nightLightStateKnown=true;m_nightLightOwned=false;return ApplyResult::ok();
}

bool WindowsPlatformServices::nightLightControlAvailable() const {
    if(windowsBuild()<kDirectNightLightMinimumBuild)return false;
    if(!QFileInfo::exists(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("EinkNightLightControl.exe"))))return false;
    QByteArray data;if(!readNightLightState(&data).success)return false;
    NightLightStateRecord record;return NightLightStateCodec::decode(data,&record,nullptr);
}

bool WindowsPlatformServices::nightLightEnabled() const {
    if(m_nightLightStateKnown)return m_lastKnownNightLightEnabled;
    QByteArray data;if(!readNightLightState(&data).success)return false;
    NightLightStateRecord record;return NightLightStateCodec::decode(data,&record,nullptr)&&record.enabled;
}

ApplyResult WindowsPlatformServices::setNightLightDisabled(bool disabled) {
    if(!nightLightControlAvailable())
        return ApplyResult::fail(QStringLiteral("Direct Night Light control is unavailable on this Windows version or state format."));
    QByteArray current;ApplyResult result=readNightLightState(&current);if(!result.success)return result;
    NightLightStateRecord record;QString parseError;
    if(!NightLightStateCodec::decode(current,&record,&parseError))
        return ApplyResult::fail(QStringLiteral("Night Light state validation failed: ")+parseError);
    bool closeSettings=false;
    if(!m_nightLightOwned) {
        bool originalEnabled=false;
        result=queryNativeNightLight(&originalEnabled,&closeSettings);if(!result.success)return result;
        result=writeNightLightRecovery(current,originalEnabled);if(!result.success)return result;
        m_originalNightLightState=current;m_originalNightLightEnabled=originalEnabled;m_nightLightOwned=true;
    }
    const bool targetEnabled=!disabled;
    result=setNativeNightLight(targetEnabled,closeSettings);if(!result.success)return result;
    if(record.enabled!=targetEnabled) {
        record.enabled=targetEnabled;
        updateNightLightTimestamps(&record);
        result=writeNightLightState(NightLightStateCodec::encode(record));if(!result.success)return result;
    }
    QByteArray verified;result=readNightLightState(&verified);if(!result.success)return result;
    NightLightStateRecord verifiedRecord;
    if(!NightLightStateCodec::decode(verified,&verifiedRecord,&parseError)||verifiedRecord.enabled!=targetEnabled)
        return ApplyResult::fail(QStringLiteral("Changing Night Light could not be verified."));
    m_lastKnownNightLightEnabled=targetEnabled;m_nightLightStateKnown=true;
    return ApplyResult::ok();
}

ApplyResult WindowsPlatformServices::restoreNightLightState() {
    QByteArray original=m_originalNightLightState;bool originalEnabled=m_originalNightLightEnabled;
    if(original.isEmpty()) {
        const QString journal=nightLightRecoveryFilePath();
        if(!QFileInfo::exists(journal))return ApplyResult::ok();
        if(!readNightLightRecovery(&original,&originalEnabled))
            return ApplyResult::fail(QStringLiteral("The Night Light crash-recovery journal is invalid; the original state was not restored."));
    }
    NightLightStateRecord originalRecord;QString parseError;
    if(!NightLightStateCodec::decode(original,&originalRecord,&parseError))
        return ApplyResult::fail(QStringLiteral("The saved original Night Light state is invalid: ")+parseError);
    ApplyResult restored=setNativeNightLight(originalEnabled);if(!restored.success)return restored;
    originalRecord.enabled=originalEnabled;updateNightLightTimestamps(&originalRecord);restored=writeNightLightState(NightLightStateCodec::encode(originalRecord));if(!restored.success)return restored;
    QByteArray verified;const ApplyResult readBack=readNightLightState(&verified);if(!readBack.success)return readBack;NightLightStateRecord verifiedRecord;
    if(!NightLightStateCodec::decode(verified,&verifiedRecord,&parseError)||verifiedRecord.enabled!=originalEnabled)
        return ApplyResult::fail(QStringLiteral("Restoring the original Night Light state could not be verified."));
    clearNightLightRecovery();m_originalNightLightState.clear();m_originalNightLightEnabled=originalEnabled;
    m_lastKnownNightLightEnabled=originalEnabled;m_nightLightStateKnown=true;m_nightLightOwned=false;return ApplyResult::ok();
}

bool WindowsPlatformServices::nightLightAvailable() const { return windowsBuild()>=15063; }

void WindowsPlatformServices::openNightLightSettings() {
    if(nightLightAvailable())ShellExecuteW(nullptr,L"open",L"ms-settings:nightlight",nullptr,nullptr,SW_SHOWNORMAL);
}

bool WindowsPlatformServices::saturationPlatformAvailable() const {
    const quint32 build=windowsBuild();
    return modernColorProfileApisAvailable()
        &&(build>=26100||(build>=19041&&build<22000));
}

QString WindowsPlatformServices::platformDiagnostic() const {
    const quint32 build=windowsBuild();
    const QString path=build>=26100?QStringLiteral("Windows 11 ACM")
        :(saturationPlatformAvailable()?QStringLiteral("Windows 10 MHC2 compatibility"):QStringLiteral("unavailable"));
    return QStringLiteral("Windows build %1; color profile path %2").arg(build).arg(path);
}

QStringList WindowsPlatformServices::generatedProfilesForDiagnostics(const DisplayInfo &display) const {
    QSet<QString> generated;
    for(const int scope:{WcsScopeCurrentUser,WcsScopeSystemWide})
        for(const QString &profile:associatedProfiles(display,scope))
            if(isGeneratedProfile(profile))generated.insert(profile);
    return generated.values();
}

QStringList WindowsPlatformServices::installedGeneratedProfilesForDiagnostics() const {
    return QDir(colorDirectory()).entryList({QStringLiteral("EinkAssistant-*.icm")},QDir::Files);
}

ApplyResult WindowsPlatformServices::cleanupGeneratedColorForDiagnostics() {
    m_recoveryComplete=false;
    return recoverInterruptedColorState();
}

void WindowsPlatformServices::closeBroker() {
    if(m_pipe!=INVALID_HANDLE_VALUE) { const QByteArray quit("QUIT\n"); DWORD written=0; WriteFile(m_pipe,quit.constData(),quit.size(),&written,nullptr); FlushFileBuffers(m_pipe); DisconnectNamedPipe(m_pipe); CloseHandle(m_pipe); m_pipe=INVALID_HANDLE_VALUE; }
    if(m_brokerProcess) { WaitForSingleObject(m_brokerProcess,5000); CloseHandle(m_brokerProcess); m_brokerProcess=nullptr; }
}

void WindowsPlatformServices::shutdown() {
    if(m_shutdown)return;
    restoreNightLightState();
    m_shutdown=true;
    const QVector<DisplayInfo> current=displays();
    for(const DisplayInfo &d:current) { restoreToneCurve(d); restoreColor(d); }
    closeBroker();
}

int WindowsPlatformServices::runColorBroker(const QString &pipeName) {
    HANDLE pipe=INVALID_HANDLE_VALUE;
    for(int i=0;i<100 && pipe==INVALID_HANDLE_VALUE;++i) {
        pipe=CreateFileW(reinterpret_cast<LPCWSTR>(pipeName.utf16()),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);
        if(pipe==INVALID_HANDLE_VALUE) Sleep(100);
    }
    if(pipe==INVALID_HANDLE_VALUE)return 2;
    QByteArray pending; char buffer[4096];
    for(;;) {
        DWORD read=0; if(!ReadFile(pipe,buffer,sizeof(buffer),&read,nullptr)||!read)break; pending.append(buffer,static_cast<int>(read));
        int newline;
        while((newline=pending.indexOf('\n'))>=0) {
            const QString line=QString::fromUtf8(pending.left(newline)); pending.remove(0,newline+1);
            const QStringList parts=line.split(QLatin1Char('\t')); QString reply=QStringLiteral("OK");
            if(parts.value(0)==QStringLiteral("QUIT")) { CloseHandle(pipe); return 0; }
            if(parts.value(0)==QStringLiteral("APPLY") && parts.size()>=7) {
                const QString path=parts[1],name=parts[2]; LUID luid{}; luid.HighPart=parts[3].toInt(); luid.LowPart=parts[4].toUInt(); const UINT32 target=parts[5].toUInt();const int scope=parts[6].toInt();
                if(!QFileInfo::exists(path))reply=QStringLiteral("ERR Temporary ICC profile is missing: ")+path;
                else if(!InstallColorProfileW(nullptr,reinterpret_cast<LPCWSTR>(path.utf16()))) reply=QStringLiteral("ERR ")+errorMessage(QStringLiteral("Install ICC profile"))+QStringLiteral(" [")+path+QStringLiteral("]");
                else {
                    const auto add=mscmsProc<ColorProfileAddDisplayAssociationFn>("ColorProfileAddDisplayAssociation");
                    if(!add) reply=QStringLiteral("ERR Modern color profile API unavailable");
                    else {
                        // setAsDefault=TRUE makes this association the default atomically.
                        // Calling ColorProfileSetDisplayDefaultAssociation again is redundant
                        // and returns E_INVALIDARG for some ACM displays after a successful add.
                        const HRESULT hr=add(scope,reinterpret_cast<LPCWSTR>(name.utf16()),luid,target,TRUE,FALSE);
                        if(FAILED(hr))reply=QStringLiteral("ERR Associate and set default ICC profile failed (0x%1)").arg(static_cast<quint32>(hr),8,16,QLatin1Char('0'));
                    }
                }
            } else if(parts.value(0)==QStringLiteral("REMOVE") && parts.size()>=6) {
                const QString name=parts[1];LUID luid{};luid.HighPart=parts[2].toInt();luid.LowPart=parts[3].toUInt();const UINT32 source=parts[4].toUInt();const int scope=parts[5].toInt();const auto remove=mscmsProc<ColorProfileRemoveDisplayAssociationFn>("ColorProfileRemoveDisplayAssociation");const HRESULT hr=remove?remove(scope,reinterpret_cast<LPCWSTR>(name.utf16()),luid,source,FALSE):E_NOTIMPL;if(FAILED(hr))reply=QStringLiteral("ERR Remove ICC profile association failed (0x%1)").arg(static_cast<quint32>(hr),8,16,QLatin1Char('0'));
            } else if(parts.value(0)==QStringLiteral("SETDEFAULT") && parts.size()>=6) {
                const QString name=parts[1];LUID luid{};luid.HighPart=parts[2].toInt();luid.LowPart=parts[3].toUInt();const UINT32 source=parts[4].toUInt();const int scope=parts[5].toInt();const auto setDefault=mscmsProc<ColorProfileSetDisplayDefaultAssociationFn>("ColorProfileSetDisplayDefaultAssociation");const HRESULT hr=setDefault?setDefault(scope,reinterpret_cast<LPCWSTR>(name.utf16()),ColorProfileTypeIcc,ColorProfileSubtypeStandard,luid,source):E_NOTIMPL;if(FAILED(hr))reply=QStringLiteral("ERR Restore default ICC profile failed (0x%1)").arg(static_cast<quint32>(hr),8,16,QLatin1Char('0'));
            } else if(parts.value(0)==QStringLiteral("UNINSTALL") && parts.size()>=2) {
                if(!UninstallColorProfileW(nullptr,reinterpret_cast<LPCWSTR>(parts[1].utf16()),TRUE)) {
                    const DWORD code=GetLastError(); if(code!=ERROR_FILE_NOT_FOUND)reply=QStringLiteral("ERR ")+errorMessage(QStringLiteral("Uninstall ICC profile"),code);
                }
            } else if(parts.value(0)!=QStringLiteral("APPLY") && parts.value(0)!=QStringLiteral("REMOVE") && parts.value(0)!=QStringLiteral("SETDEFAULT") && parts.value(0)!=QStringLiteral("UNINSTALL")) reply=QStringLiteral("ERR Unknown helper command");
            const QByteArray out=(reply+QLatin1Char('\n')).toUtf8(); DWORD written=0; WriteFile(pipe,out.constData(),out.size(),&written,nullptr); FlushFileBuffers(pipe);
        }
    }
    CloseHandle(pipe); return 0;
}

} // namespace eink
