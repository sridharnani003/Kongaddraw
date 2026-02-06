/**
 * @file dllmain.c
 * @brief DLL entry point, GDI rendering, and exports for legacy-ddraw-compat
 */

#include "core/Common.h"
#include <stdio.h>
#include <stdarg.h>

/* ============================================================================
 * Global State
 * ============================================================================ */

LDC_GlobalState g_ldc = {0};

/* ============================================================================
 * Debug Logging
 * ============================================================================ */

void LDC_DebugLog(const char* fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    OutputDebugStringA("[LDC] ");
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}

/* ============================================================================
 * GDI Rendering
 * ============================================================================ */

BOOL LDC_CreateRenderTarget(DWORD width, DWORD height, DWORD bpp)
{
    EnterCriticalSection(&g_ldc.renderLock);

    LDC_LOG("CreateRenderTarget: %ux%u %ubpp", width, height, bpp);

    /* Clean up existing */
    LDC_DestroyRenderTarget();

    /* Store dimensions */
    g_ldc.gameWidth = width;
    g_ldc.gameHeight = height;
    g_ldc.gameBpp = bpp;
    g_ldc.bitmapWidth = width;
    g_ldc.bitmapHeight = height;

    /* Calculate pitch (4-byte aligned) */
    g_ldc.primaryPitch = ((width * (bpp / 8) + 3) / 4) * 4;

    /* Allocate primary pixel buffer */
    g_ldc.primarySize = g_ldc.primaryPitch * height;
    g_ldc.primaryPixels = (BYTE*)malloc(g_ldc.primarySize);
    if (!g_ldc.primaryPixels) {
        LeaveCriticalSection(&g_ldc.renderLock);
        return FALSE;
    }
    ZeroMemory(g_ldc.primaryPixels, g_ldc.primarySize);

    /* Get window DC */
    if (g_ldc.hWnd) {
        g_ldc.hdcWindow = GetDC(g_ldc.hWnd);
        if (!g_ldc.hdcWindow) {
            LDC_LOG("Failed to get window DC");
            free(g_ldc.primaryPixels);
            g_ldc.primaryPixels = NULL;
            LeaveCriticalSection(&g_ldc.renderLock);
            return FALSE;
        }

        /* Create compatible DC */
        g_ldc.hdcMem = CreateCompatibleDC(g_ldc.hdcWindow);
        if (!g_ldc.hdcMem) {
            LDC_LOG("Failed to create compatible DC");
            ReleaseDC(g_ldc.hWnd, g_ldc.hdcWindow);
            g_ldc.hdcWindow = NULL;
            free(g_ldc.primaryPixels);
            g_ldc.primaryPixels = NULL;
            LeaveCriticalSection(&g_ldc.renderLock);
            return FALSE;
        }

        /* Create 32-bit DIB section */
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -(LONG)height;  /* Top-down */
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        g_ldc.hBitmap = CreateDIBSection(g_ldc.hdcMem, &bmi, DIB_RGB_COLORS, &g_ldc.bitmapBits, NULL, 0);
        if (!g_ldc.hBitmap || !g_ldc.bitmapBits) {
            LDC_LOG("Failed to create DIB section");
            DeleteDC(g_ldc.hdcMem);
            ReleaseDC(g_ldc.hWnd, g_ldc.hdcWindow);
            g_ldc.hdcMem = NULL;
            g_ldc.hdcWindow = NULL;
            free(g_ldc.primaryPixels);
            g_ldc.primaryPixels = NULL;
            LeaveCriticalSection(&g_ldc.renderLock);
            return FALSE;
        }

        g_ldc.hBitmapOld = (HBITMAP)SelectObject(g_ldc.hdcMem, g_ldc.hBitmap);
    }

    /* Initialize palette to grayscale */
    {
        int i;
        for (i = 0; i < 256; i++) {
            g_ldc.palette[i].rgbRed = (BYTE)i;
            g_ldc.palette[i].rgbGreen = (BYTE)i;
            g_ldc.palette[i].rgbBlue = (BYTE)i;
            g_ldc.palette[i].rgbReserved = 0;
            g_ldc.palette32[i] = 0xFF000000 | (i << 16) | (i << 8) | i;
        }
    }

    LDC_UpdateScaling();

    LDC_LOG("Render target created successfully");
    LeaveCriticalSection(&g_ldc.renderLock);
    return TRUE;
}

