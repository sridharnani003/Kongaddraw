/**
 * @file IRenderer.h
 * @brief Abstract renderer interface for legacy-ddraw-compat
 *
 * Defines the interface that all rendering backends must implement.
 */

#pragma once

#include "core/Common.h"

namespace ldc::renderer {

// ============================================================================
// Renderer Capabilities
// ============================================================================

/**
 * @brief Renderer capability information
 */
struct RendererCaps {
    /** Renderer supports shader effects */
    bool supportsShaders = false;

    /** Renderer supports VSync */
    bool supportsVSync = false;

    /** Maximum texture width supported */
    uint32_t maxTextureWidth = 0;

    /** Maximum texture height supported */
    uint32_t maxTextureHeight = 0;

    /** Renderer name string */
    std::string name;

    /** Renderer version string */
    std::string version;
};

// ============================================================================
// Renderer Interface
// ============================================================================

/**
 * @brief Abstract interface for rendering backends
 *
 * Defines the contract that GDI, OpenGL, and Direct3D 9 renderers
 * must implement for presenting DirectDraw surfaces to the display.
 */
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // ========================================================================
    // Lifecycle
    // ========================================================================

    /**
     * @brief Initialize the renderer
     * @param hWnd Window handle for rendering
     * @param width Game render width
     * @param height Game render height
     * @param bpp Bits per pixel (8, 16, 24, or 32)
     * @return true if initialization successful
     */
    virtual bool Initialize(HWND hWnd, uint32_t width, uint32_t height, uint32_t bpp) = 0;

    /**
     * @brief Shutdown the renderer and release resources
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Check if renderer is initialized
     * @return true if initialized
     */
    virtual bool IsInitialized() const = 0;

    // ========================================================================
    // Rendering
    // ========================================================================

    /**
     * @brief Present surface data to the display
     * @param pixels Pointer to pixel data
     * @param pitch Bytes per row
     * @param width Surface width
     * @param height Surface height
     * @param bpp Bits per pixel
     *
     * Copies the surface data to the GPU and presents it to the screen.
     * For 8-bit surfaces, uses the current palette for conversion.
     */
    virtual void Present(
        const void* pixels,
        uint32_t pitch,
        uint32_t width,
        uint32_t height,
        uint32_t bpp
    ) = 0;

    /**
     * @brief Set the palette for 8-bit rendering
     * @param palette256 Array of 256 32-bit XRGB colors
     *
     * Updates the color lookup table used for converting 8-bit
     * palettized surfaces to 32-bit for display.
     */
    virtual void SetPalette(const uint32_t* palette256) = 0;

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Enable or disable VSync
     * @param enabled true to enable VSync
     */
    virtual void SetVSync(bool enabled) = 0;

    /**
     * @brief Handle window resize
     * @param width New window width
     * @param height New window height
     */
    virtual void OnResize(uint32_t width, uint32_t height) = 0;

    // ========================================================================
    // Information
    // ========================================================================

    /**
     * @brief Get the renderer type
     * @return RendererType enumeration value
     */
    virtual RendererType GetType() const = 0;

    /**
     * @brief Get renderer capabilities
     * @return RendererCaps structure
     */
    virtual RendererCaps GetCaps() const = 0;

    /**
     * @brief Check if this renderer type is available on the system
     * @return true if available
     *
     * Static check that can be called before construction.
     */
    virtual bool IsAvailable() const = 0;
};

// ============================================================================
// Renderer Factory
// ============================================================================

/**
 * @brief Factory for creating renderer instances
 */
class RendererFactory {
public:
    /**
     * @brief Create a renderer of the specified type
     * @param type Renderer type to create
     * @return Unique pointer to renderer, or nullptr if failed
     */
    static std::unique_ptr<IRenderer> Create(RendererType type);

    /**
     * @brief Create the best available renderer
     * @return Unique pointer to renderer, or nullptr if all failed
     *
     * Tries renderers in order: D3D9 > OpenGL > GDI
     */
    static std::unique_ptr<IRenderer> CreateBestAvailable();

    /**
     * @brief Check if Direct3D 9 renderer is available
     * @return true if available
     */
    static bool IsD3D9Available();

    /**
     * @brief Check if OpenGL renderer is available
     * @return true if available
     */
    static bool IsOpenGLAvailable();

    // GDI is always available on Windows

private:
    static std::unique_ptr<IRenderer> TryCreate(RendererType type);
};

} // namespace ldc::renderer
