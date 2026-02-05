/**
 * @file SurfaceImpl.h
 * @brief IDirectDrawSurface interface implementation
 *
 * Implements the IDirectDrawSurface through IDirectDrawSurface7 COM interfaces.
 */

#pragma once

#include "core/Common.h"
#include <atomic>
#include <mutex>
#include <vector>

namespace ldc::interfaces {

// Forward declarations
class DirectDrawImpl;
class PaletteImpl;
class ClipperImpl;

/**
 * @brief Surface data storage
 */
struct SurfaceData {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t bpp = 0;
    uint32_t pitch = 0;
    std::vector<uint8_t> pixels;
    DDSCAPS2 caps{};
    DDPIXELFORMAT pixelFormat{};
    DWORD flags = 0;
};

/**
 * @brief IDirectDrawSurface7 implementation
 *
 * Implements all versions of the IDirectDrawSurface interface.
 * Manages pixel data in system memory and coordinates with
 * the renderer for presentation.
 */
class SurfaceImpl : public IDirectDrawSurface7 {
public:
    /**
     * @brief Construct a surface
     * @param parent Parent DirectDraw object
     * @param desc Surface description
     */
    SurfaceImpl(DirectDrawImpl* parent, const DDSURFACEDESC2& desc);
    virtual ~SurfaceImpl();

    // ========================================================================
    // IUnknown Methods
    // ========================================================================

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    // ========================================================================
    // IDirectDrawSurface Methods
    // ========================================================================

    HRESULT STDMETHODCALLTYPE AddAttachedSurface(LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface) override;
    HRESULT STDMETHODCALLTYPE AddOverlayDirtyRect(LPRECT lpRect) override;
    HRESULT STDMETHODCALLTYPE Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) override;
    HRESULT STDMETHODCALLTYPE BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags) override;
    HRESULT STDMETHODCALLTYPE BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans) override;
    HRESULT STDMETHODCALLTYPE DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface) override;
    HRESULT STDMETHODCALLTYPE EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback) override;
    HRESULT STDMETHODCALLTYPE EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback) override;
    HRESULT STDMETHODCALLTYPE Flip(LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags) override;
    HRESULT STDMETHODCALLTYPE GetAttachedSurface(LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7* lplpDDAttachedSurface) override;
    HRESULT STDMETHODCALLTYPE GetBltStatus(DWORD dwFlags) override;
    HRESULT STDMETHODCALLTYPE GetCaps(LPDDSCAPS2 lpDDSCaps) override;
    HRESULT STDMETHODCALLTYPE GetClipper(LPDIRECTDRAWCLIPPER* lplpDDClipper) override;
    HRESULT STDMETHODCALLTYPE GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey) override;
    HRESULT STDMETHODCALLTYPE GetDC(HDC* lphDC) override;
    HRESULT STDMETHODCALLTYPE GetFlipStatus(DWORD dwFlags) override;
    HRESULT STDMETHODCALLTYPE GetOverlayPosition(LPLONG lplX, LPLONG lplY) override;
    HRESULT STDMETHODCALLTYPE GetPalette(LPDIRECTDRAWPALETTE* lplpDDPalette) override;
    HRESULT STDMETHODCALLTYPE GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat) override;
    HRESULT STDMETHODCALLTYPE GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc) override;
    HRESULT STDMETHODCALLTYPE Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc) override;
    HRESULT STDMETHODCALLTYPE IsLost() override;
    HRESULT STDMETHODCALLTYPE Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent) override;
    HRESULT STDMETHODCALLTYPE ReleaseDC(HDC hDC) override;
    HRESULT STDMETHODCALLTYPE Restore() override;
    HRESULT STDMETHODCALLTYPE SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper) override;
    HRESULT STDMETHODCALLTYPE SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey) override;
    HRESULT STDMETHODCALLTYPE SetOverlayPosition(LONG lX, LONG lY) override;
    HRESULT STDMETHODCALLTYPE SetPalette(LPDIRECTDRAWPALETTE lpDDPalette) override;
    HRESULT STDMETHODCALLTYPE Unlock(LPRECT lpRect) override;
    HRESULT STDMETHODCALLTYPE UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) override;
    HRESULT STDMETHODCALLTYPE UpdateOverlayDisplay(DWORD dwFlags) override;
    HRESULT STDMETHODCALLTYPE UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference) override;

    // ========================================================================
    // IDirectDrawSurface2 Methods
    // ========================================================================

    HRESULT STDMETHODCALLTYPE GetDDInterface(LPVOID* lplpDD) override;
    HRESULT STDMETHODCALLTYPE PageLock(DWORD dwFlags) override;
    HRESULT STDMETHODCALLTYPE PageUnlock(DWORD dwFlags) override;

    // ========================================================================
    // IDirectDrawSurface3 Methods
    // ========================================================================

    HRESULT STDMETHODCALLTYPE SetSurfaceDesc(LPDDSURFACEDESC2 lpDDSD, DWORD dwFlags) override;

    // ========================================================================
    // IDirectDrawSurface4 Methods
    // ========================================================================

    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize) override;
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID guidTag) override;
    HRESULT STDMETHODCALLTYPE GetUniquenessValue(LPDWORD lpValue) override;
    HRESULT STDMETHODCALLTYPE ChangeUniquenessValue() override;

    // ========================================================================
    // IDirectDrawSurface7 Methods
    // ========================================================================

    HRESULT STDMETHODCALLTYPE SetPriority(DWORD dwPriority) override;
    HRESULT STDMETHODCALLTYPE GetPriority(LPDWORD lpdwPriority) override;
    HRESULT STDMETHODCALLTYPE SetLOD(DWORD dwMaxLOD) override;
    HRESULT STDMETHODCALLTYPE GetLOD(LPDWORD lpdwMaxLOD) override;

    // ========================================================================
    // Internal Methods
    // ========================================================================

    /** Get surface data */
    const SurfaceData& GetData() const { return m_data; }

    /** Get raw pixel data */
    void* GetPixels() { return m_data.pixels.data(); }
    const void* GetPixels() const { return m_data.pixels.data(); }

    /** Check if this is a primary surface */
    bool IsPrimary() const;

    /** Check if this is a back buffer */
    bool IsBackBuffer() const;

    /** Set back buffer in chain */
    void SetBackBuffer(SurfaceImpl* pBack) { m_backBuffer = pBack; }

    /** Get back buffer in chain */
    SurfaceImpl* GetBackBuffer() const { return m_backBuffer; }

    /** Get width */
    uint32_t GetWidth() const { return m_data.width; }

    /** Get height */
    uint32_t GetHeight() const { return m_data.height; }

    /** Get bits per pixel */
    uint32_t GetBpp() const { return m_data.bpp; }

    /** Get pitch (bytes per row) */
    uint32_t GetPitch() const { return m_data.pitch; }

    /** Notify that surface content changed (for rendering) */
    void NotifyContentChanged();

