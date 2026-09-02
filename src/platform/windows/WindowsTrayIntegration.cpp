#include "WindowsTrayIntegration.h"

#include <QCoreApplication>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QSaveFile>
#include <QScreen>
#include <QSet>
#include <QStandardPaths>
#include <QStringList>
#include <algorithm>
#include <atomic>
#include <cwchar>
#include <thread>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <winternl.h>

namespace eink::windows {
namespace {

struct TrayToolbarData {
    HWND ownerWindow;
    UINT iconId;
    UINT callbackMessage;
    DWORD reserved[2];
    HICON icon;
};

struct TrayToolbarItem {
    HWND toolbar = nullptr;
    HWND taskbar = nullptr;
    TBBUTTON button{};
    TrayToolbarData data{};
    int index = -1;
    bool overflow = false;
};

enum class RegistryEntryState { Missing, Unconfigured, Hidden, Promoted, Error };

constexpr DWORD kTrayPreferenceShowAlways=2;
constexpr DWORD kClassicIconStreamsHeaderSize=20;
constexpr DWORD kClassicIconStreamsRecordSize=1640;
constexpr DWORD kClassicIconStreamsPreferenceOffset=528;

struct LegacyNotifyItem {
    PWSTR executable;
    PWSTR tip;
    HICON icon;
    HWND window;
    DWORD preference;
    UINT id;
    GUID guid;
};

struct LegacyNotificationCallback : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Notify(ULONG event,LegacyNotifyItem *item)=0;
};

struct LegacyTrayNotify : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE RegisterCallback(LegacyNotificationCallback *callback)=0;
    virtual HRESULT STDMETHODCALLTYPE SetPreference(const LegacyNotifyItem *item)=0;
    virtual HRESULT STDMETHODCALLTYPE EnableAutoTray(BOOL enabled)=0;
};

struct Windows8TrayNotify : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE RegisterCallback(LegacyNotificationCallback *callback,ULONG *cookie)=0;
    virtual HRESULT STDMETHODCALLTYPE UnregisterCallback(ULONG *cookie)=0;
    virtual HRESULT STDMETHODCALLTYPE SetPreference(const LegacyNotifyItem *item)=0;
    virtual HRESULT STDMETHODCALLTYPE EnableAutoTray(BOOL enabled)=0;
    virtual HRESULT STDMETHODCALLTYPE DoAction(BOOL enabled)=0;
};

const CLSID kTrayNotifyClsid={0x25dead04,0x1eac,0x4911,{0x9e,0x3a,0xad,0x0a,0x4a,0xb5,0x60,0xfd}};
const IID kLegacyTrayNotifyIid={0xfb852b2c,0x6bad,0x4605,{0x95,0x51,0xf1,0x5f,0x87,0x83,0x09,0x35}};
const IID kWindows8TrayNotifyIid={0xd133ce13,0x3537,0x48ba,{0x93,0xa7,0xaf,0xcd,0x5d,0x20,0x53,0xb4}};
const IID kLegacyNotificationCallbackIid={0xd782ccba,0xafb0,0x43f1,{0x94,0xdb,0xfd,0xa3,0x77,0x9e,0xac,0xcb}};

quint32 currentWindowsBuild() {
    using RtlGetVersionFn = LONG (WINAPI *)(PRTL_OSVERSIONINFOW);
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    const auto fn = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
    RTL_OSVERSIONINFOW version{};
    version.dwOSVersionInfoSize = sizeof(version);
    return fn && fn(&version) == 0 ? version.dwBuildNumber : 0;
}

QVector<HWND> windowsOfClass(const wchar_t *className) {
    struct Context { const wchar_t *name; QVector<HWND> result; } context{className,{}};
    EnumWindows([](HWND window, LPARAM value) -> BOOL {
        auto *context = reinterpret_cast<Context *>(value);
        wchar_t current[64]{};
        GetClassNameW(window,current,static_cast<int>(std::size(current)));
        if(std::wcscmp(current,context->name)==0)context->result.push_back(window);
        return TRUE;
    },reinterpret_cast<LPARAM>(&context));
    return context.result;
}

QVector<HWND> descendantToolbars(HWND root) {
    QVector<HWND> result;
    if(!root)return result;
    EnumChildWindows(root,[](HWND window,LPARAM value)->BOOL {
        wchar_t className[64]{};
        GetClassNameW(window,className,static_cast<int>(std::size(className)));
        if(std::wcscmp(className,L"ToolbarWindow32")==0)
            reinterpret_cast<QVector<HWND> *>(value)->push_back(window);
        return TRUE;
    },reinterpret_cast<LPARAM>(&result));
    return result;
}