void LDC_DestroyRenderTarget(void)
{
    /* Note: Caller should hold renderLock or call during shutdown */

    if (g_ldc.hdcMem) {
        if (g_ldc.hBitmapOld) {
            SelectObject(g_ldc.hdcMem, g_ldc.hBitmapOld);
            g_ldc.hBitmapOld = NULL;
        }
        DeleteDC(g_ldc.hdcMem);
        g_ldc.hdcMem = NULL;
    }

    if (g_ldc.hBitmap) {
        DeleteObject(g_ldc.hBitmap);
        g_ldc.hBitmap = NULL;
    }

    if (g_ldc.hdcWindow && g_ldc.hWnd) {
        ReleaseDC(g_ldc.hWnd, g_ldc.hdcWindow);
        g_ldc.hdcWindow = NULL;
    }

    g_ldc.bitmapBits = NULL;

    if (g_ldc.primaryPixels) {
        free(g_ldc.primaryPixels);
        g_ldc.primaryPixels = NULL;
    }
}

void LDC_PresentToScreen(const BYTE* pixels, DWORD width, DWORD height, DWORD pitch, DWORD bpp)
{
    if (!g_ldc.hdcWindow || !g_ldc.hdcMem || !g_ldc.bitmapBits) return;
    if (!pixels) return;

    DWORD* dstPixels = (DWORD*)g_ldc.bitmapBits;
    DWORD x, y;

    /* Convert source pixels to 32-bit BGRA */
    if (bpp == 8) {
        /* 8-bit palettized */
        for (y = 0; y < height; y++) {
            const BYTE* srcRow = pixels + y * pitch;
            DWORD* dstRow = dstPixels + y * width;
            for (x = 0; x < width; x++) {
                dstRow[x] = g_ldc.palette32[srcRow[x]];
            }
        }
    }
    else if (bpp == 16) {
        /* 16-bit RGB565 */
        for (y = 0; y < height; y++) {
            const WORD* srcRow = (const WORD*)(pixels + y * pitch);
            DWORD* dstRow = dstPixels + y * width;
            for (x = 0; x < width; x++) {
                WORD pixel = srcRow[x];
                BYTE r = ((pixel >> 11) & 0x1F) << 3;
                BYTE g = ((pixel >> 5) & 0x3F) << 2;
                BYTE b = (pixel & 0x1F) << 3;
                dstRow[x] = 0xFF000000 | ((DWORD)r << 16) | ((DWORD)g << 8) | b;
            }
        }
    }
    else if (bpp == 24) {
        /* 24-bit BGR */
        for (y = 0; y < height; y++) {
            const BYTE* srcRow = pixels + y * pitch;
            DWORD* dstRow = dstPixels + y * width;
            for (x = 0; x < width; x++) {
                BYTE b = srcRow[x * 3 + 0];
                BYTE g = srcRow[x * 3 + 1];
                BYTE r = srcRow[x * 3 + 2];
                dstRow[x] = 0xFF000000 | ((DWORD)r << 16) | ((DWORD)g << 8) | b;
            }
        }
    }
    else if (bpp == 32) {
        /* 32-bit - direct copy */
        for (y = 0; y < height; y++) {
            const DWORD* srcRow = (const DWORD*)(pixels + y * pitch);
            DWORD* dstRow = dstPixels + y * width;
            memcpy(dstRow, srcRow, width * sizeof(DWORD));
        }
    }

    /* Blit to window */
    RECT clientRect;
    GetClientRect(g_ldc.hWnd, &clientRect);
    int windowWidth = clientRect.right - clientRect.left;
    int windowHeight = clientRect.bottom - clientRect.top;

    if (windowWidth == (int)width && windowHeight == (int)height) {
        BitBlt(g_ldc.hdcWindow, 0, 0, width, height, g_ldc.hdcMem, 0, 0, SRCCOPY);
    }
    else {
        SetStretchBltMode(g_ldc.hdcWindow, HALFTONE);
        SetBrushOrgEx(g_ldc.hdcWindow, 0, 0, NULL);
        StretchBlt(g_ldc.hdcWindow, 0, 0, windowWidth, windowHeight,
                   g_ldc.hdcMem, 0, 0, width, height, SRCCOPY);
    }

    /* Update FPS counter */
    g_ldc.frameCount++;
    DWORD now = GetTickCount();
    if (now - g_ldc.lastFpsTime >= 1000) {
        g_ldc.fps = g_ldc.frameCount;
        g_ldc.frameCount = 0;
        g_ldc.lastFpsTime = now;
    }
}

