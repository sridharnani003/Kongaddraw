/**
 * @file Exports.cpp
 * @brief Exported DirectDraw functions
 *
 * Implements the DirectDraw API entry points that applications call.
 * These functions create and manage DirectDraw objects.
 */

#include "core/Common.h"
#include "interfaces/DirectDrawImpl.h"
#include "logging/Logger.h"

using namespace ldc;
using namespace ldc::interfaces;
using namespace ldc::logging;

// ============================================================================
// Global DirectDraw Instance
// ============================================================================

namespace {
    // Single DirectDraw instance (most applications only create one)
    DirectDrawImpl* g_pDirectDraw = nullptr;
    std::mutex g_ddrawMutex;
}

// ============================================================================
// DirectDrawCreate
// ============================================================================

/**
 * @brief Create an IDirectDraw object
 *
 * This is the primary entry point for applications using DirectDraw.
 *
 * @param lpGUID Driver GUID (NULL for default display driver)
 * @param lplpDD Receives the IDirectDraw interface pointer
 * @param pUnkOuter Outer IUnknown for aggregation (must be NULL)
 * @return DD_OK on success, or error code
 */
extern "C" HRESULT WINAPI DirectDrawCreate(
    GUID* lpGUID,
    LPDIRECTDRAW* lplpDD,
    IUnknown* pUnkOuter
) {
    LOG_INFO("DirectDrawCreate(lpGUID=%p, lplpDD=%p, pUnkOuter=%p)",
             lpGUID, lplpDD, pUnkOuter);

    // Validate parameters
    if (!lplpDD) {
        LOG_ERROR("DirectDrawCreate: lplpDD is NULL");
        return DDERR_INVALIDPARAMS;
    }

    // Aggregation not supported
    if (pUnkOuter) {
        LOG_ERROR("DirectDrawCreate: Aggregation not supported");
        return DDERR_INVALIDPARAMS;
    }

    // Create DirectDraw object
    std::lock_guard<std::mutex> lock(g_ddrawMutex);

    try {
        auto* pDD = new DirectDrawImpl();
        pDD->SetInterfaceVersion(1);  // IDirectDraw (version 1)

        // Return IDirectDraw interface
        *lplpDD = reinterpret_cast<LPDIRECTDRAW>(pDD);

        LOG_INFO("DirectDrawCreate: Created IDirectDraw at %p", pDD);
        return DD_OK;
    }
    catch (const std::exception& e) {
        LOG_ERROR("DirectDrawCreate: Exception: %s", e.what());
        return DDERR_OUTOFMEMORY;
    }
}

// ============================================================================
// DirectDrawCreateEx
// ============================================================================

/**
 * @brief Create a DirectDraw object with specified interface
 *
 * Extended version that allows requesting specific interface versions.
 *
 * @param lpGuid Driver GUID (NULL for default)
 * @param lplpDD Receives the interface pointer
 * @param iid Requested interface IID
 * @param pUnkOuter Outer IUnknown for aggregation (must be NULL)
 * @return DD_OK on success, or error code
 */
extern "C" HRESULT WINAPI DirectDrawCreateEx(
    GUID* lpGuid,
    LPVOID* lplpDD,
    REFIID iid,
    IUnknown* pUnkOuter
) {
    LOG_INFO("DirectDrawCreateEx(lpGuid=%p, lplpDD=%p, iid, pUnkOuter=%p)",
             lpGuid, lplpDD, pUnkOuter);

    // Validate parameters
    if (!lplpDD) {
        LOG_ERROR("DirectDrawCreateEx: lplpDD is NULL");
        return DDERR_INVALIDPARAMS;
    }

    // Aggregation not supported
    if (pUnkOuter) {
        LOG_ERROR("DirectDrawCreateEx: Aggregation not supported");
        return DDERR_INVALIDPARAMS;
    }

    // Create DirectDraw object
    std::lock_guard<std::mutex> lock(g_ddrawMutex);

    try {
        auto* pDD = new DirectDrawImpl();

        // Query for requested interface
        HRESULT hr = pDD->QueryInterface(iid, lplpDD);
        if (FAILED(hr)) {
            LOG_ERROR("DirectDrawCreateEx: QueryInterface failed: 0x%08X", hr);
            delete pDD;
            return hr;
        }

        // Release our reference (QueryInterface added one)
        pDD->Release();

        LOG_INFO("DirectDrawCreateEx: Created DirectDraw at %p", *lplpDD);
        return DD_OK;
    }
    catch (const std::exception& e) {
        LOG_ERROR("DirectDrawCreateEx: Exception: %s", e.what());
        return DDERR_OUTOFMEMORY;
    }
}

// ============================================================================
// DirectDrawCreateClipper
// ============================================================================

/**
 * @brief Create a standalone clipper object
 *
 * @param dwFlags Creation flags (must be 0)
 * @param lplpDDClipper Receives the clipper interface
 * @param pUnkOuter Outer IUnknown for aggregation (must be NULL)
 * @return DD_OK on success, or error code
 */
