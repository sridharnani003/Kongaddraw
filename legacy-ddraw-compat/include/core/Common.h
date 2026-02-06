/**
 * @file Common.h
 * @brief Common definitions for legacy-ddraw-compat
 */

#pragma once

// Windows headers - must be first
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>  // For GET_X_LPARAM, GET_Y_LPARAM
#include <mmsystem.h>

// DirectDraw header
#include <ddraw.h>

// Standard library
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <array>
#include <unordered_map>
#include <functional>

// Link required libraries
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ddraw.lib")
#pragma comment(lib, "dxguid.lib")

namespace ldc {

#define LDC_UNUSED(x) (void)(x)

// ============================================================================
// Log Level Enumeration
// ============================================================================

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4
};

// ============================================================================
// NonCopyable Base Class
// ============================================================================

class NonCopyable {
protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

// ============================================================================
// Debug Logging
// ============================================================================

inline void DebugLog(const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}

// ============================================================================
// Global State - Single point of state for the entire wrapper
// ============================================================================

struct GlobalState {
    // Module handle
    HMODULE hModule = nullptr;
    bool initialized = false;

    // Window state
    HWND hWnd = nullptr;
    WNDPROC originalWndProc = nullptr;
    DWORD coopLevel = 0;

    // Game's requested display mode
    DWORD gameWidth = 640;
    DWORD gameHeight = 480;
    DWORD gameBpp = 8;
    DWORD gameRefresh = 0;
    bool displayModeSet = false;

    // Actual render target size (window client area)
    DWORD renderWidth = 640;
    DWORD renderHeight = 480;

    // Scaling for mouse coordinates
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    int offsetX = 0;
    int offsetY = 0;

    // GDI rendering resources
    HDC hdcWindow = nullptr;
    HDC hdcMem = nullptr;
    HBITMAP hBitmap = nullptr;
    HBITMAP hBitmapOld = nullptr;
    void* bitmapBits = nullptr;
    DWORD bitmapWidth = 0;
    DWORD bitmapHeight = 0;

    // Palette for 8-bit mode (as RGBQUAD for SetDIBitsToDevice)
    RGBQUAD palette[256] = {};
    uint32_t palette32[256] = {};  // As ARGB for conversion
    bool paletteChanged = true;

    // Primary surface pixel data
    std::vector<uint8_t> primaryPixels;
    DWORD primaryPitch = 0;

    // Converted 32-bit buffer for rendering
    std::vector<uint32_t> renderBuffer;

    // Thread safety
    std::recursive_mutex renderMutex;

    // Statistics
    DWORD frameCount = 0;
    DWORD lastFpsTime = 0;
    DWORD fps = 0;
};

// Global state instance
extern GlobalState g_state;

// ============================================================================
// Initialization / Cleanup
// ============================================================================

bool InitializeWrapper();
void ShutdownWrapper();

// ============================================================================
// Rendering
// ============================================================================

bool CreateRenderTarget(DWORD width, DWORD height, DWORD bpp);
void DestroyRenderTarget();
void PresentPrimaryToScreen();

// ============================================================================
// Window Management
// ============================================================================

void SubclassWindow(HWND hWnd);
void UnsubclassWindow();
LRESULT CALLBACK WrapperWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================================================
// Mouse Coordinate Transformation
// ============================================================================

void UpdateScaling();
POINT TransformMouseToGame(POINT pt);
POINT TransformGameToScreen(POINT pt);

// ============================================================================
// Hooked Windows API Functions
// ============================================================================

void InstallMouseHooks();
void RemoveMouseHooks();

} // namespace ldc