/* ============================================================================
 * Window Management
 * ============================================================================ */

void LDC_UpdateScaling(void)
{
    if (!g_ldc.hWnd) {
        g_ldc.scaleX = 1.0f;
        g_ldc.scaleY = 1.0f;
        g_ldc.offsetX = 0;
        g_ldc.offsetY = 0;
        return;
    }

    RECT clientRect;
    GetClientRect(g_ldc.hWnd, &clientRect);
    int windowWidth = clientRect.right - clientRect.left;
    int windowHeight = clientRect.bottom - clientRect.top;

    if (windowWidth <= 0) windowWidth = 1;
    if (windowHeight <= 0) windowHeight = 1;

    g_ldc.renderWidth = windowWidth;
    g_ldc.renderHeight = windowHeight;

    g_ldc.scaleX = (float)g_ldc.gameWidth / windowWidth;
    g_ldc.scaleY = (float)g_ldc.gameHeight / windowHeight;
    g_ldc.offsetX = 0;
    g_ldc.offsetY = 0;
}

POINT LDC_TransformMouseToGame(POINT pt)
{
    POINT result;
    result.x = (LONG)((pt.x - g_ldc.offsetX) * g_ldc.scaleX);
    result.y = (LONG)((pt.y - g_ldc.offsetY) * g_ldc.scaleY);

    /* Clamp to game bounds */
    if (result.x < 0) result.x = 0;
    if (result.y < 0) result.y = 0;
    if (result.x >= (LONG)g_ldc.gameWidth) result.x = g_ldc.gameWidth - 1;
    if (result.y >= (LONG)g_ldc.gameHeight) result.y = g_ldc.gameHeight - 1;

    return result;
}

static LRESULT CALLBACK LDC_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_SIZE:
            LDC_UpdateScaling();
            break;

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            pt = LDC_TransformMouseToGame(pt);
            lParam = MAKELPARAM(pt.x, pt.y);
            break;
        }
    }

    if (g_ldc.originalWndProc) {
        return CallWindowProcA(g_ldc.originalWndProc, hWnd, msg, wParam, lParam);
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

void LDC_SubclassWindow(HWND hWnd)
{
    if (g_ldc.originalWndProc) return;  /* Already subclassed */

    g_ldc.originalWndProc = (WNDPROC)SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR)LDC_WndProc);
    LDC_LOG("Window subclassed: %p", hWnd);
}

void LDC_UnsubclassWindow(void)
{
    if (g_ldc.originalWndProc && g_ldc.hWnd) {
        SetWindowLongPtrA(g_ldc.hWnd, GWLP_WNDPROC, (LONG_PTR)g_ldc.originalWndProc);
        g_ldc.originalWndProc = NULL;
        LDC_LOG("Window unsubclassed");
    }
}

/* ============================================================================
 * Initialization / Cleanup
 * ============================================================================ */

BOOL LDC_Initialize(void)
{
    if (g_ldc.initialized) return TRUE;

    LDC_LOG("legacy-ddraw-compat initializing...");

    InitializeCriticalSection(&g_ldc.renderLock);

    /* Set default values */
    g_ldc.gameWidth = 640;
    g_ldc.gameHeight = 480;
    g_ldc.gameBpp = 8;
    g_ldc.scaleX = 1.0f;
    g_ldc.scaleY = 1.0f;

    timeBeginPeriod(1);

    g_ldc.initialized = TRUE;
    LDC_LOG("legacy-ddraw-compat initialized");

    return TRUE;
}

void LDC_Shutdown(void)
{
    if (!g_ldc.initialized) return;

    LDC_LOG("legacy-ddraw-compat shutting down...");

    LDC_UnsubclassWindow();

    EnterCriticalSection(&g_ldc.renderLock);
    LDC_DestroyRenderTarget();
    LeaveCriticalSection(&g_ldc.renderLock);

    DeleteCriticalSection(&g_ldc.renderLock);

    timeEndPeriod(1);

    g_ldc.initialized = FALSE;
}

