/**
 * @file clipper.c
 * @brief IDirectDrawClipper implementation in pure C
 */

#include "core/Common.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * LDC_Clipper Structure
 * ============================================================================ */

struct LDC_Clipper {
    IDirectDrawClipperVtbl* lpVtbl;
    LONG refCount;
    HWND hWnd;
    BYTE* clipList;
    DWORD clipListSize;
};

/* ============================================================================
 * IUnknown Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE Clip_QueryInterface(IDirectDrawClipper* This, REFIID riid, void** ppvObj)
{
    LDC_Clipper* clip = (LDC_Clipper*)This;

    if (!ppvObj) return E_POINTER;
    *ppvObj = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectDrawClipper)) {
        InterlockedIncrement(&clip->refCount);
        *ppvObj = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE Clip_AddRef(IDirectDrawClipper* This)
{
    LDC_Clipper* clip = (LDC_Clipper*)This;
    return InterlockedIncrement(&clip->refCount);
}

static ULONG STDMETHODCALLTYPE Clip_Release(IDirectDrawClipper* This)
{
    LDC_Clipper* clip = (LDC_Clipper*)This;
    LONG ref = InterlockedDecrement(&clip->refCount);

    if (ref == 0) {
        LDC_LOG("Clipper destroyed");
        if (clip->clipList) free(clip->clipList);
        free(clip);
    }

    return ref;
}

/* ============================================================================
 * IDirectDrawClipper Methods
 * ============================================================================ */

static HRESULT STDMETHODCALLTYPE Clip_GetClipList(IDirectDrawClipper* This, LPRECT lpRect, LPRGNDATA lpClipList, LPDWORD lpdwSize)
{
    LDC_Clipper* clip = (LDC_Clipper*)This;
    LDC_UNUSED(lpRect);

    if (!lpdwSize) return DDERR_INVALIDPARAMS;

    /* If window is set, generate clip list from window */
    if (clip->hWnd) {
        RECT clientRect;
        GetClientRect(clip->hWnd, &clientRect);

        DWORD size = sizeof(RGNDATAHEADER) + sizeof(RECT);

        if (!lpClipList) {
            *lpdwSize = size;
            return DD_OK;
        }

        if (*lpdwSize < size) {
            *lpdwSize = size;
            return DDERR_REGIONTOOSMALL;
        }

        lpClipList->rdh.dwSize = sizeof(RGNDATAHEADER);
        lpClipList->rdh.iType = RDH_RECTANGLES;
        lpClipList->rdh.nCount = 1;
        lpClipList->rdh.nRgnSize = sizeof(RECT);
        lpClipList->rdh.rcBound = clientRect;
        memcpy(lpClipList->Buffer, &clientRect, sizeof(RECT));

        *lpdwSize = size;
        return DD_OK;
    }

    /* Return stored clip list */
    if (!lpClipList) {
        *lpdwSize = clip->clipListSize;
        return DD_OK;
    }

    if (*lpdwSize < clip->clipListSize) {
        *lpdwSize = clip->clipListSize;
        return DDERR_REGIONTOOSMALL;
    }

    if (clip->clipList && clip->clipListSize > 0) {
        memcpy(lpClipList, clip->clipList, clip->clipListSize);
    }

    *lpdwSize = clip->clipListSize;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Clip_GetHWnd(IDirectDrawClipper* This, HWND* lphWnd)
{
    LDC_Clipper* clip = (LDC_Clipper*)This;
    if (!lphWnd) return DDERR_INVALIDPARAMS;
    *lphWnd = clip->hWnd;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Clip_Initialize(IDirectDrawClipper* This, LPDIRECTDRAW lpDD, DWORD dwFlags)
{
    LDC_UNUSED(This);
    LDC_UNUSED(lpDD);
    LDC_UNUSED(dwFlags);
    return DDERR_ALREADYINITIALIZED;
}

static HRESULT STDMETHODCALLTYPE Clip_IsClipListChanged(IDirectDrawClipper* This, BOOL* lpbChanged)
{
    LDC_UNUSED(This);
    if (!lpbChanged) return DDERR_INVALIDPARAMS;
    *lpbChanged = FALSE;
    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Clip_SetClipList(IDirectDrawClipper* This, LPRGNDATA lpClipList, DWORD dwFlags)
{
    LDC_Clipper* clip = (LDC_Clipper*)This;
    LDC_UNUSED(dwFlags);

    /* Free existing clip list */
    if (clip->clipList) {
        free(clip->clipList);
        clip->clipList = NULL;
        clip->clipListSize = 0;
    }

    if (lpClipList) {
        DWORD size = sizeof(RGNDATAHEADER) + lpClipList->rdh.nCount * sizeof(RECT);
        clip->clipList = (BYTE*)malloc(size);
        if (!clip->clipList) return DDERR_OUTOFMEMORY;
        memcpy(clip->clipList, lpClipList, size);
        clip->clipListSize = size;
    }

    return DD_OK;
}

static HRESULT STDMETHODCALLTYPE Clip_SetHWnd(IDirectDrawClipper* This, DWORD dwFlags, HWND hWnd)
{
    LDC_Clipper* clip = (LDC_Clipper*)This;
    LDC_UNUSED(dwFlags);
    clip->hWnd = hWnd;
    return DD_OK;
}

/* ============================================================================
 * VTable
 * ============================================================================ */

static IDirectDrawClipperVtbl g_ClipperVtbl = {
    Clip_QueryInterface,
    Clip_AddRef,
    Clip_Release,
    Clip_GetClipList,
    Clip_GetHWnd,
    Clip_Initialize,
    Clip_IsClipListChanged,
    Clip_SetClipList,
    Clip_SetHWnd
};

/* ============================================================================
 * Clipper Creation
 * ============================================================================ */

HRESULT LDC_CreateClipper(LDC_DirectDraw* dd, DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpClip, IUnknown* pUnkOuter)
{
    LDC_UNUSED(dd);
    LDC_UNUSED(dwFlags);

    if (!lplpClip) return DDERR_INVALIDPARAMS;
    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    *lplpClip = NULL;

    LDC_Clipper* clip = (LDC_Clipper*)malloc(sizeof(LDC_Clipper));
    if (!clip) return DDERR_OUTOFMEMORY;

    ZeroMemory(clip, sizeof(LDC_Clipper));
    clip->lpVtbl = &g_ClipperVtbl;
    clip->refCount = 1;

    LDC_LOG("Clipper created");

    *lplpClip = (LPDIRECTDRAWCLIPPER)clip;
    return DD_OK;
}
