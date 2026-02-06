/**
 * @file Exports.cpp
 * @brief Exported DirectDraw functions
 *
 * Implements the DirectDraw API entry points that applications call.
 * These functions create and manage DirectDraw objects.
 */

#include "core/Common.h"
#include "interfaces/DirectDrawImpl.h"

using namespace ldc;
using namespace ldc::interfaces;

// ============================================================================
// Global DirectDraw Instance
// ============================================================================

namespace {
    std::mutex g_ddrawMutex;
}

// ============================================================================
// DirectDrawCreate
// ============================================================================

extern "C" HRESULT WINAPI DirectDrawCreate(
    GUID* lpGUID,
    LPDIRECTDRAW* lplpDD,
    IUnknown* pUnkOuter
) {
    DebugLog("DirectDrawCreate(lpGUID=%p, lplpDD=%p, pUnkOuter=%p)",
             lpGUID, lplpDD, pUnkOuter);

    if (!lplpDD) {
        DebugLog("DirectDrawCreate: lplpDD is NULL");
        return DDERR_INVALIDPARAMS;
    }

    if (pUnkOuter) {
        DebugLog("DirectDrawCreate: Aggregation not supported");
        return DDERR_INVALIDPARAMS;
    }

    std::lock_guard<std::mutex> lock(g_ddrawMutex);

    try {
        auto* pDD = new DirectDrawImpl();
        pDD->SetInterfaceVersion(1);

        *lplpDD = reinterpret_cast<LPDIRECTDRAW>(pDD);

        DebugLog("DirectDrawCreate: Created IDirectDraw at %p", pDD);
        return DD_OK;
    }
    catch (const std::exception& e) {
        DebugLog("DirectDrawCreate: Exception: %s", e.what());
        return DDERR_OUTOFMEMORY;
    }
}

// ============================================================================
// DirectDrawCreateEx
// ============================================================================

extern "C" HRESULT WINAPI DirectDrawCreateEx(
    GUID* lpGuid,
    LPVOID* lplpDD,
    REFIID iid,
    IUnknown* pUnkOuter
) {
    DebugLog("DirectDrawCreateEx(lpGuid=%p, lplpDD=%p, pUnkOuter=%p)",
             lpGuid, lplpDD, pUnkOuter);

    if (!lplpDD) {
        DebugLog("DirectDrawCreateEx: lplpDD is NULL");
        return DDERR_INVALIDPARAMS;
    }

    if (pUnkOuter) {
        DebugLog("DirectDrawCreateEx: Aggregation not supported");
        return DDERR_INVALIDPARAMS;
    }

    std::lock_guard<std::mutex> lock(g_ddrawMutex);

    try {
        auto* pDD = new DirectDrawImpl();

        HRESULT hr = pDD->QueryInterface(iid, lplpDD);
        if (FAILED(hr)) {
            DebugLog("DirectDrawCreateEx: QueryInterface failed: 0x%08X", hr);
            delete pDD;
            return hr;
        }

        pDD->Release();

        DebugLog("DirectDrawCreateEx: Created DirectDraw at %p", *lplpDD);
        return DD_OK;
    }
    catch (const std::exception& e) {
        DebugLog("DirectDrawCreateEx: Exception: %s", e.what());
        return DDERR_OUTOFMEMORY;
    }
}

// ============================================================================
// DirectDrawCreateClipper
// ============================================================================

extern "C" HRESULT WINAPI DirectDrawCreateClipper(
    DWORD dwFlags,
    LPDIRECTDRAWCLIPPER* lplpDDClipper,
    IUnknown* pUnkOuter
) {
    DebugLog("DirectDrawCreateClipper(dwFlags=0x%08X)", dwFlags);

    if (!lplpDDClipper) {
        return DDERR_INVALIDPARAMS;
    }

    if (pUnkOuter) {
        return DDERR_INVALIDPARAMS;
    }

    *lplpDDClipper = nullptr;
    return DDERR_UNSUPPORTED;
}

// ============================================================================
// DirectDrawEnumerate
// ============================================================================

extern "C" HRESULT WINAPI DirectDrawEnumerateA(
    LPDDENUMCALLBACKA lpCallback,
    LPVOID lpContext
) {
    DebugLog("DirectDrawEnumerateA(lpCallback=%p)", lpCallback);

    if (!lpCallback) {
        return DDERR_INVALIDPARAMS;
    }

    // LPDDENUMCALLBACKA expects LPSTR (char*), not const char*
    char driverDesc[] = "Primary Display Driver";
    char driverName[] = "display";
    lpCallback(nullptr, driverDesc, driverName, lpContext);

    return DD_OK;
}

extern "C" HRESULT WINAPI DirectDrawEnumerateW(
    LPDDENUMCALLBACKW lpCallback,
    LPVOID lpContext
) {
    DebugLog("DirectDrawEnumerateW(lpCallback=%p)", lpCallback);

    if (!lpCallback) {
        return DDERR_INVALIDPARAMS;
    }

    wchar_t driverDesc[] = L"Primary Display Driver";
    wchar_t driverName[] = L"display";
    lpCallback(nullptr, driverDesc, driverName, lpContext);

    return DD_OK;
}

extern "C" HRESULT WINAPI DirectDrawEnumerateExA(
    LPDDENUMCALLBACKEXA lpCallback,
    LPVOID lpContext,
    DWORD dwFlags
) {
    DebugLog("DirectDrawEnumerateExA(lpCallback=%p, dwFlags=0x%08X)", lpCallback, dwFlags);

    if (!lpCallback) {
        return DDERR_INVALIDPARAMS;
    }

    HMONITOR hMonitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
    char driverDesc[] = "Primary Display Driver";
    char driverName[] = "display";
    lpCallback(nullptr, driverDesc, driverName, lpContext, hMonitor);

    return DD_OK;
}

extern "C" HRESULT WINAPI DirectDrawEnumerateExW(
    LPDDENUMCALLBACKEXW lpCallback,
    LPVOID lpContext,
    DWORD dwFlags
) {
    DebugLog("DirectDrawEnumerateExW(lpCallback=%p, dwFlags=0x%08X)", lpCallback, dwFlags);

    if (!lpCallback) {
        return DDERR_INVALIDPARAMS;
    }

    HMONITOR hMonitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
    wchar_t driverDesc[] = L"Primary Display Driver";
    wchar_t driverName[] = L"display";
    lpCallback(nullptr, driverDesc, driverName, lpContext, hMonitor);

    return DD_OK;
}

// ============================================================================
// COM Entry Points (Stubs)
// ============================================================================

extern "C" HRESULT WINAPI DllCanUnloadNow() {
    return S_FALSE;
}

extern "C" HRESULT WINAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID* ppv
) {
    LDC_UNUSED(rclsid);
    LDC_UNUSED(riid);

    if (ppv) {
        *ppv = nullptr;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}