/* ============================================================================
 * DirectDraw API Exports
 * ============================================================================ */

HRESULT WINAPI DirectDrawCreate(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter)
{
    LDC_LOG("DirectDrawCreate called");

    if (!lplpDD) return DDERR_INVALIDPARAMS;

    IDirectDraw7* dd7 = NULL;
    HRESULT hr = LDC_CreateDirectDraw(lpGUID, &dd7, pUnkOuter);
    if (FAILED(hr)) return hr;

    /* Return IDirectDraw interface (same object, will handle via QueryInterface) */
    *lplpDD = (LPDIRECTDRAW)dd7;
    return DD_OK;
}

HRESULT WINAPI DirectDrawCreateEx(GUID* lpGUID, LPVOID* lplpDD, REFIID iid, IUnknown* pUnkOuter)
{
    LDC_LOG("DirectDrawCreateEx called");

    if (!lplpDD) return DDERR_INVALIDPARAMS;

    IDirectDraw7* dd7 = NULL;
    HRESULT hr = LDC_CreateDirectDraw(lpGUID, &dd7, pUnkOuter);
    if (FAILED(hr)) return hr;

    /* Query for the requested interface */
    hr = dd7->lpVtbl->QueryInterface(dd7, iid, lplpDD);
    dd7->lpVtbl->Release(dd7);

    return hr;
}

HRESULT WINAPI DirectDrawCreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpDDClipper, IUnknown* pUnkOuter)
{
    LDC_LOG("DirectDrawCreateClipper called");
    return LDC_CreateClipper(NULL, dwFlags, lplpDDClipper, pUnkOuter);
}

/* Forward declaration */
HRESULT LDC_CreateClipper(LDC_DirectDraw* dd, DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpClip, IUnknown* pUnkOuter);

/* Enumeration callback type */
typedef BOOL (WINAPI *LPDDENUMCALLBACKA)(GUID*, LPSTR, LPSTR, LPVOID);
typedef BOOL (WINAPI *LPDDENUMCALLBACKEXA)(GUID*, LPSTR, LPSTR, LPVOID, HMONITOR);

HRESULT WINAPI DirectDrawEnumerateA(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext)
{
    LDC_LOG("DirectDrawEnumerateA called");

    if (!lpCallback) return DDERR_INVALIDPARAMS;

    /* Enumerate primary display */
    char driverName[] = "display";
    char description[] = "Primary Display Driver";
    lpCallback(NULL, description, driverName, lpContext);

    return DD_OK;
}

HRESULT WINAPI DirectDrawEnumerateW(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext)
{
    LDC_LOG("DirectDrawEnumerateW called");
    /* Just call the ANSI version */
    return DirectDrawEnumerateA(lpCallback, lpContext);
}

HRESULT WINAPI DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags)
{
    LDC_LOG("DirectDrawEnumerateExA called: flags=0x%08X", dwFlags);

    if (!lpCallback) return DDERR_INVALIDPARAMS;

    /* Enumerate primary display */
    char driverName[] = "display";
    char description[] = "Primary Display Driver";
    lpCallback(NULL, description, driverName, lpContext, NULL);

    return DD_OK;
}

HRESULT WINAPI DirectDrawEnumerateExW(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags)
{
    LDC_LOG("DirectDrawEnumerateExW called: flags=0x%08X", dwFlags);
    return DirectDrawEnumerateExA(lpCallback, lpContext, dwFlags);
}

/* DllGetClassObject - required for COM */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    LDC_UNUSED(rclsid);
    LDC_UNUSED(riid);
    LDC_UNUSED(ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/* DllCanUnloadNow - required for COM */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/* ============================================================================
 * DLL Entry Point
 * ============================================================================ */

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    LDC_UNUSED(lpReserved);

    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            g_ldc.hModule = hModule;
            DisableThreadLibraryCalls(hModule);
            LDC_Initialize();
            break;

        case DLL_PROCESS_DETACH:
            LDC_Shutdown();
            break;
    }

    return TRUE;
}
