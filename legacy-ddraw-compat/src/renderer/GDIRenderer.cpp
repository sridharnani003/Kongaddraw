/**
 * @file GDIRenderer.cpp
 * @brief GDI-based rendering backend
 *
 * The GDI renderer provides maximum compatibility by using only
 * standard Windows GDI functions. It works on any Windows system
 * but does not support shaders or hardware acceleration.
 */

#include "renderer/IRenderer.h"
#include "logging/Logger.h"

using namespace ldc;
using namespace ldc::renderer;

// ============================================================================
// GDIRenderer Class
// ============================================================================

class GDIRenderer : public IRenderer {
public:
    GDIRenderer();
    ~GDIRenderer() override;

    bool Initialize(HWND hWnd, uint32_t width, uint32_t height, uint32_t bpp) override;
    void Shutdown() override;
    void Present(const void* pixels, uint32_t pitch, uint32_t width, uint32_t height, uint32_t bpp) override;
    void SetPalette(const uint32_t* palette256) override;
    void SetVSync(bool enabled) override;
    RendererType GetType() const override { return RendererType::GDI; }
    RendererCaps GetCaps() const override;
    bool IsAvailable() const override { return true; }
    void OnResize(uint32_t width, uint32_t height) override;

private:
    HWND m_hWnd = nullptr;
    HDC m_hdcWindow = nullptr;
    HDC m_hdcMem = nullptr;
    HBITMAP m_hBitmap = nullptr;
    HBITMAP m_hBitmapOld = nullptr;
    void* m_bitmapBits = nullptr;

    uint32_t m_gameWidth = 0;
    uint32_t m_gameHeight = 0;
    uint32_t m_gameBpp = 0;
    uint32_t m_windowWidth = 0;
    uint32_t m_windowHeight = 0;

    BITMAPINFO m_bitmapInfo{};
    std::array<uint32_t, 256> m_palette{};

    bool m_initialized = false;

    bool CreateDIBSection();
    void DestroyDIBSection();
    void UpdatePaletteConversion();
};

// ============================================================================
// GDIRenderer Implementation
// ============================================================================

GDIRenderer::GDIRenderer() {
    // Initialize palette to grayscale
    for (int i = 0; i < 256; ++i) {
        m_palette[i] = (i << 16) | (i << 8) | i | 0xFF000000;
    }
}

GDIRenderer::~GDIRenderer() {
    Shutdown();
}

bool GDIRenderer::Initialize(HWND hWnd, uint32_t width, uint32_t height, uint32_t bpp) {
    LOG_INFO("GDIRenderer::Initialize: %ux%u %ubpp", width, height, bpp);

    if (m_initialized) {
        Shutdown();
    }

    m_hWnd = hWnd;
    m_gameWidth = width;
    m_gameHeight = height;
    m_gameBpp = bpp;

    // Get window dimensions
    RECT rect;
    GetClientRect(hWnd, &rect);
    m_windowWidth = rect.right - rect.left;
    m_windowHeight = rect.bottom - rect.top;

    // Get window DC
    m_hdcWindow = GetDC(hWnd);
    if (!m_hdcWindow) {
        LOG_ERROR("GDIRenderer: Failed to get window DC");
        return false;
    }

    // Create memory DC
    m_hdcMem = CreateCompatibleDC(m_hdcWindow);
    if (!m_hdcMem) {
        LOG_ERROR("GDIRenderer: Failed to create compatible DC");
        ReleaseDC(hWnd, m_hdcWindow);
        m_hdcWindow = nullptr;
        return false;
    }

    // Create DIB section
    if (!CreateDIBSection()) {
        LOG_ERROR("GDIRenderer: Failed to create DIB section");
        DeleteDC(m_hdcMem);
        ReleaseDC(hWnd, m_hdcWindow);
        m_hdcMem = nullptr;
        m_hdcWindow = nullptr;
        return false;
    }

    m_initialized = true;
    LOG_INFO("GDIRenderer initialized successfully");
    return true;
}

void GDIRenderer::Shutdown() {
    LOG_DEBUG("GDIRenderer::Shutdown");

    DestroyDIBSection();

    if (m_hdcMem) {
        DeleteDC(m_hdcMem);
        m_hdcMem = nullptr;
    }

    if (m_hdcWindow && m_hWnd) {
        ReleaseDC(m_hWnd, m_hdcWindow);
        m_hdcWindow = nullptr;
    }

    m_initialized = false;
}

bool GDIRenderer::CreateDIBSection() {
    // Set up BITMAPINFO for 32-bit DIB
    ZeroMemory(&m_bitmapInfo, sizeof(m_bitmapInfo));
    m_bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_bitmapInfo.bmiHeader.biWidth = m_gameWidth;
    m_bitmapInfo.bmiHeader.biHeight = -static_cast<LONG>(m_gameHeight);  // Top-down
    m_bitmapInfo.bmiHeader.biPlanes = 1;
    m_bitmapInfo.bmiHeader.biBitCount = 32;  // Always use 32-bit for rendering
    m_bitmapInfo.bmiHeader.biCompression = BI_RGB;

    m_hBitmap = CreateDIBSection(
        m_hdcMem,
        &m_bitmapInfo,
        DIB_RGB_COLORS,
        &m_bitmapBits,
        nullptr,
        0
    );

    if (!m_hBitmap || !m_bitmapBits) {
        LOG_ERROR("CreateDIBSection failed");
        return false;
    }

    m_hBitmapOld = static_cast<HBITMAP>(SelectObject(m_hdcMem, m_hBitmap));

    LOG_DEBUG("Created DIB section: %ux%u", m_gameWidth, m_gameHeight);
    return true;
}

