#include "WindowsGpuControlPanel.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <algorithm>
#include <dxgi.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <windows.h>

namespace eink::windows {
namespace {

QString normalizedPath(QString value) {
    value=QDir::cleanPath(value.trimmed());
    return value.toLower();
}

QString joined(const QString &root, const QString &relative) {
    return root.isEmpty()?QString{}:QDir::cleanPath(QDir(root).filePath(relative));
}

QString knownFolderPath(REFKNOWNFOLDERID id) {
    PWSTR path=nullptr;
    const HRESULT result=SHGetKnownFolderPath(id,KF_FLAG_DONT_VERIFY,nullptr,&path);
    const QString value=SUCCEEDED(result)&&path?QString::fromWCharArray(path):QString{};
    if(path)CoTaskMemFree(path);
    return QDir::cleanPath(value);
}

QString windowsDirectoryPath() {
    QVector<wchar_t> buffer(MAX_PATH+1);
    UINT size=GetWindowsDirectoryW(buffer.data(),static_cast<UINT>(buffer.size()));
    if(!size)return {};
    if(size>=static_cast<UINT>(buffer.size())){buffer.resize(static_cast<int>(size+1));size=GetWindowsDirectoryW(buffer.data(),static_cast<UINT>(buffer.size()));}
    return size?QDir::cleanPath(QString::fromWCharArray(buffer.constData(),static_cast<int>(size))):QString{};
}

class ComApartment final {
public:
    ComApartment() {
        const HRESULT result=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
        m_ready=SUCCEEDED(result)||result==RPC_E_CHANGED_MODE;
        m_uninitialize=SUCCEEDED(result);
    }
    ~ComApartment(){if(m_uninitialize)CoUninitialize();}
    bool ready() const { return m_ready; }
private:
    bool m_ready=false;
    bool m_uninitialize=false;
};

QString shellName(IShellFolder *folder, PCUITEMID_CHILD item, SHGDNF flags) {
    STRRET value{};
    if(FAILED(folder->GetDisplayNameOf(item,flags,&value)))return {};
    wchar_t buffer[2048]{};
    return SUCCEEDED(StrRetToBufW(&value,item,buffer,static_cast<UINT>(std::size(buffer))))
        ?QString::fromWCharArray(buffer).trimmed():QString{};
}

QVector<RegisteredShellApplication> registeredShellApplications() {
    QVector<RegisteredShellApplication> result;
    ComApartment apartment;if(!apartment.ready())return result;
    PIDLIST_ABSOLUTE appsId=nullptr;
    if(FAILED(SHParseDisplayName(L"shell:AppsFolder",nullptr,&appsId,0,nullptr))||!appsId)return result;
    IShellFolder *desktop=nullptr,*apps=nullptr;
    if(SUCCEEDED(SHGetDesktopFolder(&desktop))&&desktop) {
        desktop->BindToObject(appsId,nullptr,IID_IShellFolder,reinterpret_cast<void**>(&apps));
        desktop->Release();
    }
    CoTaskMemFree(appsId);
    if(!apps)return result;
    IEnumIDList *items=nullptr;
    if(SUCCEEDED(apps->EnumObjects(nullptr,SHCONTF_NONFOLDERS,&items))&&items) {
        PITEMID_CHILD item=nullptr;ULONG fetched=0;
        while(items->Next(1,&item,&fetched)==S_OK) {
            RegisteredShellApplication application;
            application.name=shellName(apps,item,SHGDN_NORMAL);
            application.id=shellName(apps,item,SHGDN_FORPARSING);
            const QString prefix=QStringLiteral("shell:AppsFolder\\");
            if(application.id.startsWith(prefix,Qt::CaseInsensitive))application.id.remove(0,prefix.size());
            if(!application.id.isEmpty())result.push_back(application);
            CoTaskMemFree(item);item=nullptr;
        }
        items->Release();
    }
    apps->Release();
    return result;
}

QString expandedRegistryString(HKEY root, const QString &subkey, REGSAM view) {
    HKEY key=nullptr;
    if(RegOpenKeyExW(root,reinterpret_cast<LPCWSTR>(subkey.utf16()),0,KEY_QUERY_VALUE|view,&key)!=ERROR_SUCCESS)return {};
    DWORD type=0,bytes=0;
    LONG result=RegQueryValueExW(key,nullptr,nullptr,&type,nullptr,&bytes);
    if(result!=ERROR_SUCCESS||(type!=REG_SZ&&type!=REG_EXPAND_SZ)||bytes<sizeof(wchar_t)){RegCloseKey(key);return {};}
    QVector<wchar_t> buffer(static_cast<int>(bytes/sizeof(wchar_t))+2);
    result=RegQueryValueExW(key,nullptr,nullptr,&type,reinterpret_cast<BYTE*>(buffer.data()),&bytes);
    RegCloseKey(key);if(result!=ERROR_SUCCESS)return {};
    QString value=QString::fromWCharArray(buffer.constData()).trimmed();
    if(value.size()>=2&&value.front()==QLatin1Char('"')&&value.back()==QLatin1Char('"'))value=value.mid(1,value.size()-2);
    if(type==REG_EXPAND_SZ) {
        const DWORD required=ExpandEnvironmentStringsW(reinterpret_cast<LPCWSTR>(value.utf16()),nullptr,0);
        if(required) {QVector<wchar_t> expanded(static_cast<int>(required));if(ExpandEnvironmentStringsW(reinterpret_cast<LPCWSTR>(value.utf16()),expanded.data(),required))value=QString::fromWCharArray(expanded.constData());}
    }
    return QFileInfo(value).isFile()?QDir::cleanPath(value):QString{};
}

QString registeredAppPath(const QString &executable) {
    const QString key=QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\")+executable;
    const REGSAM views[]={0,KEY_WOW64_64KEY,KEY_WOW64_32KEY};
    // The application is elevated. Only machine-level App Paths are safe to
    // execute directly; a standard user can replace an HKCU registration.
    for(REGSAM view:views) {const QString path=expandedRegistryString(HKEY_LOCAL_MACHINE,key,view);if(!path.isEmpty())return path;}
    return {};
}

QStringList appPathExecutables(GraphicsVendor vendor) {
    switch(vendor) {
    case GraphicsVendor::Nvidia:return {QStringLiteral("nvcplui.exe")};
    case GraphicsVendor::Intel:return {QStringLiteral("GfxUIEx.exe"),QStringLiteral("GfxUI.exe")};
    case GraphicsVendor::Amd:return {QStringLiteral("RadeonSoftware.exe"),QStringLiteral("RadeonSettings.exe"),QStringLiteral("CCC.exe")};
    default:return {};
    }
}

} // namespace

GraphicsVendor graphicsVendorFromPciVendorId(quint32 vendorId) {
    switch(vendorId) {
    case 0x8086:return GraphicsVendor::Intel;
    case 0x10de:return GraphicsVendor::Nvidia;
    case 0x1002:return GraphicsVendor::Amd;
    default:return GraphicsVendor::Unknown;
    }
}

GraphicsVendor graphicsVendorFromDeviceId(const QString &deviceId) {
    const QString normalized=deviceId.toUpper();
    if(normalized.contains(QStringLiteral("VEN_8086")))return GraphicsVendor::Intel;
    if(normalized.contains(QStringLiteral("VEN_10DE")))return GraphicsVendor::Nvidia;
    if(normalized.contains(QStringLiteral("VEN_1002")))return GraphicsVendor::Amd;
    return GraphicsVendor::Unknown;
}

GraphicsAdapterDetails graphicsAdapterForLuid(qint32 highPart, quint32 lowPart) {
    GraphicsAdapterDetails result;
    IDXGIFactory *factory=nullptr;
    if(FAILED(CreateDXGIFactory(IID_IDXGIFactory,reinterpret_cast<void**>(&factory)))||!factory)return result;
    for(UINT index=0;;++index) {
        IDXGIAdapter *adapter=nullptr;
        const HRESULT enumerated=factory->EnumAdapters(index,&adapter);
        if(enumerated==DXGI_ERROR_NOT_FOUND)break;
        if(FAILED(enumerated)||!adapter)continue;
        DXGI_ADAPTER_DESC description{};
        if(SUCCEEDED(adapter->GetDesc(&description))&&description.AdapterLuid.HighPart==highPart
            &&description.AdapterLuid.LowPart==lowPart) {
            result.pciVendorId=description.VendorId;
            result.vendor=graphicsVendorFromPciVendorId(description.VendorId);
            result.name=QString::fromWCharArray(description.Description).trimmed();
            adapter->Release();break;
        }
        adapter->Release();
    }
    factory->Release();return result;
}

QVector<GpuPanelLaunchCandidate> knownGpuControlPanelCandidates(
    GraphicsVendor vendor,const QString &programFiles,const QString &programFilesX86,
    const QString &windowsDirectory) {
    QVector<GpuPanelLaunchCandidate> result;
    const auto shell=[&](const QString &id){result.push_back({GpuPanelLaunchKind::ShellApplication,id,{}});};
    const auto executable=[&](const QString &path){if(!path.isEmpty())result.push_back({GpuPanelLaunchKind::Executable,path,{}});};
    const auto applet=[&](const QString &path){if(!path.isEmpty())result.push_back({GpuPanelLaunchKind::ControlPanelApplet,path,{}});};
    switch(vendor) {
    case GraphicsVendor::Nvidia:
        shell(QStringLiteral("NVIDIACorp.NVIDIAControlPanel_56jybvy8sckqj!NVIDIACorp.NVIDIAControlPanel"));
        executable(joined(programFiles,QStringLiteral("NVIDIA Corporation/Control Panel Client/nvcplui.exe")));
        executable(joined(programFilesX86,QStringLiteral("NVIDIA Corporation/Control Panel Client/nvcplui.exe")));
        executable(joined(windowsDirectory,QStringLiteral("System32/nvcplui.exe")));
        break;
    case GraphicsVendor::Intel:
        shell(QStringLiteral("AppUp.IntelGraphicsExperience_8j3eq9eme6ctt!App"));
        shell(QStringLiteral("AppUp.IntelGraphicsControlPanel_8j3eq9eme6ctt!App"));
        executable(joined(windowsDirectory,QStringLiteral("System32/GfxUIEx.exe")));
        executable(joined(windowsDirectory,QStringLiteral("System32/GfxUI.exe")));
        applet(joined(windowsDirectory,QStringLiteral("System32/igfxcpl.cpl")));
        break;
    case GraphicsVendor::Amd:
        shell(QStringLiteral("CNEventWindowClass"));
        executable(joined(programFiles,QStringLiteral("AMD/CNext/CNext/RadeonSoftware.exe")));
        executable(joined(programFiles,QStringLiteral("AMD/CNext/CNext/RadeonSettings.exe")));
        executable(joined(programFiles,QStringLiteral("ATI Technologies/ATI.ACE/Core-Static/CCC.exe")));
        executable(joined(programFilesX86,QStringLiteral("ATI Technologies/ATI.ACE/Core-Static/CCC.exe")));
        applet(joined(windowsDirectory,QStringLiteral("System32/ati2cpl.cpl")));
        break;
    default:break;
    }
    return result;
}

bool shellApplicationMatchesVendor(GraphicsVendor vendor,const RegisteredShellApplication &application) {
    const QString id=application.id.toLower(),name=application.name.toLower();
    switch(vendor) {
    case GraphicsVendor::Nvidia:return id.contains(QStringLiteral("nvidiacontrolpanel"))||name.contains(QStringLiteral("nvidia control panel"));
    case GraphicsVendor::Intel:return id.contains(QStringLiteral("intelgraphicsexperience"))||id.contains(QStringLiteral("intelgraphicscontrolpanel"))
        ||name.contains(QStringLiteral("intel graphics command center"))||name.contains(QStringLiteral("intel graphics control panel"));
    case GraphicsVendor::Amd:return id.compare(QStringLiteral("cneventwindowclass"),Qt::CaseInsensitive)==0
        ||id.contains(QStringLiteral("amdradeonsoftware"))||name.contains(QStringLiteral("amd software"))||name.contains(QStringLiteral("radeon settings"));
    default:return false;
    }
}

GpuPanelLaunchCandidate selectGpuControlPanelCandidate(
    const QVector<GpuPanelLaunchCandidate> &candidates,const QStringList &registeredShellApplicationIds,
    const QStringList &existingFiles) {
    QSet<QString> apps,files;
    for(const QString &id:registeredShellApplicationIds)apps.insert(id.toLower());
    for(const QString &file:existingFiles)files.insert(normalizedPath(file));
    for(const GpuPanelLaunchCandidate &candidate:candidates) {
        if(candidate.kind==GpuPanelLaunchKind::ShellApplication&&apps.contains(candidate.target.toLower()))return candidate;
        if((candidate.kind==GpuPanelLaunchKind::Executable||candidate.kind==GpuPanelLaunchKind::ControlPanelApplet)
            &&files.contains(normalizedPath(candidate.target)))return candidate;
    }
    return {};
}

GpuPanelLaunchCandidate resolveGpuControlPanel(GraphicsVendor vendor) {
    if(vendor==GraphicsVendor::Unknown)return {};
    QVector<GpuPanelLaunchCandidate> candidates;
    QStringList registeredIds,existingFiles;
    const QVector<RegisteredShellApplication> applications=registeredShellApplications();
    for(const RegisteredShellApplication &application:applications) {
        registeredIds.push_back(application.id);
        if(shellApplicationMatchesVendor(vendor,application))
            candidates.push_back({GpuPanelLaunchKind::ShellApplication,application.id,{}});
    }
    for(const QString &executable:appPathExecutables(vendor)) {
        const QString path=registeredAppPath(executable);
        if(!path.isEmpty()){candidates.push_back({GpuPanelLaunchKind::Executable,path,{}});existingFiles.push_back(path);}
    }
    const QVector<GpuPanelLaunchCandidate> known=knownGpuControlPanelCandidates(vendor,
        knownFolderPath(FOLDERID_ProgramFiles),knownFolderPath(FOLDERID_ProgramFilesX86),windowsDirectoryPath());
    candidates+=known;
    for(const GpuPanelLaunchCandidate &candidate:candidates)
        if(candidate.kind!=GpuPanelLaunchKind::ShellApplication&&QFileInfo(candidate.target).isFile())existingFiles.push_back(candidate.target);
    return selectGpuControlPanelCandidate(candidates,registeredIds,existingFiles);
}

bool gpuControlPanelAvailable(GraphicsVendor vendor) { return resolveGpuControlPanel(vendor).isValid(); }

ApplyResult launchGpuControlPanel(GraphicsVendor vendor) {
    const GpuPanelLaunchCandidate candidate=resolveGpuControlPanel(vendor);
    if(!candidate.isValid())return ApplyResult::fail(QStringLiteral("No installed GPU control panel could be found for this display adapter."));
    HINSTANCE launched=nullptr;
    if(candidate.kind==GpuPanelLaunchKind::ShellApplication) {
        const QString target=QStringLiteral("shell:AppsFolder\\")+candidate.target;
        launched=ShellExecuteW(nullptr,L"open",L"explorer.exe",reinterpret_cast<LPCWSTR>(target.utf16()),nullptr,SW_SHOWNORMAL);
    } else if(candidate.kind==GpuPanelLaunchKind::ControlPanelApplet) {
        const QString arguments=QLatin1Char('"')+candidate.target+QLatin1Char('"');
        const QString control=joined(windowsDirectoryPath(),QStringLiteral("System32/control.exe"));
        launched=ShellExecuteW(nullptr,L"open",reinterpret_cast<LPCWSTR>(control.utf16()),reinterpret_cast<LPCWSTR>(arguments.utf16()),nullptr,SW_SHOWNORMAL);
    } else {
        const QString working=QFileInfo(candidate.target).absolutePath();
        launched=ShellExecuteW(nullptr,L"open",reinterpret_cast<LPCWSTR>(candidate.target.utf16()),
            candidate.arguments.isEmpty()?nullptr:reinterpret_cast<LPCWSTR>(candidate.arguments.utf16()),
            reinterpret_cast<LPCWSTR>(working.utf16()),SW_SHOWNORMAL);
    }
    const INT_PTR code=reinterpret_cast<INT_PTR>(launched);
    return code>32?ApplyResult::ok():ApplyResult::fail(
        QStringLiteral("Windows could not open the GPU control panel (ShellExecute error %1).").arg(code));
}

} // namespace eink::windows