QVector<QPair<HWND,HWND>> mainTrayToolbars() {
    QVector<QPair<HWND,HWND>> result;
    QVector<HWND> taskbars=windowsOfClass(L"Shell_TrayWnd");
    taskbars.append(windowsOfClass(L"Shell_SecondaryTrayWnd"));
    for(HWND taskbar:taskbars) {
        const HWND notifyArea=FindWindowExW(taskbar,nullptr,L"TrayNotifyWnd",nullptr);
        for(HWND toolbar:descendantToolbars(notifyArea))result.push_back(qMakePair(toolbar,taskbar));
    }
    return result;
}

bool readOwnItem(HWND toolbar,HWND taskbar,bool overflow,TrayToolbarItem *found) {
    if(!toolbar||!found)return false;
    DWORD explorerPid=0;
    GetWindowThreadProcessId(toolbar,&explorerPid);
    if(!explorerPid)return false;
    HANDLE explorer=OpenProcess(PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE,FALSE,explorerPid);
    if(!explorer)return false;
    constexpr SIZE_T bufferSize=std::max(sizeof(TBBUTTON),sizeof(RECT));
    void *remote=VirtualAllocEx(explorer,nullptr,bufferSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!remote){CloseHandle(explorer);return false;}

    bool matched=false;
    const LRESULT count=SendMessageW(toolbar,TB_BUTTONCOUNT,0,0);
    for(LRESULT index=0;index<count;++index) {
        TBBUTTON button{};
        SIZE_T read=0;
        if(!SendMessageW(toolbar,TB_GETBUTTON,static_cast<WPARAM>(index),reinterpret_cast<LPARAM>(remote)))continue;
        if(!ReadProcessMemory(explorer,remote,&button,sizeof(button),&read)||read!=sizeof(button)||!button.dwData)continue;
        TrayToolbarData data{};
        if(!ReadProcessMemory(explorer,reinterpret_cast<const void *>(button.dwData),&data,sizeof(data),&read)||read!=sizeof(data))continue;
        DWORD ownerPid=0;
        if(data.ownerWindow)GetWindowThreadProcessId(data.ownerWindow,&ownerPid);
        if(ownerPid!=GetCurrentProcessId())continue;
        found->toolbar=toolbar;
        found->taskbar=taskbar;
        found->button=button;
        found->data=data;
        found->index=static_cast<int>(index);
        found->overflow=overflow;
        matched=true;
        break;
    }

    VirtualFreeEx(explorer,remote,0,MEM_RELEASE);
    CloseHandle(explorer);
    return matched;
}

QVector<TrayToolbarItem> ownTrayItems() {
    QVector<TrayToolbarItem> result;
    for(const auto &entry:mainTrayToolbars()) {
        TrayToolbarItem item;
        if(readOwnItem(entry.first,entry.second,false,&item))result.push_back(item);
    }
    for(HWND overflowWindow:windowsOfClass(L"NotifyIconOverflowWindow")) {
        for(HWND toolbar:descendantToolbars(overflowWindow)) {
            TrayToolbarItem item;
            if(readOwnItem(toolbar,nullptr,true,&item))result.push_back(item);
        }
    }
    return result;
}

QScreen *screenForTaskbar(HWND taskbar,MONITORINFOEXW *monitorInfo=nullptr) {
    MONITORINFOEXW monitor{};
    monitor.cbSize=sizeof(monitor);
    if(!GetMonitorInfoW(MonitorFromWindow(taskbar,MONITOR_DEFAULTTOPRIMARY),&monitor))
        return QGuiApplication::primaryScreen();
    if(monitorInfo)*monitorInfo=monitor;
    const QString device=QString::fromWCharArray(monitor.szDevice);
    for(QScreen *screen:QGuiApplication::screens())
        if(QString::compare(screen->name(),device,Qt::CaseInsensitive)==0
            ||device.endsWith(screen->name(),Qt::CaseInsensitive))return screen;
    return QGuiApplication::primaryScreen();
}

