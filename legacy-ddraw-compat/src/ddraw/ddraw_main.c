/**
 * @file ddraw_main.c
 * @brief IDirectDraw7 implementation in pure C
 */

#include "core/Common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declare surface creation */
HRESULT LDC_CreateSurface(LDC_DirectDraw* dd, LPDDSURFACEDESC2 lpDesc, LPDIRECTDRAWSURFACE7* lplpSurf, IUnknown* pUnkOuter);
HRESULT LDC_CreatePalette(LDC_DirectDraw* dd, DWORD dwFlags, LPPALETTEENTRY lpEntries, LPDIRECTDRAWPALETTE* lplpPal, IUnknown* pUnkOuter);
HRESULT LDC_CreateClipper(LDC_DirectDraw* dd, DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpClip, IUnknown* pUnkOuter);

/* ============================================================================
 * LDC_DirectDraw Structure
 * ============================================================================ */

struct LDC_DirectDraw {
    IDirectDraw7Vtbl* lpVtbl;
    LONG refCount;
    HWND hWnd;
    DWORD coopFlags;
    DWORD displayWidth;
    DWORD displayHeight;
    DWORD displayBpp;
    DWORD displayRefresh;
    BOOL displayModeChanged;
    LDC_Surface* primarySurface;
};

/* ============================================================================
 * IUnknown Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE DD_QueryInterface(IDirectDraw7* This, REFIID riid, void** ppvObj)
{
    LDC_DirectDraw* dd = (LDC_DirectDraw*)This;

    if (!ppvObj) return E_POINTER;
    *ppvObj = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectDraw) ||
        IsEqualGUID(riid, &IID_IDirectDraw2) ||
        IsEqualGUID(riid, &IID_IDirectDraw4) ||
        IsEqualGUID(riid, &IID_IDirectDraw7)) {
        InterlockedIncrement(&dd->refCount);
        *ppvObj = This;
        LDC_LOG("DD_QueryInterface: OK");
        return S_OK;
    }

    LDC_LOG("DD_QueryInterface: E_NOINTERFACE");
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE DD_AddRef(IDirectDraw7* This)
{
    LDC_DirectDraw* dd = (LDC_DirectDraw*)This;
    return InterlockedIncrement(&dd->refCount);
}

static ULONG STDMETHODCALLTYPE DD_Release(IDirectDraw7* This)
{
    LDC_DirectDraw* dd = (LDC_DirectDraw*)This;
    LONG ref = InterlockedDecrement(&dd->refCount);

    if (ref == 0) {
        LDC_LOG("DirectDraw destroyed");
        free(dd);
    }

    return ref;
}

/* ============================================================================
 * IDirectDraw Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE DD_Compact(IDirectDraw7* This)
{
    LDC_UNUSED(This);
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_CreateClipper(IDirectDraw7* This, DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpDDClipper, IUnknown* pUnkOuter)
{
    LDC_DirectDraw* dd = (LDC_DirectDraw*)This;
    LDC_LOG("DD_CreateClipper: flags=0x%08X", dwFlags);
    return LDC_CreateClipper(dd, dwFlags, lplpDDClipper, pUnkOuter);
}

static HRESULT STDMETHODCALLTYPE DD_CreatePalette(IDirectDraw7* This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE* lplpDDPalette, IUnknown* pUnkOuter)
{
    LDC_DirectDraw* dd = (LDC_DirectDraw*)This;
    LDC_LOG("DD_CreatePalette: flags=0x%08X", dwFlags);
    return LDC_CreatePalette(dd, dwFlags, lpDDColorArray, lplpDDPalette, pUnkOuter);
}

static HRESULT STDMETHODCALLTYPE DD_CreateSurface(IDirectDraw7* This, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE7* lplpDDSurface, IUnknown* pUnkOuter)
{
    LDC_DirectDraw* dd = (LDC_DirectDraw*)This;
    LDC_LOG("DD_CreateSurface called");
    return LDC_CreateSurface(dd, lpDDSurfaceDesc, lplpDDSurface, pUnkOuter);
}

static HRESULT STDMETHODCALLTYPE DD_DuplicateSurface(IDirectDraw7* This, LPDIRECTDRAWSURFACE7 lpDDSurface, LPDIRECTDRAWSURFACE7* lplpDupDDSurface)
{
    LDC_UNUSED(This);
    LDC_UNUSED(lpDDSurface);
    if (lplpDupDDSurface) *lplpDupDDSurface = NULL;
    return DDERR_UNSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE DD_EnumDisplayModes(IDirectDraw7* This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback)
{
    LDC_UNUSED(This);
    LDC_UNUSED(dwFlags);

    if (!lpEnumModesCallback) return DDERR_INVALIDPARAMS;

    /* Standard display modes */
    static const struct { DWORD w, h, bpp; } modes[] = {
        { 640, 480, 8 }, { 640, 480, 16 }, { 640, 480, 32 },
        { 800, 600, 8 }, { 800, 600, 16 }, { 800, 600, 32 },
        { 1024, 768, 8 }, { 1024, 768, 16 }, { 1024, 768, 32 },
    };

    size_t i;
    for (i = 0; i < sizeof(modes)/sizeof(modes[0]); i++) {
        /* Apply filter if provided */
        if (lpDDSurfaceDesc) {
            if ((lpDDSurfaceDesc->dwFlags & DDSD_WIDTH) && lpDDSurfaceDesc->dwWidth != modes[i].w) continue;
            if ((lpDDSurfaceDesc->dwFlags & DDSD_HEIGHT) && lpDDSurfaceDesc->dwHeight != modes[i].h) continue;
            if ((lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT) &&
                lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount != modes[i].bpp) continue;
        }

        DDSURFACEDESC2 desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.dwSize = sizeof(DDSURFACEDESC2);
        desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_REFRESHRATE;
        desc.dwWidth = modes[i].w;
        desc.dwHeight = modes[i].h;
        desc.lPitch = modes[i].w * (modes[i].bpp / 8);
        desc.dwRefreshRate = 60;

        desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        desc.ddpfPixelFormat.dwRGBBitCount = modes[i].bpp;

        if (modes[i].bpp == 8) {
            desc.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
        } else {
            desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
            if (modes[i].bpp == 16) {
                desc.ddpfPixelFormat.dwRBitMask = 0xF800;
                desc.ddpfPixelFormat.dwGBitMask = 0x07E0;
                desc.ddpfPixelFormat.dwBBitMask = 0x001F;
            } else {
                desc.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
                desc.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
                desc.ddpfPixelFormat.dwBBitMask = 0x000000FF;
            }
        }

        if (lpEnumModesCallback(&desc, lpContext) == DDENUMRET_CANCEL) break;
    }

    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_EnumSurfaces(IDirectDraw7* This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback)
{
    LDC_UNUSED(This);
    LDC_UNUSED(dwFlags);
    LDC_UNUSED(lpDDSD);
    LDC_UNUSED(lpContext);
    LDC_UNUSED(lpEnumSurfacesCallback);
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_FlipToGDISurface(IDirectDraw7* This)
{
    LDC_UNUSED(This);
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_GetCaps(IDirectDraw7* This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
    LDC_UNUSED(This);

    if (lpDDDriverCaps) {
        ZeroMemory(lpDDDriverCaps, sizeof(DDCAPS));
        lpDDDriverCaps->dwSize = sizeof(DDCAPS);
        lpDDDriverCaps->dwCaps = DDCAPS_BLT | DDCAPS_BLTCOLORFILL | DDCAPS_BLTSTRETCH | DDCAPS_COLORKEY | DDCAPS_PALETTE;
        lpDDDriverCaps->dwCaps2 = DDCAPS2_PRIMARYGAMMA;
        lpDDDriverCaps->dwVidMemTotal = 64 * 1024 * 1024;
        lpDDDriverCaps->dwVidMemFree = 64 * 1024 * 1024;
        lpDDDriverCaps->ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_FLIP | DDSCAPS_OFFSCREENPLAIN |
                                          DDSCAPS_PALETTE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY;
    }

    if (lpDDHELCaps) {
        ZeroMemory(lpDDHELCaps, sizeof(DDCAPS));
        lpDDHELCaps->dwSize = sizeof(DDCAPS);
        lpDDHELCaps->dwCaps = DDCAPS_BLT | DDCAPS_BLTCOLORFILL | DDCAPS_BLTSTRETCH | DDCAPS_COLORKEY | DDCAPS_PALETTE;
        lpDDHELCaps->dwVidMemTotal = 64 * 1024 * 1024;
        lpDDHELCaps->dwVidMemFree = 64 * 1024 * 1024;
    }

    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_GetDisplayMode(IDirectDraw7* This, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
    LDC_DirectDraw* dd = (LDC_DirectDraw*)This;

    if (!lpDDSurfaceDesc) return DDERR_INVALIDPARAMS;

    ZeroMemory(lpDDSurfaceDesc, sizeof(DDSURFACEDESC2));
    lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC2);
    lpDDSurfaceDesc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_REFRESHRATE;

    if (dd->displayModeChanged) {
        lpDDSurfaceDesc->dwWidth = dd->displayWidth;
        lpDDSurfaceDesc->dwHeight = dd->displayHeight;
        lpDDSurfaceDesc->lPitch = dd->displayWidth * (dd->displayBpp / 8);
        lpDDSurfaceDesc->dwRefreshRate = dd->displayRefresh ? dd->displayRefresh : 60;
        lpDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount = dd->displayBpp;
        lpDDSurfaceDesc->ddpfPixelFormat.dwFlags = DDPF_RGB;

        if (dd->displayBpp == 8) {
            lpDDSurfaceDesc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
        } else if (dd->displayBpp == 16) {
            lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0xF800;
            lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x07E0;
            lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x001F;
        } else {
            lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
            lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
            lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x000000FF;
        }
    } else {
        /* Return desktop mode */
        HDC hdc = GetDC(NULL);
        lpDDSurfaceDesc->dwWidth = GetSystemMetrics(SM_CXSCREEN);
        lpDDSurfaceDesc->dwHeight = GetSystemMetrics(SM_CYSCREEN);
        lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount = GetDeviceCaps(hdc, BITSPIXEL);
        lpDDSurfaceDesc->lPitch = lpDDSurfaceDesc->dwWidth * (lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount / 8);
        lpDDSurfaceDesc->dwRefreshRate = 60;
        ReleaseDC(NULL, hdc);

        lpDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        lpDDSurfaceDesc->ddpfPixelFormat.dwFlags = DDPF_RGB;
        lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
        lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
        lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x000000FF;
    }

    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_GetFourCCCodes(IDirectDraw7* This, LPDWORD lpNumCodes, LPDWORD lpCodes)
{
    LDC_UNUSED(This);
    LDC_UNUSED(lpCodes);
    if (lpNumCodes) *lpNumCodes = 0;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_GetGDISurface(IDirectDraw7* This, LPDIRECTDRAWSURFACE7* lplpGDIDDSSurface)
{
    LDC_DirectDraw* dd = (LDC_DirectDraw*)This;

    if (!lplpGDIDDSSurface) return DDERR_INVALIDPARAMS;

    if (dd->primarySurface) {
        IDirectDrawSurface7* surf = (IDirectDrawSurface7*)dd->primarySurface;
        surf->lpVtbl->AddRef(surf);
        *lplpGDIDDSSurface = surf;
        return DD_OK;
    }

    *lplpGDIDDSSurface = NULL;
    return DDERR_NOTFOUND;
}

static HRESULT STDMETHODCALLTYPE DD_GetMonitorFrequency(IDirectDraw7* This, LPDWORD lpdwFrequency)
{
    LDC_DirectDraw* dd = (LDC_DirectDraw*)This;
    if (!lpdwFrequency) return DDERR_INVALIDPARAMS;
    *lpdwFrequency = dd->displayRefresh ? dd->displayRefresh : 60;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_GetScanLine(IDirectDraw7* This, LPDWORD lpdwScanLine)
{
    LDC_UNUSED(This);
    if (!lpdwScanLine) return DDERR_INVALIDPARAMS;
    *lpdwScanLine = 0;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_GetVerticalBlankStatus(IDirectDraw7* This, LPBOOL lpbIsInVB)
{
    LDC_UNUSED(This);
    if (!lpbIsInVB) return DDERR_INVALIDPARAMS;
    *lpbIsInVB = TRUE;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_Initialize(IDirectDraw7* This, GUID* lpGUID)
{
    LDC_UNUSED(This);
    LDC_UNUSED(lpGUID);
    return DDERR_ALREADYINITIALIZED;
}

static HRESULT STDMETHODCALLTYPE DD_RestoreDisplayMode(IDirectDraw7* This)
{
    LDC_DirectDraw* dd = (LDC_DirectDraw*)This;
    dd->displayModeChanged = FALSE;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_SetCooperativeLevel(IDirectDraw7* This, HWND hWnd, DWORD dwFlags)
{
    LDC_DirectDraw* dd = (LDC_DirectDraw*)This;

    LDC_LOG("DD_SetCooperativeLevel: hWnd=%p flags=0x%08X", hWnd, dwFlags);

    dd->hWnd = hWnd;
    dd->coopFlags = dwFlags;
    g_ldc.hWnd = hWnd;
    g_ldc.coopLevel = dwFlags;

    if (hWnd) {
        LDC_SubclassWindow(hWnd);
    }

    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_SetDisplayMode(IDirectDraw7* This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
    LDC_DirectDraw* dd = (LDC_DirectDraw*)This;

    LDC_LOG("DD_SetDisplayMode: %ux%u %ubpp %uHz flags=0x%08X", dwWidth, dwHeight, dwBPP, dwRefreshRate, dwFlags);

    if (dwWidth == 0 || dwHeight == 0 || dwBPP == 0) {
        return DDERR_INVALIDMODE;
    }

    dd->displayWidth = dwWidth;
    dd->displayHeight = dwHeight;
    dd->displayBpp = dwBPP;
    dd->displayRefresh = dwRefreshRate;
    dd->displayModeChanged = TRUE;

    g_ldc.gameWidth = dwWidth;
    g_ldc.gameHeight = dwHeight;
    g_ldc.gameBpp = dwBPP;
    g_ldc.gameRefresh = dwRefreshRate;
    g_ldc.displayModeSet = TRUE;

    /* Resize window */
    if (dd->hWnd) {
        RECT windowRect = { 0, 0, (LONG)dwWidth, (LONG)dwHeight };
        AdjustWindowRect(&windowRect, GetWindowLong(dd->hWnd, GWL_STYLE), FALSE);
        SetWindowPos(dd->hWnd, NULL, 0, 0,
                     windowRect.right - windowRect.left,
                     windowRect.bottom - windowRect.top,
                     SWP_NOMOVE | SWP_NOZORDER);
        LDC_UpdateScaling();
    }

    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_WaitForVerticalBlank(IDirectDraw7* This, DWORD dwFlags, HANDLE hEvent)
{
    LDC_UNUSED(This);
    LDC_UNUSED(dwFlags);
    LDC_UNUSED(hEvent);
    Sleep(1);
    return DD_OK;
}

/* IDirectDraw2 Methods */
static HRESULT STDMETHODCALLTYPE DD_GetAvailableVidMem(IDirectDraw7* This, LPDDSCAPS2 lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree)
{
    LDC_UNUSED(This);
    LDC_UNUSED(lpDDSCaps);
    if (lpdwTotal) *lpdwTotal = 64 * 1024 * 1024;
    if (lpdwFree) *lpdwFree = 64 * 1024 * 1024;
    return DD_OK;
}

/* IDirectDraw4 Methods */
static HRESULT STDMETHODCALLTYPE DD_GetSurfaceFromDC(IDirectDraw7* This, HDC hdc, LPDIRECTDRAWSURFACE7* lpDDS)
{
    LDC_UNUSED(This);
    LDC_UNUSED(hdc);
    if (lpDDS) *lpDDS = NULL;
    return DDERR_NOTFOUND;
}

static HRESULT STDMETHODCALLTYPE DD_RestoreAllSurfaces(IDirectDraw7* This)
{
    LDC_UNUSED(This);
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_TestCooperativeLevel(IDirectDraw7* This)
{
    LDC_UNUSED(This);
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE DD_GetDeviceIdentifier(IDirectDraw7* This, LPDDDEVICEIDENTIFIER2 lpdddi, DWORD dwFlags)
{
    LDC_UNUSED(This);
    LDC_UNUSED(dwFlags);

    if (!lpdddi) return DDERR_INVALIDPARAMS;

    ZeroMemory(lpdddi, sizeof(DDDEVICEIDENTIFIER2));
    strcpy_s(lpdddi->szDriver, sizeof(lpdddi->szDriver), "legacy-ddraw-compat");
    strcpy_s(lpdddi->szDescription, sizeof(lpdddi->szDescription), "Legacy DirectDraw Compatibility Layer");
    lpdddi->dwVendorId = 0;
    lpdddi->dwDeviceId = 0;
    lpdddi->dwSubSysId = 0;
    lpdddi->dwRevision = 0;

    return DD_OK;
}

/* IDirectDraw7 Methods */
static HRESULT STDMETHODCALLTYPE DD_StartModeTest(IDirectDraw7* This, LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags)
{
    LDC_UNUSED(This);
    LDC_UNUSED(lpModesToTest);
    LDC_UNUSED(dwNumEntries);
    LDC_UNUSED(dwFlags);
    return DDERR_UNSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE DD_EvaluateMode(IDirectDraw7* This, DWORD dwFlags, DWORD* pSecondsUntilTimeout)
{
    LDC_UNUSED(This);
    LDC_UNUSED(dwFlags);
    LDC_UNUSED(pSecondsUntilTimeout);
    return DDERR_UNSUPPORTED;
}

/* ============================================================================
 * VTable
 * ============================================================================ */

static IDirectDraw7Vtbl g_DirectDrawVtbl = {
    /* IUnknown */
    DD_QueryInterface,
    DD_AddRef,
    DD_Release,
    /* IDirectDraw */
    DD_Compact,
    DD_CreateClipper,
    DD_CreatePalette,
    DD_CreateSurface,
    DD_DuplicateSurface,
    DD_EnumDisplayModes,
    DD_EnumSurfaces,
    DD_FlipToGDISurface,
    DD_GetCaps,
    DD_GetDisplayMode,
    DD_GetFourCCCodes,
    DD_GetGDISurface,
    DD_GetMonitorFrequency,
    DD_GetScanLine,
    DD_GetVerticalBlankStatus,
    DD_Initialize,
    DD_RestoreDisplayMode,
    DD_SetCooperativeLevel,
    DD_SetDisplayMode,
    DD_WaitForVerticalBlank,
    /* IDirectDraw2 */
    DD_GetAvailableVidMem,
    /* IDirectDraw4 */
    DD_GetSurfaceFromDC,
    DD_RestoreAllSurfaces,
    DD_TestCooperativeLevel,
    DD_GetDeviceIdentifier,
    /* IDirectDraw7 */
    DD_StartModeTest,
    DD_EvaluateMode
};

/* ============================================================================
 * DirectDraw Creation
 * ============================================================================ */

HRESULT LDC_CreateDirectDraw(GUID* lpGUID, IDirectDraw7** lplpDD, IUnknown* pUnkOuter)
{
    LDC_UNUSED(lpGUID);

    if (!lplpDD) return DDERR_INVALIDPARAMS;
    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    *lplpDD = NULL;

    LDC_DirectDraw* dd = (LDC_DirectDraw*)malloc(sizeof(LDC_DirectDraw));
    if (!dd) return DDERR_OUTOFMEMORY;

    ZeroMemory(dd, sizeof(LDC_DirectDraw));
    dd->lpVtbl = &g_DirectDrawVtbl;
    dd->refCount = 1;

    LDC_LOG("DirectDraw created");

    *lplpDD = (IDirectDraw7*)dd;
    return DD_OK;
}

/* Accessor for setting primary surface from surface creation */
void LDC_DD_SetPrimarySurface(LDC_DirectDraw* dd, LDC_Surface* surf)
{
    if (dd) dd->primarySurface = surf;
}

HWND LDC_DD_GetHWnd(LDC_DirectDraw* dd)
{
    return dd ? dd->hWnd : NULL;
}
