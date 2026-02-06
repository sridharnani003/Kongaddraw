/**
 * @file DllMain.cpp
 * @brief DLL entry point and core implementation for legacy-ddraw-compat
 */

#include "core/Common.h"

using namespace ldc;

// ============================================================================
// Global State
// ============================================================================

namespace ldc {
    GlobalState g_state;
}

// ============================================================================
// Rendering Implementation
// ============================================================================

bool ldc::CreateRenderTarget(DWORD width, DWORD height, DWORD bpp) {
    std::lock_guard<std::recursive_mutex> lock(g_state.renderMutex);

    DebugLog("CreateRenderTarget: %ux%u %ubpp", width, height, bpp);

    // Clean up existing resources
    DestroyRenderTarget();

    // Store dimensions
    g_state.gameWidth = width;
    g_state.gameHeight = height;
    g_state.gameBpp = bpp;
    g_state.bitmapWidth = width;
    g_state.bitmapHeight = height;

    // Calculate pitch (align to 4 bytes)
    g_state.primaryPitch = ((width * (bpp / 8) + 3) / 4) * 4;

    // Allocate primary pixel buffer
    g_state.primaryPixels.resize(g_state.primaryPitch * height);
    std::memset(g_state.primaryPixels.data(), 0, g_state.primaryPixels.size());

    // Allocate 32-bit render buffer for display
    g_state.renderBuffer.resize(width * height);

    // Get window DC
    if (g_state.hWnd) {
        g_state.hdcWindow = GetDC(g_state.hWnd);
        if (!g_state.hdcWindow) {
            DebugLog("Failed to get window DC");
            return false;
        }

        // Create compatible DC
        g_state.hdcMem = CreateCompatibleDC(g_state.hdcWindow);
        if (!g_state.hdcMem) {
            DebugLog("Failed to create compatible DC");
            ReleaseDC(g_state.hWnd, g_state.hdcWindow);
            g_state.hdcWindow = nullptr;
            return false;
        }

        // Create 32-bit DIB section for rendering
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -static_cast<LONG>(height);  // Top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        g_state.hBitmap = CreateDIBSection(
            g_state.hdcMem,
            &bmi,
            DIB_RGB_COLORS,
            &g_state.bitmapBits,
            nullptr,
            0
        );

        if (!g_state.hBitmap || !g_state.bitmapBits) {
            DebugLog("Failed to create DIB section");
            DeleteDC(g_state.hdcMem);
            ReleaseDC(g_state.hWnd, g_state.hdcWindow);
            g_state.hdcMem = nullptr;
            g_state.hdcWindow = nullptr;
            return false;
        }

        g_state.hBitmapOld = (HBITMAP)SelectObject(g_state.hdcMem, g_state.hBitmap);
    }

    // Initialize palette to grayscale
    for (int i = 0; i < 256; ++i) {
        g_state.palette[i].rgbRed = static_cast<BYTE>(i);
        g_state.palette[i].rgbGreen = static_cast<BYTE>(i);
        g_state.palette[i].rgbBlue = static_cast<BYTE>(i);
        g_state.palette[i].rgbReserved = 0;
        g_state.palette32[i] = 0xFF000000 | (i << 16) | (i << 8) | i;
    }

    // Update scaling
    UpdateScaling();

    DebugLog("Render target created successfully");
    return true;
}

void ldc::DestroyRenderTarget() {
    std::lock_guard<std::recursive_mutex> lock(g_state.renderMutex);

    if (g_state.hdcMem) {
        if (g_state.hBitmapOld) {
            SelectObject(g_state.hdcMem, g_state.hBitmapOld);
            g_state.hBitmapOld = nullptr;
        }
        DeleteDC(g_state.hdcMem);
        g_state.hdcMem = nullptr;
    }

    if (g_state.hBitmap) {
        DeleteObject(g_state.hBitmap);
        g_state.hBitmap = nullptr;
    }

    if (g_state.hdcWindow && g_state.hWnd) {
        ReleaseDC(g_state.hWnd, g_state.hdcWindow);
        g_state.hdcWindow = nullptr;
    }

    g_state.bitmapBits = nullptr;
    g_state.primaryPixels.clear();
    g_state.renderBuffer.clear();
}

