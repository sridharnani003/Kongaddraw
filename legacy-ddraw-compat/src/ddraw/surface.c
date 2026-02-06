/**
 * @file surface.c
 * @brief IDirectDrawSurface7 implementation in pure C
 */

#include "core/Common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External function declarations */
void LDC_DD_SetPrimarySurface(LDC_DirectDraw* dd, LDC_Surface* surf);
HWND LDC_DD_GetHWnd(LDC_DirectDraw* dd);

/* ============================================================================
 * LDC_Surface Structure
 * ============================================================================ */

struct LDC_Surface {
    IDirectDrawSurface7Vtbl* lpVtbl;
    LONG refCount;

    /* Parent DirectDraw */
    LDC_DirectDraw* parent;

    /* Surface properties */
    DWORD width;
    DWORD height;
    DWORD bpp;
    DWORD pitch;
    DDSCAPS2 caps;
    DDPIXELFORMAT pixelFormat;
    DWORD flags;

    /* Pixel data */
    BYTE* pixels;
    DWORD pixelSize;

    /* Attached surfaces */
    LDC_Surface* backBuffer;

    /* Associated objects */
    LDC_Palette* palette;
    LDC_Clipper* clipper;

    /* Lock state */
    BOOL locked;
    RECT lockedRect;
    CRITICAL_SECTION lockCS;

    /* Color keys */
    DDCOLORKEY srcColorKey;
    DDCOLORKEY destColorKey;
    BOOL hasSrcColorKey;
    BOOL hasDestColorKey;

    /* GDI interop */
    HDC hDC;
    HBITMAP hBitmap;
    HBITMAP hBitmapOld;

    /* Misc */
    DWORD uniquenessValue;
    DWORD priority;
    DWORD lod;
};

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

static BOOL IsPrimary(LDC_Surface* surf) {
    return (surf->caps.dwCaps & DDSCAPS_PRIMARYSURFACE) != 0;
}

static void InitializePixelFormat(LDC_Surface* surf) {
    surf->pixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    surf->pixelFormat.dwRGBBitCount = surf->bpp;

    if (surf->bpp == 8) {
        surf->pixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    } else if (surf->bpp == 16) {
        surf->pixelFormat.dwFlags = DDPF_RGB;
        surf->pixelFormat.dwRBitMask = 0xF800;
        surf->pixelFormat.dwGBitMask = 0x07E0;
        surf->pixelFormat.dwBBitMask = 0x001F;
    } else if (surf->bpp == 24) {
        surf->pixelFormat.dwFlags = DDPF_RGB;
        surf->pixelFormat.dwRBitMask = 0x00FF0000;
        surf->pixelFormat.dwGBitMask = 0x0000FF00;
        surf->pixelFormat.dwBBitMask = 0x000000FF;
    } else {
        /* 32-bit */
        surf->pixelFormat.dwFlags = DDPF_RGB;
        surf->pixelFormat.dwRBitMask = 0x00FF0000;
        surf->pixelFormat.dwGBitMask = 0x0000FF00;
        surf->pixelFormat.dwBBitMask = 0x000000FF;
    }
}

static void PresentToScreen(LDC_Surface* surf) {
    if (!IsPrimary(surf)) return;

    EnterCriticalSection(&g_ldc.renderLock);
    LDC_PresentToScreen(surf->pixels, surf->width, surf->height, surf->pitch, surf->bpp);
    LeaveCriticalSection(&g_ldc.renderLock);
}

/* ============================================================================
 * IUnknown Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE Surf_QueryInterface(IDirectDrawSurface7* This, REFIID riid, void** ppvObj)
{
    LDC_Surface* surf = (LDC_Surface*)This;

    if (!ppvObj) return E_POINTER;
    *ppvObj = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectDrawSurface) ||
        IsEqualGUID(riid, &IID_IDirectDrawSurface2) ||
        IsEqualGUID(riid, &IID_IDirectDrawSurface3) ||
        IsEqualGUID(riid, &IID_IDirectDrawSurface4) ||
        IsEqualGUID(riid, &IID_IDirectDrawSurface7)) {
        InterlockedIncrement(&surf->refCount);
        *ppvObj = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE Surf_AddRef(IDirectDrawSurface7* This)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    return InterlockedIncrement(&surf->refCount);
}

static ULONG STDMETHODCALLTYPE Surf_Release(IDirectDrawSurface7* This)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    LONG ref = InterlockedDecrement(&surf->refCount);

    if (ref == 0) {
        LDC_LOG("Surface destroyed");

        /* Clean up GDI resources */
        if (surf->hDC) {
            if (surf->hBitmapOld) {
                SelectObject(surf->hDC, surf->hBitmapOld);
            }
            DeleteDC(surf->hDC);
        }
        if (surf->hBitmap) {
            DeleteObject(surf->hBitmap);
        }

        /* Release back buffer */
        if (surf->backBuffer) {
            IDirectDrawSurface7* bb = (IDirectDrawSurface7*)surf->backBuffer;
            bb->lpVtbl->Release(bb);
        }

        DeleteCriticalSection(&surf->lockCS);
        free(surf->pixels);
        free(surf);
    }

    return ref;
}