QRect itemRect(const TrayToolbarItem &item) {
    if(!item.toolbar||!item.taskbar||item.index<0||(item.button.fsState&TBSTATE_HIDDEN))return {};
    DWORD explorerPid=0;
    GetWindowThreadProcessId(item.toolbar,&explorerPid);
    HANDLE explorer=explorerPid?OpenProcess(PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE,FALSE,explorerPid):nullptr;
    if(!explorer)return {};
    void *remote=VirtualAllocEx(explorer,nullptr,sizeof(RECT),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!remote){CloseHandle(explorer);return {};}
    RECT nativeItem{};
    SIZE_T read=0;
    const bool received=SendMessageW(item.toolbar,TB_GETITEMRECT,static_cast<WPARAM>(item.index),reinterpret_cast<LPARAM>(remote))
        &&ReadProcessMemory(explorer,remote,&nativeItem,sizeof(nativeItem),&read)&&read==sizeof(nativeItem);
    VirtualFreeEx(explorer,remote,0,MEM_RELEASE);
    CloseHandle(explorer);
    if(!received)return {};

    RECT client{};
    GetClientRect(item.toolbar,&client);
    POINT origin{0,0};
    ClientToScreen(item.toolbar,&origin);
    const double toolbarScale=(nativeItem.bottom>client.bottom&&nativeItem.bottom>0)
        ?static_cast<double>(client.bottom)/nativeItem.bottom:1.0;
    QRect result(QPoint(origin.x+qRound(nativeItem.left*toolbarScale),origin.y+qRound(nativeItem.top*toolbarScale)),
                 QPoint(origin.x+qRound(nativeItem.right*toolbarScale)-1,origin.y+qRound(nativeItem.bottom*toolbarScale)-1));
    RECT toolbarRect{};
    GetWindowRect(item.toolbar,&toolbarRect);
    const QRect toolbarBounds(QPoint(toolbarRect.left,toolbarRect.top),QPoint(toolbarRect.right-1,toolbarRect.bottom-1));
    if(!result.isValid()||!toolbarBounds.intersects(result)||result.width()>80||result.height()>80)return {};

    MONITORINFOEXW monitor{};
    QScreen *screen=screenForTaskbar(item.taskbar,&monitor);
    if(!screen)return result;
    const QRect logical=screen->geometry();
    const qreal scaleX=static_cast<qreal>(monitor.rcMonitor.right-monitor.rcMonitor.left)/logical.width();
    const qreal scaleY=static_cast<qreal>(monitor.rcMonitor.bottom-monitor.rcMonitor.top)/logical.height();
    return QRect(logical.left()+qRound((result.x()-monitor.rcMonitor.left)/scaleX),
                 logical.top()+qRound((result.y()-monitor.rcMonitor.top)/scaleY),
                 qRound(result.width()/scaleX),qRound(result.height()/scaleY));
}

QString currentExecutablePath() {
    return QDir::cleanPath(QCoreApplication::applicationFilePath());
}

bool registryExecutablePath(HKEY subkey,QString *path) {
    wchar_t registeredPath[32768]{};
    DWORD bytes=sizeof(registeredPath);
    if(RegGetValueW(subkey,nullptr,L"ExecutablePath",RRF_RT_REG_SZ,nullptr,registeredPath,&bytes)!=ERROR_SUCCESS)
        return false;
    if(path)*path=QDir::cleanPath(QString::fromWCharArray(registeredPath));
    return true;
}

void touchRegistryKey(HKEY key) {
    // Explorer observes last-write notifications for these opaque per-icon
    // keys. Touching a disposable value makes an IsPromoted update visible to
    // shells that do not refresh after that value alone changes.
    constexpr wchar_t kRefreshValue[]=L"EinkAssistant.Refresh";
    const DWORD marker=1;
    if(RegSetValueExW(key,kRefreshValue,0,REG_DWORD,
        reinterpret_cast<const BYTE *>(&marker),sizeof(marker))==ERROR_SUCCESS)
        RegDeleteValueW(key,kRefreshValue);
}