void ldc::PresentPrimaryToScreen() {
    std::lock_guard<std::recursive_mutex> lock(g_state.renderMutex);

    if (!g_state.hdcWindow || !g_state.hdcMem || !g_state.bitmapBits) {
        return;
    }

    if (g_state.primaryPixels.empty()) {
        return;
    }

    const uint8_t* srcPixels = g_state.primaryPixels.data();
    uint32_t* dstPixels = static_cast<uint32_t*>(g_state.bitmapBits);
    DWORD width = g_state.gameWidth;
    DWORD height = g_state.gameHeight;
    DWORD pitch = g_state.primaryPitch;
    DWORD bpp = g_state.gameBpp;

    // Convert source pixels to 32-bit BGRA
    if (bpp == 8) {
        // 8-bit palettized
        for (DWORD y = 0; y < height; ++y) {
            const uint8_t* srcRow = srcPixels + y * pitch;
            uint32_t* dstRow = dstPixels + y * width;
            for (DWORD x = 0; x < width; ++x) {
                dstRow[x] = g_state.palette32[srcRow[x]];
            }
        }
    }
    else if (bpp == 16) {
        // 16-bit RGB565
        for (DWORD y = 0; y < height; ++y) {
            const uint16_t* srcRow = reinterpret_cast<const uint16_t*>(srcPixels + y * pitch);
            uint32_t* dstRow = dstPixels + y * width;
            for (DWORD x = 0; x < width; ++x) {
                uint16_t pixel = srcRow[x];
                uint8_t r = ((pixel >> 11) & 0x1F) << 3;
                uint8_t g = ((pixel >> 5) & 0x3F) << 2;
                uint8_t b = (pixel & 0x1F) << 3;
                dstRow[x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }
    else if (bpp == 24) {
        // 24-bit RGB
        for (DWORD y = 0; y < height; ++y) {
            const uint8_t* srcRow = srcPixels + y * pitch;
            uint32_t* dstRow = dstPixels + y * width;
            for (DWORD x = 0; x < width; ++x) {
                uint8_t b = srcRow[x * 3 + 0];
                uint8_t g = srcRow[x * 3 + 1];
                uint8_t r = srcRow[x * 3 + 2];
                dstRow[x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }
    else if (bpp == 32) {
        // 32-bit - direct copy
        for (DWORD y = 0; y < height; ++y) {
            const uint32_t* srcRow = reinterpret_cast<const uint32_t*>(srcPixels + y * pitch);
            uint32_t* dstRow = dstPixels + y * width;
            memcpy(dstRow, srcRow, width * sizeof(uint32_t));
        }
    }

    // Blit to window
    RECT clientRect;
    GetClientRect(g_state.hWnd, &clientRect);
    int windowWidth = clientRect.right - clientRect.left;
    int windowHeight = clientRect.bottom - clientRect.top;

    if (windowWidth == (int)width && windowHeight == (int)height) {
        // No scaling needed
        BitBlt(g_state.hdcWindow, 0, 0, width, height,
               g_state.hdcMem, 0, 0, SRCCOPY);
    }
    else {
        // Scale to fit window
        SetStretchBltMode(g_state.hdcWindow, HALFTONE);
        SetBrushOrgEx(g_state.hdcWindow, 0, 0, nullptr);
        StretchBlt(g_state.hdcWindow, 0, 0, windowWidth, windowHeight,
                   g_state.hdcMem, 0, 0, width, height, SRCCOPY);
    }

    // Update FPS counter
    g_state.frameCount++;
    DWORD now = GetTickCount();
    if (now - g_state.lastFpsTime >= 1000) {
        g_state.fps = g_state.frameCount;
        g_state.frameCount = 0;
        g_state.lastFpsTime = now;
    }
}

// ============================================================================
// Window Management
// ============================================================================

void ldc::UpdateScaling() {
    if (!g_state.hWnd) {
        g_state.scaleX = 1.0f;
        g_state.scaleY = 1.0f;
        g_state.offsetX = 0;
        g_state.offsetY = 0;
        return;
    }

    RECT clientRect;
    GetClientRect(g_state.hWnd, &clientRect);
    int windowWidth = clientRect.right - clientRect.left;
    int windowHeight = clientRect.bottom - clientRect.top;

    if (windowWidth <= 0) windowWidth = 1;
    if (windowHeight <= 0) windowHeight = 1;

    g_state.renderWidth = windowWidth;
    g_state.renderHeight = windowHeight;

    g_state.scaleX = static_cast<float>(g_state.gameWidth) / windowWidth;
    g_state.scaleY = static_cast<float>(g_state.gameHeight) / windowHeight;
    g_state.offsetX = 0;
    g_state.offsetY = 0;
}

POINT ldc::TransformMouseToGame(POINT pt) {
    POINT result;
    result.x = static_cast<LONG>((pt.x - g_state.offsetX) * g_state.scaleX);
    result.y = static_cast<LONG>((pt.y - g_state.offsetY) * g_state.scaleY);

    // Clamp to game bounds
    if (result.x < 0) result.x = 0;
    if (result.y < 0) result.y = 0;
    if (result.x >= (LONG)g_state.gameWidth) result.x = g_state.gameWidth - 1;
    if (result.y >= (LONG)g_state.gameHeight) result.y = g_state.gameHeight - 1;

    return result;
}

POINT ldc::TransformGameToScreen(POINT pt) {
    POINT result;
    result.x = static_cast<LONG>(pt.x / g_state.scaleX) + g_state.offsetX;
    result.y = static_cast<LONG>(pt.y / g_state.scaleY) + g_state.offsetY;
    return result;
}

LRESULT CALLBACK ldc::WrapperWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE:
            UpdateScaling();
            break;

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        {
            // Transform mouse coordinates
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            pt = TransformMouseToGame(pt);
            lParam = MAKELPARAM(pt.x, pt.y);
            break;
        }
    }

    if (g_state.originalWndProc) {
        return CallWindowProcA(g_state.originalWndProc, hWnd, msg, wParam, lParam);
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

void ldc::SubclassWindow(HWND hWnd) {
    if (g_state.originalWndProc) {
        return;  // Already subclassed
    }

    g_state.originalWndProc = (WNDPROC)SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR)WrapperWndProc);
    DebugLog("Window subclassed: %p", hWnd);
}

void ldc::UnsubclassWindow() {
    if (g_state.originalWndProc && g_state.hWnd) {
        SetWindowLongPtrA(g_state.hWnd, GWLP_WNDPROC, (LONG_PTR)g_state.originalWndProc);
        g_state.originalWndProc = nullptr;
        DebugLog("Window unsubclassed");
    }
}

// ============================================================================
// Initialization / Cleanup
// ============================================================================

bool ldc::InitializeWrapper() {
    if (g_state.initialized) {
        return true;
    }

    DebugLog("legacy-ddraw-compat initializing...");

    timeBeginPeriod(1);

    g_state.initialized = true;
    DebugLog("legacy-ddraw-compat initialized");

    return true;
}

void ldc::ShutdownWrapper() {
    if (!g_state.initialized) {
        return;
    }

    DebugLog("legacy-ddraw-compat shutting down...");

    UnsubclassWindow();
    DestroyRenderTarget();

    timeEndPeriod(1);

    g_state.initialized = false;
}

// ============================================================================
// DLL Entry Point
// ============================================================================

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    LDC_UNUSED(lpReserved);

    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            g_state.hModule = hModule;
            DisableThreadLibraryCalls(hModule);
            InitializeWrapper();
            break;

        case DLL_PROCESS_DETACH:
            ShutdownWrapper();
            break;
    }

    return TRUE;
}