/* ============================================================================
 * Core Surface Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE Surf_Lock(IDirectDrawSurface7* This, LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    LDC_UNUSED(hEvent);
    LDC_UNUSED(dwFlags);

    if (!lpDDSurfaceDesc) return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&surf->lockCS);

    if (surf->locked) {
        LeaveCriticalSection(&surf->lockCS);
        return DDERR_SURFACEBUSY;
    }

    ZeroMemory(lpDDSurfaceDesc, sizeof(DDSURFACEDESC2));
    lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC2);
    lpDDSurfaceDesc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_LPSURFACE | DDSD_CAPS;
    lpDDSurfaceDesc->dwWidth = surf->width;
    lpDDSurfaceDesc->dwHeight = surf->height;
    lpDDSurfaceDesc->lPitch = surf->pitch;
    lpDDSurfaceDesc->ddpfPixelFormat = surf->pixelFormat;
    lpDDSurfaceDesc->ddsCaps = surf->caps;

    if (lpDestRect) {
        size_t offset = lpDestRect->top * surf->pitch + lpDestRect->left * (surf->bpp / 8);
        lpDDSurfaceDesc->lpSurface = surf->pixels + offset;
        surf->lockedRect = *lpDestRect;
    } else {
        lpDDSurfaceDesc->lpSurface = surf->pixels;
        surf->lockedRect.left = 0;
        surf->lockedRect.top = 0;
        surf->lockedRect.right = surf->width;
        surf->lockedRect.bottom = surf->height;
    }

    surf->locked = TRUE;
    LeaveCriticalSection(&surf->lockCS);

    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surf_Unlock(IDirectDrawSurface7* This, LPRECT lpRect)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    LDC_UNUSED(lpRect);

    EnterCriticalSection(&surf->lockCS);

    if (!surf->locked) {
        LeaveCriticalSection(&surf->lockCS);
        return DDERR_NOTLOCKED;
    }

    surf->locked = FALSE;
    surf->uniquenessValue++;

    LeaveCriticalSection(&surf->lockCS);

    PresentToScreen(surf);

    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surf_Blt(IDirectDrawSurface7* This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    LDC_Surface* srcSurf = (LDC_Surface*)lpDDSrcSurface;

    RECT dstRect;
    if (lpDestRect) {
        dstRect = *lpDestRect;
    } else {
        dstRect.left = 0;
        dstRect.top = 0;
        dstRect.right = surf->width;
        dstRect.bottom = surf->height;
    }

    /* Color fill */
    if (dwFlags & DDBLT_COLORFILL) {
        if (!lpDDBltFx) return DDERR_INVALIDPARAMS;

        DWORD color = lpDDBltFx->dwFillColor;
        DWORD bytesPerPixel = surf->bpp / 8;
        LONG x, y;

        for (y = dstRect.top; y < dstRect.bottom && y < (LONG)surf->height; y++) {
            BYTE* row = surf->pixels + y * surf->pitch + dstRect.left * bytesPerPixel;
            for (x = dstRect.left; x < dstRect.right && x < (LONG)surf->width; x++) {
                if (bytesPerPixel == 1) {
                    *row = (BYTE)color;
                } else if (bytesPerPixel == 2) {
                    *(WORD*)row = (WORD)color;
                } else if (bytesPerPixel == 4) {
                    *(DWORD*)row = color;
                }
                row += bytesPerPixel;
            }
        }

        surf->uniquenessValue++;
        PresentToScreen(surf);
        return DD_OK;
    }

    /* Source blit */
    if (srcSurf) {
        RECT srcRect;
        if (lpSrcRect) {
            srcRect = *lpSrcRect;
        } else {
            srcRect.left = 0;
            srcRect.top = 0;
            srcRect.right = srcSurf->width;
            srcRect.bottom = srcSurf->height;
        }

        DWORD bytesPerPixel = surf->bpp / 8;
        LONG srcWidth = srcRect.right - srcRect.left;
        LONG srcHeight = srcRect.bottom - srcRect.top;
        LONG dstWidth = dstRect.right - dstRect.left;
        LONG dstHeight = dstRect.bottom - dstRect.top;
        LONG copyWidth = (srcWidth < dstWidth) ? srcWidth : dstWidth;
        LONG copyHeight = (srcHeight < dstHeight) ? srcHeight : dstHeight;

        /* Clamp to boundaries */
        if (dstRect.left + copyWidth > (LONG)surf->width)
            copyWidth = surf->width - dstRect.left;
        if (dstRect.top + copyHeight > (LONG)surf->height)
            copyHeight = surf->height - dstRect.top;
        if (srcRect.left + copyWidth > (LONG)srcSurf->width)
            copyWidth = srcSurf->width - srcRect.left;
        if (srcRect.top + copyHeight > (LONG)srcSurf->height)
            copyHeight = srcSurf->height - srcRect.top;

        if (copyWidth <= 0 || copyHeight <= 0) return DD_OK;

        BOOL useColorKey = (dwFlags & DDBLT_KEYSRC) && srcSurf->hasSrcColorKey;
        DWORD colorKey = srcSurf->srcColorKey.dwColorSpaceLowValue;
        LONG y, x;

        for (y = 0; y < copyHeight; y++) {
            BYTE* dstRow = surf->pixels + (dstRect.top + y) * surf->pitch + dstRect.left * bytesPerPixel;
            const BYTE* srcRow = srcSurf->pixels + (srcRect.top + y) * srcSurf->pitch + srcRect.left * bytesPerPixel;

            if (!useColorKey) {
                memcpy(dstRow, srcRow, copyWidth * bytesPerPixel);
            } else {
                for (x = 0; x < copyWidth; x++) {
                    DWORD pixel = 0;
                    if (bytesPerPixel == 1) {
                        pixel = srcRow[x];
                    } else if (bytesPerPixel == 2) {
                        pixel = ((const WORD*)srcRow)[x];
                    } else if (bytesPerPixel == 4) {
                        pixel = ((const DWORD*)srcRow)[x];
                    }

                    if (pixel != colorKey) {
                        if (bytesPerPixel == 1) {
                            dstRow[x] = (BYTE)pixel;
                        } else if (bytesPerPixel == 2) {
                            ((WORD*)dstRow)[x] = (WORD)pixel;
                        } else if (bytesPerPixel == 4) {
                            ((DWORD*)dstRow)[x] = pixel;
                        }
                    }
                }
            }
        }

        surf->uniquenessValue++;
        PresentToScreen(surf);
        return DD_OK;
    }

    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surf_BltFast(IDirectDrawSurface7* This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
    LDC_Surface* srcSurf = (LDC_Surface*)lpDDSrcSurface;
    if (!srcSurf) return DDERR_INVALIDPARAMS;

    RECT srcRect;
    if (lpSrcRect) {
        srcRect = *lpSrcRect;
    } else {
        srcRect.left = 0;
        srcRect.top = 0;
        srcRect.right = srcSurf->width;
        srcRect.bottom = srcSurf->height;
    }

    LONG width = srcRect.right - srcRect.left;
    LONG height = srcRect.bottom - srcRect.top;
    RECT dstRect = { (LONG)dwX, (LONG)dwY, (LONG)dwX + width, (LONG)dwY + height };

    DWORD flags = 0;
    if (dwTrans & DDBLTFAST_SRCCOLORKEY) flags |= DDBLT_KEYSRC;
    if (dwTrans & DDBLTFAST_DESTCOLORKEY) flags |= DDBLT_KEYDEST;

    return Surf_Blt(This, &dstRect, lpDDSrcSurface, &srcRect, flags, NULL);
}

