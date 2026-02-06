/**
 * @file palette.c
 * @brief IDirectDrawPalette implementation in pure C
 */

#include "core/Common.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * LDC_Palette Structure
 * ============================================================================ */

struct LDC_Palette {
    IDirectDrawPaletteVtbl* lpVtbl;
    LONG refCount;
    DWORD flags;
    PALETTEENTRY entries[256];
};

/* ============================================================================
 * IUnknown Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE Pal_QueryInterface(IDirectDrawPalette* This, REFIID riid, void** ppvObj)
{
    LDC_Palette* pal = (LDC_Palette*)This;

    if (!ppvObj) return E_POINTER;
    *ppvObj = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectDrawPalette)) {
        InterlockedIncrement(&pal->refCount);
        *ppvObj = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE Pal_AddRef(IDirectDrawPalette* This)
{
    LDC_Palette* pal = (LDC_Palette*)This;
    return InterlockedIncrement(&pal->refCount);
}

static ULONG STDMETHODCALLTYPE Pal_Release(IDirectDrawPalette* This)
{
    LDC_Palette* pal = (LDC_Palette*)This;
    LONG ref = InterlockedDecrement(&pal->refCount);

    if (ref == 0) {
        LDC_LOG("Palette destroyed");
        free(pal);
    }

    return ref;
}

/* ============================================================================
 * IDirectDrawPalette Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE Pal_GetCaps(IDirectDrawPalette* This, LPDWORD lpdwCaps)
{
    LDC_Palette* pal = (LDC_Palette*)This;
    if (!lpdwCaps) return DDERR_INVALIDPARAMS;
    *lpdwCaps = pal->flags;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Pal_GetEntries(IDirectDrawPalette* This, DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries)
{
    LDC_Palette* pal = (LDC_Palette*)This;
    LDC_UNUSED(dwFlags);

    if (!lpEntries) return DDERR_INVALIDPARAMS;
    if (dwBase >= 256) return DDERR_INVALIDPARAMS;
    if (dwBase + dwNumEntries > 256) dwNumEntries = 256 - dwBase;

    memcpy(lpEntries, &pal->entries[dwBase], dwNumEntries * sizeof(PALETTEENTRY));
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Pal_Initialize(IDirectDrawPalette* This, LPDIRECTDRAW lpDD, DWORD dwFlags, LPPALETTEENTRY lpDDColorTable)
{
    LDC_UNUSED(This);
    LDC_UNUSED(lpDD);
    LDC_UNUSED(dwFlags);
    LDC_UNUSED(lpDDColorTable);
    return DDERR_ALREADYINITIALIZED;
}

static HRESULT STDMETHODCALLTYPE Pal_SetEntries(IDirectDrawPalette* This, DWORD dwFlags, DWORD dwStartingEntry, DWORD dwCount, LPPALETTEENTRY lpEntries)
{
    LDC_Palette* pal = (LDC_Palette*)This;
    LDC_UNUSED(dwFlags);

    if (!lpEntries) return DDERR_INVALIDPARAMS;
    if (dwStartingEntry >= 256) return DDERR_INVALIDPARAMS;
    if (dwStartingEntry + dwCount > 256) dwCount = 256 - dwStartingEntry;

    memcpy(&pal->entries[dwStartingEntry], lpEntries, dwCount * sizeof(PALETTEENTRY));

    /* Update global palette */
    DWORD i;
    for (i = dwStartingEntry; i < dwStartingEntry + dwCount; i++) {
        g_ldc.palette[i].rgbRed = pal->entries[i].peRed;
        g_ldc.palette[i].rgbGreen = pal->entries[i].peGreen;
        g_ldc.palette[i].rgbBlue = pal->entries[i].peBlue;
        g_ldc.palette[i].rgbReserved = 0;
        g_ldc.palette32[i] = 0xFF000000 |
            ((DWORD)pal->entries[i].peRed << 16) |
            ((DWORD)pal->entries[i].peGreen << 8) |
            (DWORD)pal->entries[i].peBlue;
    }
    g_ldc.paletteChanged = TRUE;

    return DD_OK;
}

/* ============================================================================
 * VTable
 * ============================================================================ */

static IDirectDrawPaletteVtbl g_PaletteVtbl = {
    Pal_QueryInterface,
    Pal_AddRef,
    Pal_Release,
    Pal_GetCaps,
    Pal_GetEntries,
    Pal_Initialize,
    Pal_SetEntries
};

/* ============================================================================
 * Palette Creation
 * ============================================================================ */

HRESULT LDC_CreatePalette(LDC_DirectDraw* dd, DWORD dwFlags, LPPALETTEENTRY lpEntries, LPDIRECTDRAWPALETTE* lplpPal, IUnknown* pUnkOuter)
{
    LDC_UNUSED(dd);

    if (!lplpPal) return DDERR_INVALIDPARAMS;
    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    *lplpPal = NULL;

    LDC_Palette* pal = (LDC_Palette*)malloc(sizeof(LDC_Palette));
    if (!pal) return DDERR_OUTOFMEMORY;

    ZeroMemory(pal, sizeof(LDC_Palette));
    pal->lpVtbl = &g_PaletteVtbl;
    pal->refCount = 1;
    pal->flags = dwFlags;

    /* Initialize entries */
    if (lpEntries) {
        DWORD count = (dwFlags & DDPCAPS_8BIT) ? 256 : 16;
        memcpy(pal->entries, lpEntries, count * sizeof(PALETTEENTRY));

        /* Update global palette */
        DWORD i;
        for (i = 0; i < count; i++) {
            g_ldc.palette[i].rgbRed = lpEntries[i].peRed;
            g_ldc.palette[i].rgbGreen = lpEntries[i].peGreen;
            g_ldc.palette[i].rgbBlue = lpEntries[i].peBlue;
            g_ldc.palette[i].rgbReserved = 0;
            g_ldc.palette32[i] = 0xFF000000 |
                ((DWORD)lpEntries[i].peRed << 16) |
                ((DWORD)lpEntries[i].peGreen << 8) |
                (DWORD)lpEntries[i].peBlue;
        }
        g_ldc.paletteChanged = TRUE;
    }

    LDC_LOG("Palette created");

    *lplpPal = (LPDIRECTDRAWPALETTE)pal;
    return DD_OK;
}