RegistryEntryState inspectOwnRegistryEntry(HKEY root,bool promoteIfUnconfigured,
                                           const QSet<QString> *baseline=nullptr) {
    const QString executable=currentExecutablePath();
    RegistryEntryState state=RegistryEntryState::Missing;
    QStringList newOrphans;
    for(DWORD index=0;;++index) {
        wchar_t subkeyName[256]{};
        DWORD nameLength=static_cast<DWORD>(std::size(subkeyName));
        const LONG enumerated=RegEnumKeyExW(root,index,subkeyName,&nameLength,nullptr,nullptr,nullptr,nullptr);
        if(enumerated==ERROR_NO_MORE_ITEMS)break;
        if(enumerated!=ERROR_SUCCESS)continue;
        HKEY subkey=nullptr;
        if(RegOpenKeyExW(root,subkeyName,0,KEY_QUERY_VALUE|KEY_SET_VALUE,&subkey)!=ERROR_SUCCESS)continue;
        QString registered;
        const bool hasExecutable=registryExecutablePath(subkey,&registered);
        if(hasExecutable
            &&QString::compare(registered,executable,Qt::CaseInsensitive)==0) {
            DWORD promoted=0;
            DWORD bytes=sizeof(promoted);
            const LONG queried=RegGetValueW(subkey,nullptr,L"IsPromoted",RRF_RT_REG_DWORD,nullptr,&promoted,&bytes);
            if(queried==ERROR_FILE_NOT_FOUND) {
                state=RegistryEntryState::Unconfigured;
                if(promoteIfUnconfigured) {
                    promoted=1;
                    if(RegSetValueExW(subkey,L"IsPromoted",0,REG_DWORD,
                        reinterpret_cast<const BYTE *>(&promoted),sizeof(promoted))==ERROR_SUCCESS) {
                        touchRegistryKey(subkey);
                        state=RegistryEntryState::Promoted;
                    } else state=RegistryEntryState::Error;
                }
            } else if(queried==ERROR_SUCCESS) {
                state=promoted==1?RegistryEntryState::Promoted:RegistryEntryState::Hidden;
            } else state=RegistryEntryState::Error;
            RegCloseKey(subkey);
            return state;
        }
        if(!hasExecutable&&promoteIfUnconfigured&&baseline
            &&!baseline->contains(QString::fromWCharArray(subkeyName))) {
            DWORD type=0,size=0;
            if(RegQueryValueExW(subkey,L"IconSnapshot",nullptr,&type,nullptr,&size)==ERROR_SUCCESS
                &&type==REG_BINARY&&size>0)
                newOrphans.push_back(QString::fromWCharArray(subkeyName));
        }
        RegCloseKey(subkey);
    }
    // Explorer can retain an old identity in memory after its registry record
    // is removed. Its next NIM_ADD then creates only an IconSnapshot. Claim a
    // sole new orphan captured after our pre-registration baseline; multiple
    // candidates are ambiguous and intentionally left untouched.
    if(promoteIfUnconfigured&&newOrphans.size()==1) {
        HKEY orphan=nullptr;
        const QString &name=newOrphans.front();
        if(RegOpenKeyExW(root,reinterpret_cast<const wchar_t *>(name.utf16()),0,
            KEY_QUERY_VALUE|KEY_SET_VALUE,&orphan)!=ERROR_SUCCESS)return RegistryEntryState::Error;
        DWORD promoted=0,bytes=sizeof(promoted);
        const LONG queried=RegGetValueW(orphan,nullptr,L"IsPromoted",RRF_RT_REG_DWORD,
            nullptr,&promoted,&bytes);
        if(queried==ERROR_SUCCESS&&promoted==0)state=RegistryEntryState::Hidden;
        else {
            const QString tooltip=QStringLiteral("E-Ink Assistant");
            const LONG pathWritten=RegSetValueExW(orphan,L"ExecutablePath",0,REG_SZ,
                reinterpret_cast<const BYTE *>(executable.utf16()),
                static_cast<DWORD>((executable.size()+1)*sizeof(wchar_t)));
            const LONG tooltipWritten=RegSetValueExW(orphan,L"InitialTooltip",0,REG_SZ,
                reinterpret_cast<const BYTE *>(tooltip.utf16()),
                static_cast<DWORD>((tooltip.size()+1)*sizeof(wchar_t)));
            promoted=1;
            const LONG promotedWritten=RegSetValueExW(orphan,L"IsPromoted",0,REG_DWORD,
                reinterpret_cast<const BYTE *>(&promoted),sizeof(promoted));
            if(pathWritten==ERROR_SUCCESS&&tooltipWritten==ERROR_SUCCESS
                &&promotedWritten==ERROR_SUCCESS) {
                touchRegistryKey(orphan);
                state=RegistryEntryState::Promoted;
            } else state=RegistryEntryState::Error;
        }
        RegCloseKey(orphan);
    }
    return state;
}

QSet<QString> registrySubkeyBaseline(HKEY root) {
    QSet<QString> result;
    for(DWORD index=0;;++index) {
        wchar_t subkeyName[256]{};
        DWORD nameLength=static_cast<DWORD>(std::size(subkeyName));
        const LONG enumerated=RegEnumKeyExW(root,index,subkeyName,&nameLength,nullptr,nullptr,nullptr,nullptr);
        if(enumerated==ERROR_NO_MORE_ITEMS)break;
        if(enumerated==ERROR_SUCCESS)result.insert(QString::fromWCharArray(subkeyName));
    }
    return result;
}

bool cleanOwnRegistryEntries(HKEY root) {
    const QString executable=currentExecutablePath();
    const QString executableName=QFileInfo(executable).fileName();
    QStringList staleMatches;
    bool cleaned=true;
    for(DWORD index=0;;++index) {
        wchar_t subkeyName[256]{};
        DWORD nameLength=static_cast<DWORD>(std::size(subkeyName));
        const LONG enumerated=RegEnumKeyExW(root,index,subkeyName,&nameLength,nullptr,nullptr,nullptr,nullptr);
        if(enumerated==ERROR_NO_MORE_ITEMS)break;
        if(enumerated!=ERROR_SUCCESS)continue;
        HKEY subkey=nullptr;
        if(RegOpenKeyExW(root,subkeyName,0,KEY_QUERY_VALUE,&subkey)!=ERROR_SUCCESS)continue;
        QString registered;
        if(registryExecutablePath(subkey,&registered)) {
            if(QString::compare(registered,executable,Qt::CaseInsensitive)!=0
                &&QString::compare(QFileInfo(registered).fileName(),executableName,Qt::CaseInsensitive)==0) {
                staleMatches.push_back(QString::fromWCharArray(subkeyName));
            }
        }
        RegCloseKey(subkey);
    }
    for(const QString &name:staleMatches)
        cleaned=RegDeleteKeyW(root,reinterpret_cast<const wchar_t *>(name.utf16()))==ERROR_SUCCESS&&cleaned;
    return cleaned;
}

