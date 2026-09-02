#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace eink {

constexpr int DisplayConfigGetAdvancedColorInfo2 = 15;
constexpr int DisplayConfigSetHdrState = 16;
constexpr int DisplayConfigSetWcgState = 17;

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