static HRESULT STDMETHODCALLTYPE Surf_Flip(IDirectDrawSurface7* This, LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    LDC_UNUSED(lpDDSurfaceTargetOverride);

    if (surf->backBuffer) {
        /* Swap pixel buffers */
        BYTE* temp = surf->pixels;
        surf->pixels = surf->backBuffer->pixels;
        surf->backBuffer->pixels = temp;
    }

    PresentToScreen(surf);

    if (!(dwFlags & DDFLIP_NOVSYNC)) {
        Sleep(1);
    }

    return DD_OK;
}

/* ============================================================================
 * Surface Description Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE Surf_GetSurfaceDesc(IDirectDrawSurface7* This, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    if (!lpDDSurfaceDesc) return DDERR_INVALIDPARAMS;

    ZeroMemory(lpDDSurfaceDesc, sizeof(DDSURFACEDESC2));
    lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC2);
    lpDDSurfaceDesc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;
    lpDDSurfaceDesc->dwWidth = surf->width;
    lpDDSurfaceDesc->dwHeight = surf->height;
    lpDDSurfaceDesc->lPitch = surf->pitch;
    lpDDSurfaceDesc->ddpfPixelFormat = surf->pixelFormat;
    lpDDSurfaceDesc->ddsCaps = surf->caps;

    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surf_GetCaps(IDirectDrawSurface7* This, LPDDSCAPS2 lpDDSCaps)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    if (!lpDDSCaps) return DDERR_INVALIDPARAMS;
    *lpDDSCaps = surf->caps;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surf_GetPixelFormat(IDirectDrawSurface7* This, LPDDPIXELFORMAT lpDDPixelFormat)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    if (!lpDDPixelFormat) return DDERR_INVALIDPARAMS;
    *lpDDPixelFormat = surf->pixelFormat;
    return DD_OK;
}

/* ============================================================================
 * Color Key Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE Surf_GetColorKey(IDirectDrawSurface7* This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    if (!lpDDColorKey) return DDERR_INVALIDPARAMS;

    if (dwFlags & DDCKEY_SRCBLT) {
        if (!surf->hasSrcColorKey) return DDERR_NOCOLORKEY;
        *lpDDColorKey = surf->srcColorKey;
    } else if (dwFlags & DDCKEY_DESTBLT) {
        if (!surf->hasDestColorKey) return DDERR_NOCOLORKEY;
        *lpDDColorKey = surf->destColorKey;
    } else {
        return DDERR_INVALIDPARAMS;
    }

    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surf_SetColorKey(IDirectDrawSurface7* This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
    LDC_Surface* surf = (LDC_Surface*)This;

    if (dwFlags & DDCKEY_SRCBLT) {
        if (lpDDColorKey) {
            surf->srcColorKey = *lpDDColorKey;
            surf->hasSrcColorKey = TRUE;
        } else {
            surf->hasSrcColorKey = FALSE;
        }
    } else if (dwFlags & DDCKEY_DESTBLT) {
        if (lpDDColorKey) {
            surf->destColorKey = *lpDDColorKey;
            surf->hasDestColorKey = TRUE;
        } else {
            surf->hasDestColorKey = FALSE;
        }
    }

    return DD_OK;
}

/* ============================================================================
 * GDI Interop Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE Surf_GetDC(IDirectDrawSurface7* This, HDC* lphDC)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    if (!lphDC) return DDERR_INVALIDPARAMS;
    if (surf->hDC) return DDERR_DCALREADYCREATED;

    HDC hScreenDC = GetDC(NULL);
    surf->hDC = CreateCompatibleDC(hScreenDC);

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = surf->width;
    bmi.bmiHeader.biHeight = -(LONG)surf->height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = (WORD)surf->bpp;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = NULL;
    surf->hBitmap = CreateDIBSection(surf->hDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);

    if (!surf->hBitmap || !pBits) {
        DeleteDC(surf->hDC);
        surf->hDC = NULL;
        ReleaseDC(NULL, hScreenDC);
        return DDERR_GENERIC;
    }

    memcpy(pBits, surf->pixels, surf->pixelSize);
    surf->hBitmapOld = (HBITMAP)SelectObject(surf->hDC, surf->hBitmap);
    ReleaseDC(NULL, hScreenDC);

    *lphDC = surf->hDC;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surf_ReleaseDC(IDirectDrawSurface7* This, HDC hDC)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    if (hDC != surf->hDC || !surf->hDC) return DDERR_INVALIDPARAMS;

    BITMAP bm;
    GetObject(surf->hBitmap, sizeof(bm), &bm);
    if (bm.bmBits) {
        memcpy(surf->pixels, bm.bmBits, surf->pixelSize);
    }

    SelectObject(surf->hDC, surf->hBitmapOld);
    DeleteObject(surf->hBitmap);
    DeleteDC(surf->hDC);

    surf->hDC = NULL;
    surf->hBitmap = NULL;
    surf->hBitmapOld = NULL;

    surf->uniquenessValue++;
    PresentToScreen(surf);

    return DD_OK;
}

/* ============================================================================
 * Palette/Clipper Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE Surf_GetPalette(IDirectDrawSurface7* This, LPDIRECTDRAWPALETTE* lplpDDPalette)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    if (!lplpDDPalette) return DDERR_INVALIDPARAMS;

    if (surf->palette) {
        IDirectDrawPalette* pal = (IDirectDrawPalette*)surf->palette;
        pal->lpVtbl->AddRef(pal);
        *lplpDDPalette = pal;
        return DD_OK;
    }

    *lplpDDPalette = NULL;
    return DDERR_NOPALETTEATTACHED;
}

static HRESULT STDMETHODCALLTYPE Surf_SetPalette(IDirectDrawSurface7* This, LPDIRECTDRAWPALETTE lpDDPalette)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    surf->palette = (LDC_Palette*)lpDDPalette;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surf_GetClipper(IDirectDrawSurface7* This, LPDIRECTDRAWCLIPPER* lplpDDClipper)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    if (!lplpDDClipper) return DDERR_INVALIDPARAMS;

    if (surf->clipper) {
        IDirectDrawClipper* clip = (IDirectDrawClipper*)surf->clipper;
        clip->lpVtbl->AddRef(clip);
        *lplpDDClipper = clip;
        return DD_OK;
    }

    *lplpDDClipper = NULL;
    return DDERR_NOCLIPPERATTACHED;
}

static HRESULT STDMETHODCALLTYPE Surf_SetClipper(IDirectDrawSurface7* This, LPDIRECTDRAWCLIPPER lpDDClipper)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    surf->clipper = (LDC_Clipper*)lpDDClipper;
    return DD_OK;
}

/* ============================================================================
 * Attached Surface Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE Surf_AddAttachedSurface(IDirectDrawSurface7* This, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
    LDC_UNUSED(This);
    LDC_UNUSED(lpDDSAttachedSurface);
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surf_DeleteAttachedSurface(IDirectDrawSurface7* This, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
    LDC_UNUSED(This);
    LDC_UNUSED(dwFlags);
    LDC_UNUSED(lpDDSAttachedSurface);
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surf_EnumAttachedSurfaces(IDirectDrawSurface7* This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback)
{
    LDC_Surface* surf = (LDC_Surface*)This;

    if (surf->backBuffer && lpEnumSurfacesCallback) {
        DDSURFACEDESC2 desc;
        IDirectDrawSurface7* bb = (IDirectDrawSurface7*)surf->backBuffer;
        bb->lpVtbl->GetSurfaceDesc(bb, &desc);
        lpEnumSurfacesCallback(bb, &desc, lpContext);
    }

    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surf_GetAttachedSurface(IDirectDrawSurface7* This, LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7* lplpDDAttachedSurface)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    if (!lplpDDAttachedSurface) return DDERR_INVALIDPARAMS;

    if (surf->backBuffer && (lpDDSCaps->dwCaps & DDSCAPS_BACKBUFFER)) {
        IDirectDrawSurface7* bb = (IDirectDrawSurface7*)surf->backBuffer;
        bb->lpVtbl->AddRef(bb);
        *lplpDDAttachedSurface = bb;
        return DD_OK;
    }

    *lplpDDAttachedSurface = NULL;
    return DDERR_NOTFOUND;
}

/* ============================================================================
 * Stub Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE Surf_AddOverlayDirtyRect(IDirectDrawSurface7* This, LPRECT lpRect) { LDC_UNUSED(This); LDC_UNUSED(lpRect); return DDERR_UNSUPPORTED; }
static HRESULT STDMETHODCALLTYPE Surf_BltBatch(IDirectDrawSurface7* This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags) { LDC_UNUSED(This); LDC_UNUSED(lpDDBltBatch); LDC_UNUSED(dwCount); LDC_UNUSED(dwFlags); return DDERR_UNSUPPORTED; }
static HRESULT STDMETHODCALLTYPE Surf_EnumOverlayZOrders(IDirectDrawSurface7* This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback) { LDC_UNUSED(This); LDC_UNUSED(dwFlags); LDC_UNUSED(lpContext); LDC_UNUSED(lpfnCallback); return DDERR_UNSUPPORTED; }
static HRESULT STDMETHODCALLTYPE Surf_GetBltStatus(IDirectDrawSurface7* This, DWORD dwFlags) { LDC_UNUSED(This); LDC_UNUSED(dwFlags); return DD_OK; }
static HRESULT STDMETHODCALLTYPE Surf_GetFlipStatus(IDirectDrawSurface7* This, DWORD dwFlags) { LDC_UNUSED(This); LDC_UNUSED(dwFlags); return DD_OK; }
static HRESULT STDMETHODCALLTYPE Surf_GetOverlayPosition(IDirectDrawSurface7* This, LPLONG lplX, LPLONG lplY) { LDC_UNUSED(This); LDC_UNUSED(lplX); LDC_UNUSED(lplY); return DDERR_UNSUPPORTED; }
static HRESULT STDMETHODCALLTYPE Surf_Initialize(IDirectDrawSurface7* This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc) { LDC_UNUSED(This); LDC_UNUSED(lpDD); LDC_UNUSED(lpDDSurfaceDesc); return DDERR_ALREADYINITIALIZED; }
static HRESULT STDMETHODCALLTYPE Surf_IsLost(IDirectDrawSurface7* This) { LDC_UNUSED(This); return DD_OK; }
static HRESULT STDMETHODCALLTYPE Surf_Restore(IDirectDrawSurface7* This) { LDC_UNUSED(This); return DD_OK; }
static HRESULT STDMETHODCALLTYPE Surf_SetOverlayPosition(IDirectDrawSurface7* This, LONG lX, LONG lY) { LDC_UNUSED(This); LDC_UNUSED(lX); LDC_UNUSED(lY); return DDERR_UNSUPPORTED; }
static HRESULT STDMETHODCALLTYPE Surf_UpdateOverlay(IDirectDrawSurface7* This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) { LDC_UNUSED(This); LDC_UNUSED(lpSrcRect); LDC_UNUSED(lpDDDestSurface); LDC_UNUSED(lpDestRect); LDC_UNUSED(dwFlags); LDC_UNUSED(lpDDOverlayFx); return DDERR_UNSUPPORTED; }
static HRESULT STDMETHODCALLTYPE Surf_UpdateOverlayDisplay(IDirectDrawSurface7* This, DWORD dwFlags) { LDC_UNUSED(This); LDC_UNUSED(dwFlags); return DDERR_UNSUPPORTED; }
static HRESULT STDMETHODCALLTYPE Surf_UpdateOverlayZOrder(IDirectDrawSurface7* This, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference) { LDC_UNUSED(This); LDC_UNUSED(dwFlags); LDC_UNUSED(lpDDSReference); return DDERR_UNSUPPORTED; }

/* IDirectDrawSurface2+ */
static HRESULT STDMETHODCALLTYPE Surf_GetDDInterface(IDirectDrawSurface7* This, LPVOID* lplpDD)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    if (!lplpDD) return DDERR_INVALIDPARAMS;
    if (surf->parent) {
        IDirectDraw7* dd = (IDirectDraw7*)surf->parent;
        dd->lpVtbl->AddRef(dd);
        *lplpDD = dd;
        return DD_OK;
    }
    return DDERR_INVALIDOBJECT;
}