TrayPromotionResult promoteWithWindows11Registry() {
    HKEY root=nullptr;
    if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Control Panel\\NotifyIconSettings",0,KEY_READ,&root)!=ERROR_SUCCESS)
        return TrayPromotionResult::Unavailable;
    const RegistryEntryState state=inspectOwnRegistryEntry(root,false);
    RegCloseKey(root);
    if(state==RegistryEntryState::Promoted)return TrayPromotionResult::Promoted;
    if(state==RegistryEntryState::Hidden)return TrayPromotionResult::Overflow;
    if(state==RegistryEntryState::Missing||state==RegistryEntryState::Unconfigured)return TrayPromotionResult::Pending;
    return TrayPromotionResult::Unavailable;
}

class LegacyPromotionCallback final : public LegacyNotificationCallback {
public:
    explicit LegacyPromotionCallback(LegacyTrayNotify *legacy):m_legacy(legacy) {}
    explicit LegacyPromotionCallback(Windows8TrayNotify *windows8):m_windows8(windows8) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void **object) override {
        if(!object)return E_POINTER;
        *object=nullptr;
        if(IsEqualIID(iid,IID_IUnknown)||IsEqualIID(iid,kLegacyNotificationCallbackIid))
            *object=static_cast<LegacyNotificationCallback *>(this);
        if(!*object)return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return 2; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE Notify(ULONG,LegacyNotifyItem *item) override {
        if(!item)return S_OK;
        DWORD ownerPid=0;
        if(item->window)GetWindowThreadProcessId(item->window,&ownerPid);
        if(ownerPid!=GetCurrentProcessId())return S_OK;
        matched=true;
        if(item->preference==kTrayPreferenceShowAlways) {
            promoted=true;
            return S_OK;
        }
        if(item->preference!=0)return S_OK; // Preserve an explicit user-hidden choice.
        LegacyNotifyItem updated=*item;
        updated.preference=kTrayPreferenceShowAlways;
        const HRESULT result=m_legacy?m_legacy->SetPreference(&updated):m_windows8->SetPreference(&updated);
        promoted=SUCCEEDED(result);
        return S_OK;
    }

    bool matched=false;
    bool promoted=false;

private:
    LegacyTrayNotify *m_legacy=nullptr;
    Windows8TrayNotify *m_windows8=nullptr;
};

TrayPromotionResult promoteWithLegacyCom(quint32 build) {
    const HRESULT initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    const bool uninitialize=initialized==S_OK||initialized==S_FALSE;
    if(FAILED(initialized)&&initialized!=RPC_E_CHANGED_MODE)return TrayPromotionResult::Unavailable;
    TrayPromotionResult result=TrayPromotionResult::Unavailable;
    if(build<9200) {
        LegacyTrayNotify *trayNotify=nullptr;
        const HRESULT created=CoCreateInstance(kTrayNotifyClsid,nullptr,CLSCTX_LOCAL_SERVER,
            kLegacyTrayNotifyIid,reinterpret_cast<void **>(&trayNotify));
        if(SUCCEEDED(created)&&trayNotify) {
            LegacyPromotionCallback callback(trayNotify);
            const HRESULT registered=trayNotify->RegisterCallback(&callback);
            trayNotify->RegisterCallback(nullptr);
            result=FAILED(registered)?TrayPromotionResult::Unavailable
                :callback.promoted?TrayPromotionResult::Promoted
                :callback.matched?TrayPromotionResult::Overflow:TrayPromotionResult::Pending;
            trayNotify->Release();
        }
    } else {
        Windows8TrayNotify *trayNotify=nullptr;
        const HRESULT created=CoCreateInstance(kTrayNotifyClsid,nullptr,CLSCTX_LOCAL_SERVER,
            kWindows8TrayNotifyIid,reinterpret_cast<void **>(&trayNotify));
        if(SUCCEEDED(created)&&trayNotify) {
            LegacyPromotionCallback callback(trayNotify);
            ULONG cookie=0;
            const HRESULT registered=trayNotify->RegisterCallback(&callback,&cookie);
            trayNotify->UnregisterCallback(&cookie);
            result=FAILED(registered)?TrayPromotionResult::Unavailable
                :callback.promoted?TrayPromotionResult::Promoted
                :callback.matched?TrayPromotionResult::Overflow:TrayPromotionResult::Pending;
            trayNotify->Release();
        }
    }
    if(uninitialize)CoUninitialize();
    return result;
}

ushort rot13Ascii(ushort value) {
    if(value>='A'&&value<='Z')return static_cast<ushort>((value-'A'+13)%26+'A');
    if(value>='a'&&value<='z')return static_cast<ushort>((value-'a'+13)%26+'a');
    return value;
}

