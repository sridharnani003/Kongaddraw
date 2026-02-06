/**
 * @file Common.h
 * @brief Common definitions for legacy-ddraw-compat (Pure C implementation)
 */

#ifndef LDC_COMMON_H
#define LDC_COMMON_H

/* Windows headers */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <ddraw.h>

/* Link required libraries */
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "dxguid.lib")

/* ============================================================================
 * Macros
 * ============================================================================ */

#define LDC_UNUSED(x) ((void)(x))

#define LDC_MAX_SURFACES 64
#define LDC_MAX_PALETTES 16
#define LDC_MAX_CLIPPERS 16

/* Safe release macro */
#define LDC_SAFE_RELEASE(p) do { if (p) { (p)->lpVtbl->Release(p); (p) = NULL; } } while(0)

/* ============================================================================
 * Debug Logging
 * ============================================================================ */

#ifdef _DEBUG
#define LDC_LOG(fmt, ...) LDC_DebugLog(fmt, ##__VA_ARGS__)
#else
#define LDC_LOG(fmt, ...) ((void)0)
#endif

void LDC_DebugLog(const char* fmt, ...);

/* ============================================================================
 * Global State Structure
 * ============================================================================ */

#define LDC_PALETTE_SIZE 256

typedef struct LDC_GlobalState {
    /* Module handle */
    HMODULE hModule;
    BOOL initialized;

    /* Window state */
    HWND hWnd;
    WNDPROC originalWndProc;
    DWORD coopLevel;

    /* Game's requested display mode */
    DWORD gameWidth;
    DWORD gameHeight;
    DWORD gameBpp;
    DWORD gameRefresh;
    BOOL displayModeSet;

    /* Actual render target size */
    DWORD renderWidth;
    DWORD renderHeight;

    /* Scaling for mouse coordinates */
    float scaleX;
    float scaleY;
    int offsetX;
    int offsetY;

    /* GDI rendering resources */
    HDC hdcWindow;
    HDC hdcMem;
    HBITMAP hBitmap;
    HBITMAP hBitmapOld;
    void* bitmapBits;
    DWORD bitmapWidth;
    DWORD bitmapHeight;

    /* Palette for 8-bit mode */
    RGBQUAD palette[LDC_PALETTE_SIZE];
    DWORD palette32[LDC_PALETTE_SIZE];
    BOOL paletteChanged;

    /* Primary surface pixel data */
    BYTE* primaryPixels;
    DWORD primarySize;
    DWORD primaryPitch;

    /* Thread safety */
    CRITICAL_SECTION renderLock;

    /* Statistics */
    DWORD frameCount;
    DWORD lastFpsTime;
    DWORD fps;

} LDC_GlobalState;

/* Global state instance */
extern LDC_GlobalState g_ldc;

/* ============================================================================
 * Initialization / Cleanup
 * ============================================================================ */

BOOL LDC_Initialize(void);
void LDC_Shutdown(void);

/* ============================================================================
 * Rendering Functions
 * ============================================================================ */

BOOL LDC_CreateRenderTarget(DWORD width, DWORD height, DWORD bpp);
void LDC_DestroyRenderTarget(void);
void LDC_PresentToScreen(const BYTE* pixels, DWORD width, DWORD height, DWORD pitch, DWORD bpp);

/* ============================================================================
 * Window Management
 * ============================================================================ */

void LDC_SubclassWindow(HWND hWnd);
void LDC_UnsubclassWindow(void);
void LDC_UpdateScaling(void);
POINT LDC_TransformMouseToGame(POINT pt);

/* ============================================================================
 * Forward Declarations for COM Objects
 * ============================================================================ */

typedef struct LDC_DirectDraw LDC_DirectDraw;
typedef struct LDC_Surface LDC_Surface;
typedef struct LDC_Palette LDC_Palette;
typedef struct LDC_Clipper LDC_Clipper;

/* ============================================================================
 * DirectDraw Object Creation
 * ============================================================================ */

HRESULT LDC_CreateDirectDraw(GUID* lpGUID, IDirectDraw7** lplpDD, IUnknown* pUnkOuter);

#endif /* LDC_COMMON_H */