static HRESULT STDMETHODCALLTYPE Surf_PageLock(IDirectDrawSurface7* This, DWORD dwFlags) { LDC_UNUSED(This); LDC_UNUSED(dwFlags); return DD_OK; }
static HRESULT STDMETHODCALLTYPE Surf_PageUnlock(IDirectDrawSurface7* This, DWORD dwFlags) { LDC_UNUSED(This); LDC_UNUSED(dwFlags); return DD_OK; }

/* IDirectDrawSurface3+ */
static HRESULT STDMETHODCALLTYPE Surf_SetSurfaceDesc(IDirectDrawSurface7* This, LPDDSURFACEDESC2 lpDDSD, DWORD dwFlags) { LDC_UNUSED(This); LDC_UNUSED(lpDDSD); LDC_UNUSED(dwFlags); return DDERR_UNSUPPORTED; }

/* IDirectDrawSurface4+ */
static HRESULT STDMETHODCALLTYPE Surf_SetPrivateData(IDirectDrawSurface7* This, REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags) { LDC_UNUSED(This); LDC_UNUSED(guidTag); LDC_UNUSED(lpData); LDC_UNUSED(cbSize); LDC_UNUSED(dwFlags); return DD_OK; }
static HRESULT STDMETHODCALLTYPE Surf_GetPrivateData(IDirectDrawSurface7* This, REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize) { LDC_UNUSED(This); LDC_UNUSED(guidTag); LDC_UNUSED(lpBuffer); LDC_UNUSED(lpcbBufferSize); return DDERR_NOTFOUND; }
static HRESULT STDMETHODCALLTYPE Surf_FreePrivateData(IDirectDrawSurface7* This, REFGUID guidTag) { LDC_UNUSED(This); LDC_UNUSED(guidTag); return DD_OK; }