QString classicIconStreamsPath(const QByteArray &data,int recordOffset) {
    QVector<ushort> characters;
    characters.reserve(260);
    for(int offset=recordOffset;offset+1<recordOffset+static_cast<int>(kClassicIconStreamsPreferenceOffset);offset+=2) {
        const ushort encoded=static_cast<ushort>(static_cast<uchar>(data[offset]))
            |static_cast<ushort>(static_cast<uchar>(data[offset+1])<<8);
        if(encoded==0)break;
        characters.push_back(rot13Ascii(encoded));
    }
    if(characters.isEmpty())return {};
    return QDir::cleanPath(QString::fromUtf16(characters.constData(),characters.size()));
}

bool saveClassicIconStreamsBackup(const QByteArray &data) {
    const QString directory=QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if(!QDir().mkpath(directory))return false;
    const QString path=QDir(directory).filePath(QStringLiteral("tray-iconstreams-backup.bin"));
    if(QFileInfo::exists(path))return true;
    QSaveFile backup(path);
    if(!backup.open(QIODevice::WriteOnly)||backup.write(data)!=data.size())return false;
    return backup.commit();
}

TrayPromotionResult promoteWithClassicIconStreams() {
    const QVector<TrayToolbarItem> currentItems=ownTrayItems();
    for(const TrayToolbarItem &item:currentItems)
        if(!item.overflow&&!(item.button.fsState&TBSTATE_HIDDEN))return TrayPromotionResult::Promoted;

    HKEY key=nullptr;
    if(RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\TrayNotify",
        0,KEY_QUERY_VALUE|KEY_SET_VALUE,&key)!=ERROR_SUCCESS)return TrayPromotionResult::Unavailable;
    DWORD type=0,size=0;
    LONG status=RegQueryValueExW(key,L"IconStreams",nullptr,&type,nullptr,&size);
    if(status!=ERROR_SUCCESS||type!=REG_BINARY||size<kClassicIconStreamsHeaderSize) {
        RegCloseKey(key);
        return TrayPromotionResult::Unavailable;
    }
    QByteArray data(static_cast<int>(size),Qt::Uninitialized);
    status=RegQueryValueExW(key,L"IconStreams",nullptr,&type,
        reinterpret_cast<BYTE *>(data.data()),&size);
    if(status!=ERROR_SUCCESS||data.size()<static_cast<int>(kClassicIconStreamsHeaderSize)) {
        RegCloseKey(key);
        return TrayPromotionResult::Unavailable;
    }
    const QByteArray original=data;
    bool matched=false,changed=false;
    if(!patchClassicIconStreamsBlobForExecutable(&data,currentExecutablePath(),&matched,&changed)) {
        RegCloseKey(key);
        return TrayPromotionResult::Unavailable;
    }
    if(!matched) {
        RegCloseKey(key);
        return TrayPromotionResult::Pending;
    }
    if(changed) {
        if(!saveClassicIconStreamsBackup(original)) {
            RegCloseKey(key);
            return TrayPromotionResult::Unavailable;
        }
        status=RegSetValueExW(key,L"IconStreams",0,REG_BINARY,
            reinterpret_cast<const BYTE *>(data.constData()),static_cast<DWORD>(data.size()));
        if(status!=ERROR_SUCCESS) {
            RegCloseKey(key);
            return TrayPromotionResult::Unavailable;
        }
    }
    RegCloseKey(key);
    return std::any_of(currentItems.cbegin(),currentItems.cend(),
        [](const TrayToolbarItem &item){return item.overflow;})
        ?TrayPromotionResult::Overflow:TrayPromotionResult::Pending;
}

TrayPromotionResult promoteWithLegacyShellState() {
    const QVector<TrayToolbarItem> items=ownTrayItems();
    if(items.isEmpty())return TrayPromotionResult::Pending;
    bool foundOverflow=false;
    for(const TrayToolbarItem &item:items) {
        if(!item.overflow&&!(item.button.fsState&TBSTATE_HIDDEN))return TrayPromotionResult::Promoted;
        if(item.overflow) {
            foundOverflow=true;
            continue;
        }
        NOTIFYICONDATAW notification{};
        notification.cbSize=sizeof(notification);
        notification.hWnd=item.data.ownerWindow;
        notification.uID=item.data.iconId;
        notification.uFlags=NIF_STATE;
        notification.dwState=0;
        notification.dwStateMask=NIS_HIDDEN;
        if(Shell_NotifyIconW(NIM_MODIFY,&notification)) {
            if(!item.overflow&&item.toolbar)
                SendMessageW(item.toolbar,TB_HIDEBUTTON,static_cast<WPARAM>(item.button.idCommand),FALSE);
            return TrayPromotionResult::Pending;
        }
    }
    return foundOverflow?TrayPromotionResult::Overflow:TrayPromotionResult::Unavailable;
}

} // namespace

