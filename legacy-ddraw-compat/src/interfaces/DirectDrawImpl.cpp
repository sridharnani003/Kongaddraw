/**
 * @file DirectDrawImpl.cpp
 * @brief IDirectDraw7 interface implementation
 */

#include "interfaces/DirectDrawImpl.h"
#include "interfaces/SurfaceImpl.h"
#include "core/Common.h"

using namespace ldc;
using namespace ldc::interfaces;

// ============================================================================
// DirectDrawImpl Implementation
// ============================================================================

DirectDrawImpl::DirectDrawImpl()
    : m_refCount(1)
    , m_interfaceVersion(7)
    , m_initialized(false)
    , m_hWnd(nullptr)
    , m_coopFlags(0)
    , m_displayWidth(0)
    , m_displayHeight(0)
    , m_displayBpp(0)
    , m_displayRefresh(0)
    , m_displayModeChanged(false)
    , m_primarySurface(nullptr)
{
    DebugLog("DirectDrawImpl created");
}

DirectDrawImpl::~DirectDrawImpl() {
    DebugLog("DirectDrawImpl destroyed");
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

    // Check for supported interfaces - we support all DirectDraw versions
    if (riid == IID_IUnknown ||
        riid == IID_IDirectDraw ||
        riid == IID_IDirectDraw2 ||
        riid == IID_IDirectDraw4 ||
        riid == IID_IDirectDraw7) {

        AddRef();
        *ppvObj = static_cast<IDirectDraw7*>(this);
        DebugLog("QueryInterface: returning IDirectDraw7");
        return S_OK;
    }

    DebugLog("QueryInterface: unsupported interface requested");
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE DirectDrawImpl::AddRef() {
    ULONG count = ++m_refCount;
    return count;
}

ULONG STDMETHODCALLTYPE DirectDrawImpl::Release() {
    ULONG count = --m_refCount;

    if (count == 0) {
        delete this;
    }

    return count;
}

// ============================================================================
// IDirectDraw7 Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE DirectDrawImpl::Compact() {
    // Compact is a legacy method that has no effect
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::CreateClipper(
    DWORD dwFlags,
    LPDIRECTDRAWCLIPPER* lplpDDClipper,
    IUnknown* pUnkOuter)
{
    DebugLog("CreateClipper: flags=0x%08X", dwFlags);

    if (!lplpDDClipper) {
        return DDERR_INVALIDPARAMS;
    }

    if (pUnkOuter) {
        return CLASS_E_NOAGGREGATION;
    }

    // Clipper not yet implemented - return a stub that works
    *lplpDDClipper = nullptr;
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::CreatePalette(
    DWORD dwFlags,
    LPPALETTEENTRY lpDDColorArray,
    LPDIRECTDRAWPALETTE* lplpDDPalette,
    IUnknown* pUnkOuter)
{
    DebugLog("CreatePalette: flags=0x%08X", dwFlags);

    if (!lplpDDPalette) {
        return DDERR_INVALIDPARAMS;
    }

    if (pUnkOuter) {
        return CLASS_E_NOAGGREGATION;
    }

    // Update global palette for 8-bit mode
    if (lpDDColorArray) {
        for (int i = 0; i < 256; i++) {
            g_state.palette[i].rgbRed = lpDDColorArray[i].peRed;
            g_state.palette[i].rgbGreen = lpDDColorArray[i].peGreen;
            g_state.palette[i].rgbBlue = lpDDColorArray[i].peBlue;
            g_state.palette[i].rgbReserved = 0;
            g_state.palette32[i] = 0xFF000000 |
                (lpDDColorArray[i].peRed << 16) |
                (lpDDColorArray[i].peGreen << 8) |
                lpDDColorArray[i].peBlue;
        }
        g_state.paletteChanged = true;
    }

    // Palette not fully implemented as separate object
    *lplpDDPalette = nullptr;
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::CreateSurface(
    LPDDSURFACEDESC2 lpDDSurfaceDesc,
    LPDIRECTDRAWSURFACE7* lplpDDSurface,
    IUnknown* pUnkOuter)
{
    DebugLog("CreateSurface called");

    if (!lpDDSurfaceDesc || !lplpDDSurface) {
        return DDERR_INVALIDPARAMS;
    }

    if (pUnkOuter) {
        return CLASS_E_NOAGGREGATION;
    }

    // Validate structure size
    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC2) &&
        lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) {
        DebugLog("CreateSurface: invalid dwSize=%lu", lpDDSurfaceDesc->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    // Create the surface
    try {
        auto* surface = new SurfaceImpl(this, *lpDDSurfaceDesc);

        // Check if this is a primary surface
        if (surface->IsPrimary()) {
            m_primarySurface = surface;
            DebugLog("Created primary surface %ux%u %ubpp",
                     surface->GetWidth(), surface->GetHeight(), surface->GetBpp());

            // Initialize render target
            CreateRenderTarget(surface->GetWidth(), surface->GetHeight(), surface->GetBpp());
        } else {
            DebugLog("Created surface %ux%u %ubpp",
                      surface->GetWidth(), surface->GetHeight(), surface->GetBpp());
        }

        *lplpDDSurface = surface;
        return DD_OK;
    }
    catch (const std::bad_alloc&) {
        DebugLog("CreateSurface: out of memory");
        return DDERR_OUTOFMEMORY;
    }
    catch (const std::exception& e) {
        DebugLog("CreateSurface: exception: %s", e.what());
        return DDERR_GENERIC;
    }
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::DuplicateSurface(
    LPDIRECTDRAWSURFACE7 lpDDSurface,
    LPDIRECTDRAWSURFACE7* lplpDupDDSurface)
{
    DebugLog("DuplicateSurface called");

    if (!lpDDSurface || !lplpDupDDSurface) {
        return DDERR_INVALIDPARAMS;
    }

    *lplpDupDDSurface = nullptr;
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::EnumDisplayModes(
    DWORD dwFlags,
    LPDDSURFACEDESC2 lpDDSurfaceDesc,
    LPVOID lpContext,
    LPDDENUMMODESCALLBACK2 lpEnumModesCallback)
{
    DebugLog("EnumDisplayModes: flags=0x%08X", dwFlags);

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
                desc.ddpfPixelFormat.dwRBitMask = 0xF800;
                desc.ddpfPixelFormat.dwGBitMask = 0x07E0;
                desc.ddpfPixelFormat.dwBBitMask = 0x001F;
            } else {
                desc.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
                desc.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
                desc.ddpfPixelFormat.dwBBitMask = 0x000000FF;
            }
        }

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
    DebugLog("EnumSurfaces: flags=0x%08X", dwFlags);

    if (!lpEnumSurfacesCallback) {
        return DDERR_INVALIDPARAMS;
    }

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::FlipToGDISurface() {
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetCaps(
    LPDDCAPS lpDDDriverCaps,
    LPDDCAPS lpDDHELCaps)
{
    DebugLog("GetCaps called");
    FillCaps(lpDDDriverCaps);
    FillCaps(lpDDHELCaps);
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetDisplayMode(LPDDSURFACEDESC2 lpDDSurfaceDesc) {
    DebugLog("GetDisplayMode called");

    if (!lpDDSurfaceDesc) {
        return DDERR_INVALIDPARAMS;
    }

    ZeroMemory(lpDDSurfaceDesc, sizeof(DDSURFACEDESC2));
    lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC2);
    lpDDSurfaceDesc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_REFRESHRATE;

    if (m_displayModeChanged) {
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
    if (lpNumCodes) {
        *lpNumCodes = 0;
    }
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetGDISurface(
    LPDIRECTDRAWSURFACE7* lplpGDISurface)
{
    DebugLog("GetGDISurface called");

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
    if (!lpdwFrequency) {
        return DDERR_INVALIDPARAMS;
    }
    *lpdwFrequency = m_displayRefresh ? m_displayRefresh : 60;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetScanLine(LPDWORD lpdwScanLine) {
    if (!lpdwScanLine) {
        return DDERR_INVALIDPARAMS;
    }
    *lpdwScanLine = 0;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetVerticalBlankStatus(LPBOOL lpbIsInVB) {
    if (!lpbIsInVB) {
        return DDERR_INVALIDPARAMS;
    }
    *lpbIsInVB = TRUE;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::Initialize(GUID* lpGUID) {
    LDC_UNUSED(lpGUID);
    return DDERR_ALREADYINITIALIZED;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::RestoreDisplayMode() {
    DebugLog("RestoreDisplayMode called");
    m_displayModeChanged = false;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::SetCooperativeLevel(HWND hWnd, DWORD dwFlags) {
    DebugLog("SetCooperativeLevel: hWnd=%p flags=0x%08X", hWnd, dwFlags);

    m_hWnd = hWnd;
    m_coopFlags = dwFlags;
    g_state.hWnd = hWnd;
    g_state.coopLevel = dwFlags;

    // Subclass window for mouse coordinate transformation
    if (hWnd) {
        SubclassWindow(hWnd);
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
    DebugLog("SetDisplayMode: %ux%u %ubpp %uHz flags=0x%08X",
             dwWidth, dwHeight, dwBPP, dwRefreshRate, dwFlags);

    if (dwWidth == 0 || dwHeight == 0 || dwBPP == 0) {
        return DDERR_INVALIDMODE;
    }

    m_displayWidth = dwWidth;
    m_displayHeight = dwHeight;
    m_displayBpp = dwBPP;
    m_displayRefresh = dwRefreshRate;
    m_displayModeChanged = true;

    g_state.gameWidth = dwWidth;
    g_state.gameHeight = dwHeight;
    g_state.gameBpp = dwBPP;
    g_state.gameRefresh = dwRefreshRate;
    g_state.displayModeSet = true;

    // Resize window to match game resolution
    if (m_hWnd) {
        RECT windowRect = { 0, 0, (LONG)dwWidth, (LONG)dwHeight };
        AdjustWindowRect(&windowRect, GetWindowLong(m_hWnd, GWL_STYLE), FALSE);
        SetWindowPos(m_hWnd, nullptr, 0, 0,
                     windowRect.right - windowRect.left,
                     windowRect.bottom - windowRect.top,
                     SWP_NOMOVE | SWP_NOZORDER);
        UpdateScaling();
    }

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent) {
    LDC_UNUSED(dwFlags);
    LDC_UNUSED(hEvent);
    Sleep(1);
    return DD_OK;
}

// IDirectDraw2 Methods
HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetAvailableVidMem(
    LPDDSCAPS2 lpDDSCaps,
    LPDWORD lpdwTotal,
    LPDWORD lpdwFree)
{
    LDC_UNUSED(lpDDSCaps);
    if (lpdwTotal) *lpdwTotal = 64 * 1024 * 1024;
    if (lpdwFree) *lpdwFree = 64 * 1024 * 1024;
    return DD_OK;
}

// IDirectDraw4 Methods
HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetSurfaceFromDC(HDC hdc, LPDIRECTDRAWSURFACE7* lpDDS) {
    LDC_UNUSED(hdc);
    if (!lpDDS) return DDERR_INVALIDPARAMS;
    *lpDDS = nullptr;
    return DDERR_NOTFOUND;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::RestoreAllSurfaces() {
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::TestCooperativeLevel() {
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::GetDeviceIdentifier(
    LPDDDEVICEIDENTIFIER2 lpdddi,
    DWORD dwFlags)
{
    LDC_UNUSED(dwFlags);
    if (!lpdddi) return DDERR_INVALIDPARAMS;
    FillDeviceIdentifier(lpdddi);
    return DD_OK;
}

// IDirectDraw7 Methods
HRESULT STDMETHODCALLTYPE DirectDrawImpl::StartModeTest(LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags) {
    LDC_UNUSED(lpModesToTest);
    LDC_UNUSED(dwNumEntries);
    LDC_UNUSED(dwFlags);
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE DirectDrawImpl::EvaluateMode(DWORD dwFlags, DWORD* pSecondsUntilTimeout) {
    LDC_UNUSED(dwFlags);
    LDC_UNUSED(pSecondsUntilTimeout);
    return DDERR_UNSUPPORTED;
}

// ============================================================================
// Helper Methods
// ============================================================================

void DirectDrawImpl::FillCaps(LPDDCAPS pCaps) {
    if (!pCaps) return;

    ZeroMemory(pCaps, sizeof(DDCAPS));
    pCaps->dwSize = sizeof(DDCAPS);
    pCaps->dwCaps = DDCAPS_BLT | DDCAPS_BLTCOLORFILL |
                    DDCAPS_BLTSTRETCH | DDCAPS_COLORKEY |
                    DDCAPS_PALETTE;
    pCaps->dwCaps2 = DDCAPS2_PRIMARYGAMMA;
    pCaps->dwVidMemTotal = 64 * 1024 * 1024;
    pCaps->dwVidMemFree = 64 * 1024 * 1024;
    pCaps->ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_FLIP |
                            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_PALETTE |
                            DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY |
                            DDSCAPS_VIDEOMEMORY;
}

void DirectDrawImpl::FillDeviceIdentifier(LPDDDEVICEIDENTIFIER2 pDDDI) {
    if (!pDDDI) return;

    ZeroMemory(pDDDI, sizeof(DDDEVICEIDENTIFIER2));
    strcpy_s(pDDDI->szDriver, "legacy-ddraw-compat");
    strcpy_s(pDDDI->szDescription, "Legacy DirectDraw Compatibility Layer");
    pDDDI->dwVendorId = 0;
    pDDDI->dwDeviceId = 0;
    pDDDI->dwSubSysId = 0;
    pDDDI->dwRevision = 0;
}