static HRESULT STDMETHODCALLTYPE Surf_GetUniquenessValue(IDirectDrawSurface7* This, LPDWORD lpValue)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    if (lpValue) *lpValue = surf->uniquenessValue;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Surf_ChangeUniquenessValue(IDirectDrawSurface7* This)
{
    LDC_Surface* surf = (LDC_Surface*)This;
    surf->uniquenessValue++;
    return DD_OK;
}

/* IDirectDrawSurface7 */
static HRESULT STDMETHODCALLTYPE Surf_SetPriority(IDirectDrawSurface7* This, DWORD dwPriority) { LDC_Surface* surf = (LDC_Surface*)This; surf->priority = dwPriority; return DD_OK; }
static HRESULT STDMETHODCALLTYPE Surf_GetPriority(IDirectDrawSurface7* This, LPDWORD lpdwPriority) { LDC_Surface* surf = (LDC_Surface*)This; if (lpdwPriority) *lpdwPriority = surf->priority; return DD_OK; }
static HRESULT STDMETHODCALLTYPE Surf_SetLOD(IDirectDrawSurface7* This, DWORD dwMaxLOD) { LDC_Surface* surf = (LDC_Surface*)This; surf->lod = dwMaxLOD; return DD_OK; }
static HRESULT STDMETHODCALLTYPE Surf_GetLOD(IDirectDrawSurface7* This, LPDWORD lpdwMaxLOD) { LDC_Surface* surf = (LDC_Surface*)This; if (lpdwMaxLOD) *lpdwMaxLOD = surf->lod; return DD_OK; }