bool patchClassicIconStreamsBlobForExecutable(QByteArray *data,const QString &executable,
                                               bool *matched,bool *changed) {
    if(matched)*matched=false;
    if(changed)*changed=false;
    if(!data||data->size()<static_cast<int>(kClassicIconStreamsHeaderSize))return false;
    auto dwordAt=[data](int offset) {
        return static_cast<quint32>(static_cast<uchar>((*data)[offset]))
            |(static_cast<quint32>(static_cast<uchar>((*data)[offset+1]))<<8)
            |(static_cast<quint32>(static_cast<uchar>((*data)[offset+2]))<<16)
            |(static_cast<quint32>(static_cast<uchar>((*data)[offset+3]))<<24);
    };
    const quint32 headerSize=dwordAt(0);
    const quint32 count=dwordAt(12);
    const quint64 expected=static_cast<quint64>(kClassicIconStreamsHeaderSize)
        +static_cast<quint64>(count)*kClassicIconStreamsRecordSize;
    if(headerSize!=kClassicIconStreamsHeaderSize||expected!=static_cast<quint64>(data->size()))return false;
    const QString normalized=QDir::cleanPath(executable);
    int matchedOffset=-1;
    for(quint32 index=0;index<count;++index) {
        const int record=static_cast<int>(kClassicIconStreamsHeaderSize+index*kClassicIconStreamsRecordSize);
        if(QString::compare(classicIconStreamsPath(*data,record),normalized,Qt::CaseInsensitive)!=0)continue;
        if(matchedOffset!=-1)return false; // Ambiguous identity: do not mutate either entry.
        matchedOffset=record;
    }
    if(matchedOffset<0)return true;
    if(matched)*matched=true;
    const int preferenceOffset=matchedOffset+static_cast<int>(kClassicIconStreamsPreferenceOffset);
    if(static_cast<uchar>((*data)[preferenceOffset])==kTrayPreferenceShowAlways)return true;
    (*data)[preferenceOffset]=static_cast<char>(kTrayPreferenceShowAlways);
    if(changed)*changed=true;
    return true;
}

struct TrayPromotionWatcher::Impl {
    HKEY root=nullptr;
    HANDLE stopEvent=nullptr;
    HANDLE changeEvent=nullptr;
    std::thread worker;
    QSet<QString> baseline;
    std::atomic<TrayPromotionResult> observed{TrayPromotionResult::Unavailable};
};

TrayPromotionWatcher::TrayPromotionWatcher():m_impl(std::make_unique<Impl>()) {}

TrayPromotionWatcher::~TrayPromotionWatcher() {
    stop();
}

bool TrayPromotionWatcher::arm(bool cleanStaleEntries) {
    stop();
    m_impl=std::make_unique<Impl>();
    if(trayPromotionStrategyForBuild(currentWindowsBuild())!=TrayPromotionStrategy::PerIconRegistry)
        return false;
    if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Control Panel\\NotifyIconSettings",0,
        KEY_READ|KEY_WRITE|KEY_NOTIFY,&m_impl->root)!=ERROR_SUCCESS)return false;
    if(cleanStaleEntries&&!cleanOwnRegistryEntries(m_impl->root)) {
        RegCloseKey(m_impl->root);
        m_impl->root=nullptr;
        return false;
    }
    m_impl->baseline=registrySubkeyBaseline(m_impl->root);
    m_impl->stopEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    m_impl->changeEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if(!m_impl->stopEvent||!m_impl->changeEvent
       ||RegNotifyChangeKeyValue(m_impl->root,TRUE,REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET,
          m_impl->changeEvent,TRUE)!=ERROR_SUCCESS) {
        stop();
        return false;
    }
    Impl *impl=m_impl.get();
    impl->observed.store(TrayPromotionResult::Pending);
    impl->worker=std::thread([impl]{
        auto inspect=[impl]{
            const RegistryEntryState state=inspectOwnRegistryEntry(impl->root,true,&impl->baseline);
            if(state==RegistryEntryState::Promoted)impl->observed.store(TrayPromotionResult::Promoted);
            else if(state==RegistryEntryState::Hidden)impl->observed.store(TrayPromotionResult::Overflow);
            else if(state==RegistryEntryState::Error)impl->observed.store(TrayPromotionResult::Unavailable);
            else impl->observed.store(TrayPromotionResult::Pending);
        };
        inspect();
        while(impl->observed.load()!=TrayPromotionResult::Promoted) {
            HANDLE events[2]{impl->stopEvent,impl->changeEvent};
            const DWORD wait=WaitForMultipleObjects(2,events,FALSE,INFINITE);
            if(wait==WAIT_OBJECT_0)break;
            if(wait!=WAIT_OBJECT_0+1) {
                impl->observed.store(TrayPromotionResult::Unavailable);
                break;
            }
            ResetEvent(impl->changeEvent);
            if(RegNotifyChangeKeyValue(impl->root,TRUE,REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET,
                impl->changeEvent,TRUE)!=ERROR_SUCCESS) {
                impl->observed.store(TrayPromotionResult::Unavailable);
                break;
            }
            inspect();
        }
    });
    return true;
}

