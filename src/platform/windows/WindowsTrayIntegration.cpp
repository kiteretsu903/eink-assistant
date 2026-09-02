#include "WindowsTrayIntegration.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QScreen>
#include <QStringList>
#include <algorithm>
#include <cwchar>

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

TrayPromotionResult promoteWithWindows11Registry() {
    HKEY root=nullptr;
    if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Control Panel\\NotifyIconSettings",0,KEY_READ|KEY_WRITE,&root)!=ERROR_SUCCESS)
        return TrayPromotionResult::Unavailable;
    const QString executable=QDir::cleanPath(QCoreApplication::applicationFilePath());
    const QString executableName=QFileInfo(executable).fileName();
    QStringList staleAppEntries;
    bool matched=false;
    bool written=false;
    for(DWORD index=0;;++index) {
        wchar_t subkeyName[256]{};
        DWORD nameLength=static_cast<DWORD>(std::size(subkeyName));
        const LONG enumerated=RegEnumKeyExW(root,index,subkeyName,&nameLength,nullptr,nullptr,nullptr,nullptr);
        if(enumerated==ERROR_NO_MORE_ITEMS)break;
        if(enumerated!=ERROR_SUCCESS)continue;
        HKEY subkey=nullptr;
        if(RegOpenKeyExW(root,subkeyName,0,KEY_QUERY_VALUE|KEY_SET_VALUE,&subkey)!=ERROR_SUCCESS)continue;
        wchar_t registeredPath[32768]{};
        DWORD bytes=sizeof(registeredPath);
        if(RegGetValueW(subkey,nullptr,L"ExecutablePath",RRF_RT_REG_SZ,nullptr,registeredPath,&bytes)==ERROR_SUCCESS) {
            const QString registered=QDir::cleanPath(QString::fromWCharArray(registeredPath));
            if(QString::compare(registered,executable,Qt::CaseInsensitive)==0) {
                const DWORD promoted=1;
                matched=true;
                written=RegSetValueExW(subkey,L"IsPromoted",0,REG_DWORD,
                    reinterpret_cast<const BYTE *>(&promoted),sizeof(promoted))==ERROR_SUCCESS;
            } else if(QString::compare(QFileInfo(registered).fileName(),executableName,Qt::CaseInsensitive)==0) {
                staleAppEntries.push_back(QString::fromWCharArray(subkeyName));
            }
        }
        RegCloseKey(subkey);
    }
    for(const QString &subkeyName:staleAppEntries)
        RegDeleteKeyW(root,reinterpret_cast<const wchar_t *>(subkeyName.utf16()));
    RegCloseKey(root);
    if(matched)return written?TrayPromotionResult::Promoted:TrayPromotionResult::Unavailable;
    return TrayPromotionResult::Pending;
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

TrayPromotionStrategy trayPromotionStrategyForBuild(quint32 build) {
    return build>=22000?TrayPromotionStrategy::PerIconRegistry:TrayPromotionStrategy::LegacyShellState;
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
    return promoteWithLegacyShellState();
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