extern "C" HRESULT WINAPI DirectDrawCreateClipper(
    DWORD dwFlags,
    LPDIRECTDRAWCLIPPER* lplpDDClipper,
    IUnknown* pUnkOuter
) {
    LOG_INFO("DirectDrawCreateClipper(dwFlags=0x%08X, lplpDDClipper=%p, pUnkOuter=%p)",
             dwFlags, lplpDDClipper, pUnkOuter);

    // Validate parameters
    if (!lplpDDClipper) {
        LOG_ERROR("DirectDrawCreateClipper: lplpDDClipper is NULL");
        return DDERR_INVALIDPARAMS;
    }

    if (pUnkOuter) {
        LOG_ERROR("DirectDrawCreateClipper: Aggregation not supported");
        return DDERR_INVALIDPARAMS;
    }

    // TODO: Implement clipper creation
    LOG_WARN("DirectDrawCreateClipper: Clipper creation not yet implemented");
    *lplpDDClipper = nullptr;
    return DDERR_UNSUPPORTED;
}

// ============================================================================
// DirectDrawEnumerate
// ============================================================================

/**
 * @brief Enumerate DirectDraw drivers (ANSI)
 *
 * @param lpCallback Callback function
 * @param lpContext User-defined context passed to callback
 * @return DD_OK on success
 */
extern "C" HRESULT WINAPI DirectDrawEnumerateA(
    LPDDENUMCALLBACKA lpCallback,
    LPVOID lpContext
) {
    LOG_INFO("DirectDrawEnumerateA(lpCallback=%p, lpContext=%p)",
             lpCallback, lpContext);

    if (!lpCallback) {
        return DDERR_INVALIDPARAMS;
    }

    // Call callback once for default display driver
    if (!lpCallback(nullptr, "Primary Display Driver", "display", lpContext)) {
        LOG_DEBUG("DirectDrawEnumerateA: Callback returned FALSE, stopping enumeration");
    }

    return DD_OK;
}

/**
 * @brief Enumerate DirectDraw drivers (Unicode)
 */
extern "C" HRESULT WINAPI DirectDrawEnumerateW(
    LPDDENUMCALLBACKW lpCallback,
    LPVOID lpContext
) {
    LOG_INFO("DirectDrawEnumerateW(lpCallback=%p, lpContext=%p)",
             lpCallback, lpContext);

    if (!lpCallback) {
        return DDERR_INVALIDPARAMS;
    }

    // Call callback once for default display driver
    if (!lpCallback(nullptr, L"Primary Display Driver", L"display", lpContext)) {
        LOG_DEBUG("DirectDrawEnumerateW: Callback returned FALSE, stopping enumeration");
    }

    return DD_OK;
}

/**
 * @brief Extended enumerate DirectDraw drivers (ANSI)
 */
extern "C" HRESULT WINAPI DirectDrawEnumerateExA(
    LPDDENUMCALLBACKEXA lpCallback,
    LPVOID lpContext,
    DWORD dwFlags
) {
    LOG_INFO("DirectDrawEnumerateExA(lpCallback=%p, lpContext=%p, dwFlags=0x%08X)",
             lpCallback, lpContext, dwFlags);

    if (!lpCallback) {
        return DDERR_INVALIDPARAMS;
    }

    // Call callback once for default display driver
    HMONITOR hMonitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
    if (!lpCallback(nullptr, "Primary Display Driver", "display", lpContext, hMonitor)) {
        LOG_DEBUG("DirectDrawEnumerateExA: Callback returned FALSE, stopping enumeration");
    }

    return DD_OK;
}

/**
 * @brief Extended enumerate DirectDraw drivers (Unicode)
 */
extern "C" HRESULT WINAPI DirectDrawEnumerateExW(
    LPDDENUMCALLBACKEXW lpCallback,
    LPVOID lpContext,
    DWORD dwFlags
) {
    LOG_INFO("DirectDrawEnumerateExW(lpCallback=%p, lpContext=%p, dwFlags=0x%08X)",
             lpCallback, lpContext, dwFlags);

    if (!lpCallback) {
        return DDERR_INVALIDPARAMS;
    }

    // Call callback once for default display driver
    HMONITOR hMonitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
    if (!lpCallback(nullptr, L"Primary Display Driver", L"display", lpContext, hMonitor)) {
        LOG_DEBUG("DirectDrawEnumerateExW: Callback returned FALSE, stopping enumeration");
    }

    return DD_OK;
}

// ============================================================================
// COM Entry Points (Stubs)
// ============================================================================

/**
 * @brief Check if DLL can be unloaded
 */
extern "C" HRESULT WINAPI DllCanUnloadNow() {
    LOG_DEBUG("DllCanUnloadNow");
    return S_FALSE;  // Don't unload
}

/**
 * @brief Get class object for COM creation
 */
extern "C" HRESULT WINAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID* ppv
) {
    LOG_DEBUG("DllGetClassObject");
    LDC_UNUSED(rclsid);
    LDC_UNUSED(riid);

    if (ppv) {
        *ppv = nullptr;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}
