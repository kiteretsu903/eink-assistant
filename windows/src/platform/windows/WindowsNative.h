#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace eink {

constexpr int DisplayConfigGetAdvancedColorInfo2 = 15;
constexpr int DisplayConfigSetHdrState = 16;
constexpr int DisplayConfigSetWcgState = 17;
constexpr int DisplayConfigGetAdvancedColorInfo = 9;
constexpr int DisplayConfigGetColorManagementCaps = -12;

struct DisplayConfigAdvancedColorInfo2 {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    UINT32 value;
    UINT32 colorEncoding;
    UINT32 bitsPerColorChannel;
    UINT32 activeColorMode;
    bool advancedColorSupported() const { return (value & (1u << 0)) != 0; }
    bool advancedColorActive() const { return (value & (1u << 1)) != 0; }
    bool wideColorSupported() const { return (value & (1u << 6)) != 0; }
    bool wideColorEnabled() const { return (value & (1u << 7)) != 0; }
};

struct DisplayConfigSetWcg {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    UINT32 value;
};

struct DisplayConfigAdvancedColorInfo {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    UINT32 value;
    UINT32 colorEncoding;
    UINT32 bitsPerColorChannel;
    bool advancedColorEnabled() const { return (value & (1u << 1)) != 0; }
};

struct DisplayConfigColorManagementCaps {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    INT32 value;
    bool matrixDdiSupported() const { return (value & 1) != 0; }
};

using D3dkmtHandle = UINT;
struct D3dkmtOpenAdapterFromLuid {
    LUID adapterLuid;
    D3dkmtHandle adapter;
};
struct D3dkmtQueryAdapterInfo {
    D3dkmtHandle adapter;
    UINT type;
    void *data;
    UINT dataSize;
};
struct D3dkmtCloseAdapter { D3dkmtHandle adapter; };
using D3dkmtOpenAdapterFromLuidFn = LONG (WINAPI *)(D3dkmtOpenAdapterFromLuid *);
using D3dkmtQueryAdapterInfoFn = LONG (WINAPI *)(D3dkmtQueryAdapterInfo *);
using D3dkmtCloseAdapterFn = LONG (WINAPI *)(D3dkmtCloseAdapter *);

constexpr UINT KmtQueryDriverVersion = 13;
constexpr int Wddm26 = 2600;

enum class WcsDeviceCapabilitiesType : INT {
    VideoCardGammaTable = 1,
    MicrosoftHardwareColorV2 = 2
};

struct WcsDeviceMhc2Capabilities {
    DWORD size;
    BOOL supportsMhc2;
    DWORD regammaLutEntryCount;
    DWORD cscXyzMatrixRows;
    DWORD cscXyzMatrixColumns;
};

using ColorProfileAddDisplayAssociationFn = HRESULT (WINAPI *)(INT, PCWSTR, LUID, UINT32, BOOL, BOOL);
using ColorProfileRemoveDisplayAssociationFn = HRESULT (WINAPI *)(INT, PCWSTR, LUID, UINT32, BOOL);
using ColorProfileSetDisplayDefaultAssociationFn = HRESULT (WINAPI *)(INT, PCWSTR, INT, INT, LUID, UINT32);
using ColorProfileGetDisplayDefaultFn = HRESULT (WINAPI *)(INT, LUID, UINT32, INT, INT, PWSTR *);
using ColorProfileGetDisplayUserScopeFn = HRESULT (WINAPI *)(LUID, UINT32, INT *);
using ColorProfileGetDisplayListFn = HRESULT (WINAPI *)(INT, LUID, UINT32, PWSTR **, PDWORD);
using ColorProfileGetDeviceCapabilitiesFn = HRESULT (WINAPI *)(INT, LUID, UINT32, INT, PVOID);

constexpr int WcsScopeSystemWide = 0;
constexpr int WcsScopeCurrentUser = 1;
constexpr int ColorProfileTypeIcc = 0;
constexpr int ColorProfileSubtypeStandard = 7;
} // namespace eink