/* ============================================================================
 * VTable
 * ============================================================================ */

static IDirectDrawSurface7Vtbl g_SurfaceVtbl = {
    /* IUnknown */
    Surf_QueryInterface,
    Surf_AddRef,
    Surf_Release,
    /* IDirectDrawSurface */
    Surf_AddAttachedSurface,
    Surf_AddOverlayDirtyRect,
    Surf_Blt,
    Surf_BltBatch,
    Surf_BltFast,
    Surf_DeleteAttachedSurface,
    Surf_EnumAttachedSurfaces,
    Surf_EnumOverlayZOrders,
    Surf_Flip,
    Surf_GetAttachedSurface,
    Surf_GetBltStatus,
    Surf_GetCaps,
    Surf_GetClipper,
    Surf_GetColorKey,
    Surf_GetDC,
    Surf_GetFlipStatus,
    Surf_GetOverlayPosition,
    Surf_GetPalette,
    Surf_GetPixelFormat,
    Surf_GetSurfaceDesc,
    Surf_Initialize,
    Surf_IsLost,
    Surf_Lock,
    Surf_ReleaseDC,
    Surf_Restore,
    Surf_SetClipper,
    Surf_SetColorKey,
    Surf_SetOverlayPosition,
    Surf_SetPalette,
    Surf_Unlock,
    Surf_UpdateOverlay,
    Surf_UpdateOverlayDisplay,
    Surf_UpdateOverlayZOrder,
    /* IDirectDrawSurface2 */
    Surf_GetDDInterface,
    Surf_PageLock,
    Surf_PageUnlock,
    /* IDirectDrawSurface3 */
    Surf_SetSurfaceDesc,
    /* IDirectDrawSurface4 */
    Surf_SetPrivateData,
    Surf_GetPrivateData,
    Surf_FreePrivateData,
    Surf_GetUniquenessValue,
    Surf_ChangeUniquenessValue,
    /* IDirectDrawSurface7 */
    Surf_SetPriority,
    Surf_GetPriority,
    Surf_SetLOD,
    Surf_GetLOD
};