void GDIRenderer::DestroyDIBSection() {
    if (m_hdcMem && m_hBitmapOld) {
        SelectObject(m_hdcMem, m_hBitmapOld);
        m_hBitmapOld = nullptr;
    }

    if (m_hBitmap) {
        DeleteObject(m_hBitmap);
        m_hBitmap = nullptr;
    }

    m_bitmapBits = nullptr;
}

void GDIRenderer::Present(
    const void* pixels,
    uint32_t pitch,
    uint32_t width,
    uint32_t height,
    uint32_t bpp)
{
    if (!m_initialized || !m_bitmapBits || !pixels) {
        return;
    }

    // Convert source pixels to 32-bit BGRA in our DIB
    uint32_t* dest = static_cast<uint32_t*>(m_bitmapBits);
    const uint8_t* src = static_cast<const uint8_t*>(pixels);

    // Handle size mismatch by using minimum
    uint32_t copyWidth = (width < m_gameWidth) ? width : m_gameWidth;
    uint32_t copyHeight = (height < m_gameHeight) ? height : m_gameHeight;

    if (bpp == 8) {
        // 8-bit palettized to 32-bit
        for (uint32_t y = 0; y < copyHeight; ++y) {
            const uint8_t* srcRow = src + y * pitch;
            uint32_t* destRow = dest + y * m_gameWidth;

            for (uint32_t x = 0; x < copyWidth; ++x) {
                destRow[x] = m_palette[srcRow[x]];
            }
        }
    }
    else if (bpp == 16) {
        // 16-bit RGB565 to 32-bit
        for (uint32_t y = 0; y < copyHeight; ++y) {
            const uint16_t* srcRow = reinterpret_cast<const uint16_t*>(src + y * pitch);
            uint32_t* destRow = dest + y * m_gameWidth;

            for (uint32_t x = 0; x < copyWidth; ++x) {
                uint16_t pixel = srcRow[x];
                uint8_t r = ((pixel >> 11) & 0x1F) << 3;
                uint8_t g = ((pixel >> 5) & 0x3F) << 2;
                uint8_t b = (pixel & 0x1F) << 3;
                destRow[x] = (r << 16) | (g << 8) | b | 0xFF000000;
            }
        }
    }
    else if (bpp == 24) {
        // 24-bit RGB to 32-bit
        for (uint32_t y = 0; y < copyHeight; ++y) {
            const uint8_t* srcRow = src + y * pitch;
            uint32_t* destRow = dest + y * m_gameWidth;

            for (uint32_t x = 0; x < copyWidth; ++x) {
                uint8_t b = srcRow[x * 3 + 0];
                uint8_t g = srcRow[x * 3 + 1];
                uint8_t r = srcRow[x * 3 + 2];
                destRow[x] = (r << 16) | (g << 8) | b | 0xFF000000;
            }
        }
    }
    else if (bpp == 32) {
        // 32-bit direct copy
        for (uint32_t y = 0; y < copyHeight; ++y) {
            const uint32_t* srcRow = reinterpret_cast<const uint32_t*>(src + y * pitch);
            uint32_t* destRow = dest + y * m_gameWidth;
            memcpy(destRow, srcRow, copyWidth * sizeof(uint32_t));
        }
    }

    // Blit to window
    if (m_gameWidth == m_windowWidth && m_gameHeight == m_windowHeight) {
        // Direct blit (no scaling)
        BitBlt(
            m_hdcWindow,
            0, 0,
            m_gameWidth, m_gameHeight,
            m_hdcMem,
            0, 0,
            SRCCOPY
        );
    }
    else {
        // Scaled blit
        SetStretchBltMode(m_hdcWindow, HALFTONE);
        SetBrushOrgEx(m_hdcWindow, 0, 0, nullptr);

        StretchBlt(
            m_hdcWindow,
            0, 0,
            m_windowWidth, m_windowHeight,
            m_hdcMem,
            0, 0,
            m_gameWidth, m_gameHeight,
            SRCCOPY
        );
    }
}

void GDIRenderer::SetPalette(const uint32_t* palette256) {
    if (palette256) {
        memcpy(m_palette.data(), palette256, 256 * sizeof(uint32_t));
        LOG_DEBUG("GDIRenderer: Palette updated");
    }
}

void GDIRenderer::SetVSync(bool enabled) {
    // GDI doesn't support VSync directly
    // We could use DWM composition for vsync but that's complex
    LDC_UNUSED(enabled);
    LOG_DEBUG("GDIRenderer: VSync not supported in GDI mode");
}

RendererCaps GDIRenderer::GetCaps() const {
    RendererCaps caps{};
    caps.supportsShaders = false;
    caps.supportsVSync = false;
    caps.maxTextureWidth = 8192;
    caps.maxTextureHeight = 8192;
    return caps;
}

void GDIRenderer::OnResize(uint32_t width, uint32_t height) {
    LOG_DEBUG("GDIRenderer::OnResize: %ux%u", width, height);

    m_windowWidth = width;
    m_windowHeight = height;
}

// ============================================================================
// Factory Function
// ============================================================================

namespace ldc::renderer {

std::unique_ptr<IRenderer> CreateGDIRenderer() {
    return std::make_unique<GDIRenderer>();
}

} // namespace ldc::renderer