private:
    std::atomic<ULONG> m_refCount{1};

    DirectDrawImpl* m_parent;
    SurfaceData m_data;
    SurfaceImpl* m_backBuffer = nullptr;
    PaletteImpl* m_palette = nullptr;
    ClipperImpl* m_clipper = nullptr;

    bool m_locked = false;
    RECT m_lockedRect{};
    std::mutex m_lockMutex;

    // Color key
    DDCOLORKEY m_srcColorKey{};
    DDCOLORKEY m_destColorKey{};
    bool m_hasSrcColorKey = false;
    bool m_hasDestColorKey = false;

    // GDI interop
    HDC m_hDC = nullptr;
    HBITMAP m_hBitmap = nullptr;
    HBITMAP m_hBitmapOld = nullptr;

    // Private data storage
    std::unordered_map<GUID, std::vector<uint8_t>, struct GuidHash> m_privateData;

    // Uniqueness value
    DWORD m_uniquenessValue = 0;

    // Helper methods
    void InitializePixelFormat();
    void AllocatePixelData();
    HRESULT BlitInternal(const RECT& dstRect, SurfaceImpl* pSrc, const RECT& srcRect, DWORD flags, const DDBLTFX* pFx);

    // GUID hash for unordered_map
    struct GuidHash {
        size_t operator()(const GUID& guid) const {
            const uint64_t* p = reinterpret_cast<const uint64_t*>(&guid);
            return std::hash<uint64_t>()(p[0]) ^ std::hash<uint64_t>()(p[1]);
        }
    };
};

} // namespace ldc::interfaces