/* ============================================================================
 * Surface Creation
 * ============================================================================ */

HRESULT LDC_CreateSurface(LDC_DirectDraw* dd, LPDDSURFACEDESC2 lpDesc, LPDIRECTDRAWSURFACE7* lplpSurf, IUnknown* pUnkOuter)
{
    if (!lpDesc || !lplpSurf) return DDERR_INVALIDPARAMS;
    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    *lplpSurf = NULL;

    /* Validate size */
    if (lpDesc->dwSize != sizeof(DDSURFACEDESC2) && lpDesc->dwSize != sizeof(DDSURFACEDESC)) {
        return DDERR_INVALIDPARAMS;
    }

    LDC_Surface* surf = (LDC_Surface*)malloc(sizeof(LDC_Surface));
    if (!surf) return DDERR_OUTOFMEMORY;

    ZeroMemory(surf, sizeof(LDC_Surface));
    surf->lpVtbl = &g_SurfaceVtbl;
    surf->refCount = 1;
    surf->parent = dd;

    InitializeCriticalSection(&surf->lockCS);

    /* Copy flags and caps */
    surf->flags = lpDesc->dwFlags;
    if (lpDesc->dwFlags & DDSD_CAPS) {
        surf->caps = lpDesc->ddsCaps;
    }

    /* Determine dimensions */
    if (lpDesc->dwFlags & DDSD_WIDTH) {
        surf->width = lpDesc->dwWidth;
    } else if (IsPrimary(surf)) {
        surf->width = g_ldc.gameWidth;
    }

    if (lpDesc->dwFlags & DDSD_HEIGHT) {
        surf->height = lpDesc->dwHeight;
    } else if (IsPrimary(surf)) {
        surf->height = g_ldc.gameHeight;
    }

    /* Determine pixel format */
    if (lpDesc->dwFlags & DDSD_PIXELFORMAT) {
        surf->pixelFormat = lpDesc->ddpfPixelFormat;
        surf->bpp = lpDesc->ddpfPixelFormat.dwRGBBitCount;
    } else if (IsPrimary(surf)) {
        surf->bpp = g_ldc.gameBpp;
    }

    /* Defaults */
    if (surf->width == 0) surf->width = 640;
    if (surf->height == 0) surf->height = 480;
    if (surf->bpp == 0) surf->bpp = 8;

    /* Calculate pitch (4-byte aligned) */
    surf->pitch = ((surf->width * (surf->bpp / 8) + 3) / 4) * 4;

    /* Initialize pixel format */
    InitializePixelFormat(surf);

    /* Allocate pixel data */
    surf->pixelSize = surf->pitch * surf->height;
    surf->pixels = (BYTE*)malloc(surf->pixelSize);
    if (!surf->pixels) {
        DeleteCriticalSection(&surf->lockCS);
        free(surf);
        return DDERR_OUTOFMEMORY;
    }
    ZeroMemory(surf->pixels, surf->pixelSize);

    /* Handle back buffer */
    if ((lpDesc->dwFlags & DDSD_BACKBUFFERCOUNT) && lpDesc->dwBackBufferCount > 0) {
        DDSURFACEDESC2 backDesc = *lpDesc;
        backDesc.ddsCaps.dwCaps = (backDesc.ddsCaps.dwCaps & ~DDSCAPS_PRIMARYSURFACE) | DDSCAPS_BACKBUFFER;
        backDesc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
        backDesc.dwBackBufferCount = 0;

        HRESULT hr = LDC_CreateSurface(dd, &backDesc, (LPDIRECTDRAWSURFACE7*)&surf->backBuffer, NULL);
        if (FAILED(hr)) {
            free(surf->pixels);
            DeleteCriticalSection(&surf->lockCS);
            free(surf);
            return hr;
        }
        LDC_LOG("Created back buffer for flip chain");
    }

    /* If primary, register with DirectDraw and create render target */
    if (IsPrimary(surf)) {
        LDC_DD_SetPrimarySurface(dd, surf);
        LDC_CreateRenderTarget(surf->width, surf->height, surf->bpp);
        LDC_LOG("Created primary surface %ux%u %ubpp", surf->width, surf->height, surf->bpp);
    } else {
        LDC_LOG("Created surface %ux%u %ubpp", surf->width, surf->height, surf->bpp);
    }

    *lplpSurf = (LPDIRECTDRAWSURFACE7)surf;
    return DD_OK;
}
