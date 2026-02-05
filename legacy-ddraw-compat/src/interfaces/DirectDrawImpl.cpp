/**
 * @file DirectDrawImpl.cpp
 * @brief IDirectDraw interface implementation
 */

#include "interfaces/DirectDrawImpl.h"
#include "interfaces/SurfaceImpl.h"
#include "logging/Logger.h"
#include "config/Config.h"

using namespace ldc;
using namespace ldc::interfaces;

// ============================================================================
// GUIDs for interface identification
// ============================================================================

// Define GUIDs if not already defined
#ifndef DEFINE_GUID_INLINE
#define DEFINE_GUID_INLINE(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
#endif

DEFINE_GUID_INLINE(LOCAL_IID_IDirectDraw,  0x6C14DB80,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
DEFINE_GUID_INLINE(LOCAL_IID_IDirectDraw2, 0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_GUID_INLINE(LOCAL_IID_IDirectDraw4, 0x9c59509a,0x39bd,0x11d1,0x8c,0x4a,0x00,0xc0,0x4f,0xd9,0x30,0xc5);
DEFINE_GUID_INLINE(LOCAL_IID_IDirectDraw7, 0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b);

// ============================================================================
// DirectDrawImpl Implementation
// ============================================================================

DirectDrawImpl::DirectDrawImpl()
    : m_refCount(1)
    , m_hWnd(nullptr)
    , m_coopLevel(0)
    , m_displayWidth(0)
    , m_displayHeight(0)
    , m_displayBpp(0)
    , m_displayRefresh(0)
    , m_displayModeSet(false)
    , m_renderer(nullptr)
{
    LOG_DEBUG("DirectDrawImpl created");
}

DirectDrawImpl::~DirectDrawImpl() {
    LOG_DEBUG("DirectDrawImpl destroyed");

    // Release renderer
    if (m_renderer) {
        m_renderer->Shutdown();
        m_renderer.reset();
    }

    // Release all surfaces
    m_surfaces.clear();
    m_primarySurface = nullptr;
}

// ============================================================================
// IUnknown Implementation
// ============================================================================

HRESULT STDMETHODCALLTYPE DirectDrawImpl::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }

    *ppvObj = nullptr;

    // Check for supported interfaces
    if (riid == IID_IUnknown ||
        riid == LOCAL_IID_IDirectDraw ||
        riid == LOCAL_IID_IDirectDraw2 ||
        riid == LOCAL_IID_IDirectDraw4 ||
        riid == LOCAL_IID_IDirectDraw7) {

        AddRef();
        *ppvObj = static_cast<IDirectDraw7*>(this);
        LOG_DEBUG("QueryInterface: returning IDirectDraw7");
        return S_OK;
    }

    LOG_WARN("QueryInterface: unsupported interface requested");
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE DirectDrawImpl::AddRef() {
    ULONG count = ++m_refCount;
    LOG_TRACE("DirectDrawImpl::AddRef: %lu", count);
    return count;
}

ULONG STDMETHODCALLTYPE DirectDrawImpl::Release() {
    ULONG count = --m_refCount;
    LOG_TRACE("DirectDrawImpl::Release: %lu", count);

    if (count == 0) {
        delete this;
    }

    return count;
}

// ============================================================================
// IDirectDraw Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE DirectDrawImpl::Compact() {
    // Compact is a legacy method that has no effect
    LOG_TRACE("Compact called (no-op)");
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::CreateClipper(
    DWORD dwFlags,
    LPDIRECTDRAWCLIPPER* lplpDDClipper,
    IUnknown* pUnkOuter)
{
    LOG_DEBUG("CreateClipper: flags=0x%08X", dwFlags);

    if (!lplpDDClipper) {
        return DDERR_INVALIDPARAMS;
    }

    if (pUnkOuter) {
        return CLASS_E_NOAGGREGATION;
    }

    // TODO: Implement clipper creation
    *lplpDDClipper = nullptr;
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::CreatePalette(
    DWORD dwFlags,
    LPPALETTEENTRY lpDDColorArray,
    LPDIRECTDRAWPALETTE* lplpDDPalette,
    IUnknown* pUnkOuter)
{
    LOG_DEBUG("CreatePalette: flags=0x%08X", dwFlags);

    if (!lplpDDPalette) {
        return DDERR_INVALIDPARAMS;
    }

    if (pUnkOuter) {
        return CLASS_E_NOAGGREGATION;
    }

    // TODO: Implement palette creation
    *lplpDDPalette = nullptr;
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::CreateSurface(
    LPDDSURFACEDESC2 lpDDSurfaceDesc,
    LPDIRECTDRAWSURFACE7* lplpDDSurface,
    IUnknown* pUnkOuter)
{
    LOG_DEBUG("CreateSurface called");

    if (!lpDDSurfaceDesc || !lplpDDSurface) {
        return DDERR_INVALIDPARAMS;
    }

    if (pUnkOuter) {
        return CLASS_E_NOAGGREGATION;
    }

    // Validate structure size
    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC2) &&
        lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) {
        LOG_WARN("CreateSurface: invalid dwSize=%lu", lpDDSurfaceDesc->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    // Create the surface
    try {
        auto surface = std::make_unique<SurfaceImpl>(this, *lpDDSurfaceDesc);

        // Check if this is a primary surface
        if (surface->IsPrimary()) {
            m_primarySurface = surface.get();
            LOG_INFO("Created primary surface %ux%u %ubpp",
                     surface->GetWidth(), surface->GetHeight(), surface->GetBpp());
        } else {
            LOG_DEBUG("Created surface %ux%u %ubpp",
                      surface->GetWidth(), surface->GetHeight(), surface->GetBpp());
        }

        *lplpDDSurface = surface.get();
        m_surfaces.push_back(std::move(surface));

        return DD_OK;
    }
    catch (const std::bad_alloc&) {
        LOG_ERROR("CreateSurface: out of memory");
        return DDERR_OUTOFMEMORY;
    }
    catch (const std::exception& e) {
        LOG_ERROR("CreateSurface: exception: %s", e.what());
        return DDERR_GENERIC;
    }
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::DuplicateSurface(
    LPDIRECTDRAWSURFACE7 lpDDSurface,
    LPDIRECTDRAWSURFACE7* lplpDupDDSurface)
{
    LOG_DEBUG("DuplicateSurface called");

    if (!lpDDSurface || !lplpDupDDSurface) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO: Implement surface duplication
    *lplpDupDDSurface = nullptr;
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::EnumDisplayModes(
    DWORD dwFlags,
    LPDDSURFACEDESC2 lpDDSurfaceDesc,
    LPVOID lpContext,
    LPDDENUMMODESCALLBACK2 lpEnumModesCallback)
{
    LOG_DEBUG("EnumDisplayModes: flags=0x%08X", dwFlags);

    if (!lpEnumModesCallback) {
        return DDERR_INVALIDPARAMS;
    }

    // Standard display modes to enumerate
    struct DisplayMode {
        DWORD width;
        DWORD height;
        DWORD bpp;
    };

    static const DisplayMode modes[] = {
        { 640,  480,  8 },
        { 640,  480, 16 },
        { 640,  480, 32 },
        { 800,  600,  8 },
        { 800,  600, 16 },
        { 800,  600, 32 },
        { 1024, 768,  8 },
        { 1024, 768, 16 },
        { 1024, 768, 32 },
        { 1280, 720, 16 },
        { 1280, 720, 32 },
        { 1280, 1024, 16 },
        { 1280, 1024, 32 },
        { 1920, 1080, 32 },
    };

    for (const auto& mode : modes) {
        // Check if filter matches
        if (lpDDSurfaceDesc) {
            if ((lpDDSurfaceDesc->dwFlags & DDSD_WIDTH) &&
                lpDDSurfaceDesc->dwWidth != mode.width) {
                continue;
            }
            if ((lpDDSurfaceDesc->dwFlags & DDSD_HEIGHT) &&
                lpDDSurfaceDesc->dwHeight != mode.height) {
                continue;
            }
            if ((lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT) &&
                lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount != mode.bpp) {
                continue;
            }
        }

        // Build surface description for callback
        DDSURFACEDESC2 desc{};
        desc.dwSize = sizeof(DDSURFACEDESC2);
        desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_REFRESHRATE;
        desc.dwWidth = mode.width;
        desc.dwHeight = mode.height;
        desc.lPitch = mode.width * (mode.bpp / 8);
        desc.dwRefreshRate = 60;

        desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        desc.ddpfPixelFormat.dwRGBBitCount = mode.bpp;

        if (mode.bpp == 8) {
            desc.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
        } else {
            desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
            if (mode.bpp == 16) {
                // RGB565
                desc.ddpfPixelFormat.dwRBitMask = 0xF800;
                desc.ddpfPixelFormat.dwGBitMask = 0x07E0;
                desc.ddpfPixelFormat.dwBBitMask = 0x001F;
            } else {
                // RGB888 or XRGB8888
                desc.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
                desc.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
                desc.ddpfPixelFormat.dwBBitMask = 0x000000FF;
            }
        }

        // Call the callback
        if (lpEnumModesCallback(&desc, lpContext) == DDENUMRET_CANCEL) {
            break;
        }
    }

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::EnumSurfaces(
    DWORD dwFlags,
    LPDDSURFACEDESC2 lpDDSD,
    LPVOID lpContext,
    LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback)
{
    LOG_DEBUG("EnumSurfaces: flags=0x%08X", dwFlags);

    if (!lpEnumSurfacesCallback) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO: Implement surface enumeration
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::FlipToGDISurface() {
    LOG_TRACE("FlipToGDISurface called");
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetCaps(
    LPDDCAPS lpDDDriverCaps,
    LPDDCAPS lpDDHELCaps)
{
    LOG_DEBUG("GetCaps called");

    // Fill in driver caps
    if (lpDDDriverCaps) {
        ZeroMemory(lpDDDriverCaps, sizeof(DDCAPS));
        lpDDDriverCaps->dwSize = sizeof(DDCAPS);
        lpDDDriverCaps->dwCaps = DDCAPS_BLT | DDCAPS_BLTCOLORFILL |
                                  DDCAPS_BLTSTRETCH | DDCAPS_COLORKEY |
                                  DDCAPS_PALETTE;
        lpDDDriverCaps->dwCaps2 = DDCAPS2_PRIMARYGAMMA;
        lpDDDriverCaps->dwVidMemTotal = 64 * 1024 * 1024;  // 64 MB
        lpDDDriverCaps->dwVidMemFree = 64 * 1024 * 1024;
        lpDDDriverCaps->ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_FLIP |
                                          DDSCAPS_OFFSCREENPLAIN | DDSCAPS_PALETTE |
                                          DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY |
                                          DDSCAPS_VIDEOMEMORY;
    }

    // Fill in HEL caps (same as driver for our purposes)
    if (lpDDHELCaps) {
        if (lpDDDriverCaps) {
            memcpy(lpDDHELCaps, lpDDDriverCaps, sizeof(DDCAPS));
        } else {
            ZeroMemory(lpDDHELCaps, sizeof(DDCAPS));
            lpDDHELCaps->dwSize = sizeof(DDCAPS);
        }
    }

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetDisplayMode(LPDDSURFACEDESC2 lpDDSurfaceDesc) {
    LOG_DEBUG("GetDisplayMode called");

    if (!lpDDSurfaceDesc) {
        return DDERR_INVALIDPARAMS;
    }

    // Return current display mode
    ZeroMemory(lpDDSurfaceDesc, sizeof(DDSURFACEDESC2));
    lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC2);
    lpDDSurfaceDesc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_REFRESHRATE;

    if (m_displayModeSet) {
        lpDDSurfaceDesc->dwWidth = m_displayWidth;
        lpDDSurfaceDesc->dwHeight = m_displayHeight;
        lpDDSurfaceDesc->lPitch = m_displayWidth * (m_displayBpp / 8);
        lpDDSurfaceDesc->dwRefreshRate = m_displayRefresh ? m_displayRefresh : 60;

        lpDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount = m_displayBpp;
        lpDDSurfaceDesc->ddpfPixelFormat.dwFlags = DDPF_RGB;

        if (m_displayBpp == 8) {
            lpDDSurfaceDesc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
        } else if (m_displayBpp == 16) {
            lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0xF800;
            lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x07E0;
            lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x001F;
        } else {
            lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
            lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
            lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x000000FF;
        }
    } else {
        // Return desktop mode
        HDC hdc = GetDC(nullptr);
        lpDDSurfaceDesc->dwWidth = GetSystemMetrics(SM_CXSCREEN);
        lpDDSurfaceDesc->dwHeight = GetSystemMetrics(SM_CYSCREEN);
        lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount = GetDeviceCaps(hdc, BITSPIXEL);
        lpDDSurfaceDesc->lPitch = lpDDSurfaceDesc->dwWidth * (lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount / 8);
        lpDDSurfaceDesc->dwRefreshRate = 60;
        ReleaseDC(nullptr, hdc);

        lpDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        lpDDSurfaceDesc->ddpfPixelFormat.dwFlags = DDPF_RGB;
        lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
        lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
        lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x000000FF;
    }

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetFourCCCodes(
    LPDWORD lpNumCodes,
    LPDWORD lpCodes)
{
    LOG_DEBUG("GetFourCCCodes called");

    if (lpNumCodes) {
        *lpNumCodes = 0;
    }

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetGDISurface(
    LPDIRECTDRAWSURFACE7* lplpGDISurface)
{
    LOG_DEBUG("GetGDISurface called");

    if (!lplpGDISurface) {
        return DDERR_INVALIDPARAMS;
    }

    if (m_primarySurface) {
        m_primarySurface->AddRef();
        *lplpGDISurface = m_primarySurface;
        return DD_OK;
    }

    *lplpGDISurface = nullptr;
    return DDERR_NOTFOUND;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetMonitorFrequency(LPDWORD lpdwFrequency) {
    LOG_DEBUG("GetMonitorFrequency called");

    if (!lpdwFrequency) {
        return DDERR_INVALIDPARAMS;
    }

    *lpdwFrequency = m_displayRefresh ? m_displayRefresh : 60;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetScanLine(LPDWORD lpdwScanLine) {
    LOG_TRACE("GetScanLine called");

    if (!lpdwScanLine) {
        return DDERR_INVALIDPARAMS;
    }

    // Return a simulated scan line position
    *lpdwScanLine = 0;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetVerticalBlankStatus(LPBOOL lpbIsInVB) {
    LOG_TRACE("GetVerticalBlankStatus called");

    if (!lpbIsInVB) {
        return DDERR_INVALIDPARAMS;
    }

    // Always report in vertical blank for simplicity
    *lpbIsInVB = TRUE;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::Initialize(GUID* lpGUID) {
    LOG_DEBUG("Initialize called");

    // Already initialized
    return DDERR_ALREADYINITIALIZED;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::RestoreDisplayMode() {
    LOG_DEBUG("RestoreDisplayMode called");

    if (!m_displayModeSet) {
        return DD_OK;
    }

    m_displayModeSet = false;
    m_displayWidth = 0;
    m_displayHeight = 0;
    m_displayBpp = 0;
    m_displayRefresh = 0;

    // TODO: Restore window to original state

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::SetCooperativeLevel(HWND hWnd, DWORD dwFlags) {
    LOG_INFO("SetCooperativeLevel: hWnd=%p flags=0x%08X", hWnd, dwFlags);

    m_hWnd = hWnd;
    m_coopLevel = dwFlags;

    // Validate flags
    if ((dwFlags & DDSCL_EXCLUSIVE) && !(dwFlags & DDSCL_FULLSCREEN)) {
        return DDERR_INVALIDPARAMS;
    }

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::SetDisplayMode(
    DWORD dwWidth,
    DWORD dwHeight,
    DWORD dwBPP,
    DWORD dwRefreshRate,
    DWORD dwFlags)
{
    LOG_INFO("SetDisplayMode: %ux%u %ubpp %uHz flags=0x%08X",
             dwWidth, dwHeight, dwBPP, dwRefreshRate, dwFlags);

    // Validate parameters
    if (dwWidth == 0 || dwHeight == 0 || dwBPP == 0) {
        return DDERR_INVALIDMODE;
    }

    // Store the mode
    m_displayWidth = dwWidth;
    m_displayHeight = dwHeight;
    m_displayBpp = dwBPP;
    m_displayRefresh = dwRefreshRate;
    m_displayModeSet = true;

    // TODO: Configure window for the display mode

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent) {
    LOG_TRACE("WaitForVerticalBlank: flags=0x%08X", dwFlags);

    // Simple implementation: sleep for approximately one frame
    Sleep(1);

    return DD_OK;
}

// ============================================================================
// IDirectDraw2 Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetAvailableVidMem(
    LPDDSCAPS2 lpDDSCaps,
    LPDWORD lpdwTotal,
    LPDWORD lpdwFree)
{
    LOG_DEBUG("GetAvailableVidMem called");

    if (lpdwTotal) {
        *lpdwTotal = 64 * 1024 * 1024;  // 64 MB
    }
    if (lpdwFree) {
        *lpdwFree = 64 * 1024 * 1024;
    }

    return DD_OK;
}

// ============================================================================
// IDirectDraw4 Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetSurfaceFromDC(HDC hdc, LPDIRECTDRAWSURFACE7* lpDDS) {
    LOG_DEBUG("GetSurfaceFromDC called");

    if (!lpDDS) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO: Find surface by DC
    *lpDDS = nullptr;
    return DDERR_NOTFOUND;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::RestoreAllSurfaces() {
    LOG_DEBUG("RestoreAllSurfaces called");

    for (auto& surface : m_surfaces) {
        surface->Restore();
    }

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::TestCooperativeLevel() {
    LOG_TRACE("TestCooperativeLevel called");
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetDeviceIdentifier(
    LPDDDEVICEIDENTIFIER2 lpdddi,
    DWORD dwFlags)
{
    LOG_DEBUG("GetDeviceIdentifier: flags=0x%08X", dwFlags);

    if (!lpdddi) {
        return DDERR_INVALIDPARAMS;
    }

    ZeroMemory(lpdddi, sizeof(DDDEVICEIDENTIFIER2));
    strcpy_s(lpdddi->szDriver, "legacy-ddraw-compat");
    strcpy_s(lpdddi->szDescription, "Legacy DirectDraw Compatibility Layer");
    lpdddi->dwVendorId = 0;
    lpdddi->dwDeviceId = 0;
    lpdddi->dwSubSysId = 0;
    lpdddi->dwRevision = 0;

    return DD_OK;
}

// ============================================================================
// IDirectDraw7 Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE DirectDrawImpl::StartModeTest(LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags) {
    LOG_DEBUG("StartModeTest called");
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::EvaluateMode(DWORD dwFlags, DWORD* pSecondsUntilTimeout) {
    LOG_DEBUG("EvaluateMode called");
    return DDERR_UNSUPPORTED;
}