void TrayPromotionWatcher::stop() {
    if(!m_impl)return;
    if(m_impl->stopEvent)SetEvent(m_impl->stopEvent);
    if(m_impl->worker.joinable())m_impl->worker.join();
    if(m_impl->changeEvent)CloseHandle(m_impl->changeEvent);
    if(m_impl->stopEvent)CloseHandle(m_impl->stopEvent);
    if(m_impl->root)RegCloseKey(m_impl->root);
    m_impl->changeEvent=nullptr;
    m_impl->stopEvent=nullptr;
    m_impl->root=nullptr;
}

TrayPromotionResult TrayPromotionWatcher::result() const {
    return m_impl?m_impl->observed.load():TrayPromotionResult::Unavailable;
}

TrayPromotionStrategy trayPromotionStrategyForBuild(quint32 build) {
    if(build>=22000)return TrayPromotionStrategy::PerIconRegistry;
    if(build>=16299)return TrayPromotionStrategy::ClassicIconStreams;
    return TrayPromotionStrategy::LegacyCom;
}

TrayPromotionResult promoteOwnTrayIcon() {
    const TrayPromotionStrategy strategy=trayPromotionStrategyForBuild(currentWindowsBuild());
    if(strategy==TrayPromotionStrategy::PerIconRegistry) {
        const TrayPromotionResult registry=promoteWithWindows11Registry();
        if(registry==TrayPromotionResult::Promoted)return registry;
        const TrayPromotionResult legacy=promoteWithLegacyShellState();
        if(legacy==TrayPromotionResult::Promoted)return legacy;
        if(registry==TrayPromotionResult::Pending)return registry;
        if(legacy!=TrayPromotionResult::Pending)return legacy;
        return registry==TrayPromotionResult::Pending?registry:legacy;
    }
    if(strategy==TrayPromotionStrategy::LegacyCom) {
        const TrayPromotionResult com=promoteWithLegacyCom(currentWindowsBuild());
        if(com==TrayPromotionResult::Promoted||com==TrayPromotionResult::Overflow)return com;
        const TrayPromotionResult shell=promoteWithLegacyShellState();
        if(shell==TrayPromotionResult::Promoted||shell==TrayPromotionResult::Overflow)return shell;
        const TrayPromotionResult persisted=promoteWithClassicIconStreams();
        if(persisted!=TrayPromotionResult::Unavailable)return persisted;
        return shell;
    }
    const TrayPromotionResult persisted=promoteWithClassicIconStreams();
    if(persisted==TrayPromotionResult::Promoted)return persisted;
    const TrayPromotionResult shell=promoteWithLegacyShellState();
    if(shell==TrayPromotionResult::Promoted)return shell;
    if(persisted==TrayPromotionResult::Overflow)return persisted;
    if(persisted!=TrayPromotionResult::Unavailable)return persisted;
    return shell;
}

QVector<QRect> ownPromotedTrayIconRects() {
    QVector<QRect> result;
    for(const TrayToolbarItem &item:ownTrayItems()) {
        if(item.overflow)continue;
        const QRect rect=itemRect(item);
        if(rect.isValid())result.push_back(rect);
    }
    return result;
}

bool ownTrayIconInOverflow() {
    const QVector<TrayToolbarItem> items=ownTrayItems();
    return std::any_of(items.cbegin(),items.cend(),[](const TrayToolbarItem &item){return item.overflow;});
}

QString trayIntegrationDiagnostic() {
    QStringList lines;
    const QVector<TrayToolbarItem> items=ownTrayItems();
    lines.push_back(QStringLiteral("items=%1 mainToolbars=%2 overflowWindows=%3")
        .arg(items.size()).arg(mainTrayToolbars().size()).arg(windowsOfClass(L"NotifyIconOverflowWindow").size()));
    for(const TrayToolbarItem &item:items) {
        const QRect rect=itemRect(item);
        lines.push_back(QStringLiteral("overflow=%1 hidden=%2 index=%3 command=%4 owner=0x%5 id=%6 rect=%7,%8,%9,%10")
            .arg(item.overflow).arg((item.button.fsState&TBSTATE_HIDDEN)!=0).arg(item.index).arg(item.button.idCommand)
            .arg(reinterpret_cast<quintptr>(item.data.ownerWindow),0,16).arg(item.data.iconId)
            .arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height()));
    }
    return lines.join(QLatin1Char('\n'));
}

} // namespace eink::windows
