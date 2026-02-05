/**
 * @file SurfaceImpl.cpp
 * @brief IDirectDrawSurface interface implementation
 */

#include "interfaces/SurfaceImpl.h"
#include "interfaces/DirectDrawImpl.h"
#include "logging/Logger.h"

using namespace ldc;
using namespace ldc::interfaces;

// ============================================================================
// Surface GUIDs
// ============================================================================

#ifndef DEFINE_GUID_INLINE
#define DEFINE_GUID_INLINE(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
#endif

DEFINE_GUID_INLINE(LOCAL_IID_IDirectDrawSurface,  0x6C14DB81,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
DEFINE_GUID_INLINE(LOCAL_IID_IDirectDrawSurface2, 0x57805885,0x6eec,0x11cf,0x94,0x41,0xa8,0x23,0x03,0xc1,0x0e,0x27);
DEFINE_GUID_INLINE(LOCAL_IID_IDirectDrawSurface3, 0xDA044E00,0x69B2,0x11D0,0xA1,0xD5,0x00,0xAA,0x00,0xB8,0xDF,0xBB);
DEFINE_GUID_INLINE(LOCAL_IID_IDirectDrawSurface4, 0x0B2B8630,0xAD35,0x11D0,0x8E,0xA6,0x00,0x60,0x97,0x97,0xEA,0x5B);
DEFINE_GUID_INLINE(LOCAL_IID_IDirectDrawSurface7, 0x06675a80,0x3b9b,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b);

// ============================================================================
// SurfaceImpl Implementation
// ============================================================================

SurfaceImpl::SurfaceImpl(DirectDrawImpl* parent, const DDSURFACEDESC2& desc)
    : m_refCount(1)
    , m_parent(parent)
{
    LOG_DEBUG("SurfaceImpl creating surface");

    // Copy flags and caps
    m_data.flags = desc.dwFlags;
    if (desc.dwFlags & DDSD_CAPS) {
        m_data.caps = desc.ddsCaps;
    }

    // Determine dimensions
    if (desc.dwFlags & DDSD_WIDTH) {
        m_data.width = desc.dwWidth;
    } else if (IsPrimary()) {
        // Primary surface uses display mode dimensions
        DDSURFACEDESC2 displayMode{};
        displayMode.dwSize = sizeof(DDSURFACEDESC2);
        parent->GetDisplayMode(&displayMode);
        m_data.width = displayMode.dwWidth;
    }

    if (desc.dwFlags & DDSD_HEIGHT) {
        m_data.height = desc.dwHeight;
    } else if (IsPrimary()) {
        DDSURFACEDESC2 displayMode{};
        displayMode.dwSize = sizeof(DDSURFACEDESC2);
        parent->GetDisplayMode(&displayMode);
        m_data.height = displayMode.dwHeight;
    }

    // Determine pixel format
    if (desc.dwFlags & DDSD_PIXELFORMAT) {
        m_data.pixelFormat = desc.ddpfPixelFormat;
        m_data.bpp = desc.ddpfPixelFormat.dwRGBBitCount;
    } else {
        // Default to display mode format
        DDSURFACEDESC2 displayMode{};
        displayMode.dwSize = sizeof(DDSURFACEDESC2);
        parent->GetDisplayMode(&displayMode);
        m_data.bpp = displayMode.ddpfPixelFormat.dwRGBBitCount;
        m_data.pixelFormat = displayMode.ddpfPixelFormat;
    }

    // Ensure we have valid dimensions and bpp
    if (m_data.width == 0) m_data.width = 640;
    if (m_data.height == 0) m_data.height = 480;
    if (m_data.bpp == 0) m_data.bpp = 32;

    // Calculate pitch (align to 4 bytes)
    m_data.pitch = ((m_data.width * (m_data.bpp / 8) + 3) / 4) * 4;

    // Initialize pixel format if not set
    InitializePixelFormat();

    // Allocate pixel data
    AllocatePixelData();

    LOG_INFO("SurfaceImpl created: %ux%u %ubpp pitch=%u",
             m_data.width, m_data.height, m_data.bpp, m_data.pitch);
}

SurfaceImpl::~SurfaceImpl() {
    LOG_DEBUG("SurfaceImpl destroyed");

    // Clean up GDI resources
    if (m_hDC) {
        if (m_hBitmapOld) {
            SelectObject(m_hDC, m_hBitmapOld);
        }
        DeleteDC(m_hDC);
        m_hDC = nullptr;
    }
    if (m_hBitmap) {
        DeleteObject(m_hBitmap);
        m_hBitmap = nullptr;
    }
}

void SurfaceImpl::InitializePixelFormat() {
    m_data.pixelFormat.dwSize = sizeof(DDPIXELFORMAT);

    if (m_data.bpp == 8) {
        m_data.pixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
        m_data.pixelFormat.dwRGBBitCount = 8;
    } else if (m_data.bpp == 16) {
        m_data.pixelFormat.dwFlags = DDPF_RGB;
        m_data.pixelFormat.dwRGBBitCount = 16;
        // RGB565
        m_data.pixelFormat.dwRBitMask = 0xF800;
        m_data.pixelFormat.dwGBitMask = 0x07E0;
        m_data.pixelFormat.dwBBitMask = 0x001F;
    } else if (m_data.bpp == 24) {
        m_data.pixelFormat.dwFlags = DDPF_RGB;
        m_data.pixelFormat.dwRGBBitCount = 24;
        m_data.pixelFormat.dwRBitMask = 0x00FF0000;
        m_data.pixelFormat.dwGBitMask = 0x0000FF00;
        m_data.pixelFormat.dwBBitMask = 0x000000FF;
    } else {
        // 32-bit XRGB
        m_data.pixelFormat.dwFlags = DDPF_RGB;
        m_data.pixelFormat.dwRGBBitCount = 32;
        m_data.pixelFormat.dwRBitMask = 0x00FF0000;
        m_data.pixelFormat.dwGBitMask = 0x0000FF00;
        m_data.pixelFormat.dwBBitMask = 0x000000FF;
    }
}

void SurfaceImpl::AllocatePixelData() {
    size_t size = static_cast<size_t>(m_data.pitch) * m_data.height;
    m_data.pixels.resize(size, 0);
    LOG_DEBUG("Allocated %zu bytes for surface pixels", size);
}

bool SurfaceImpl::IsPrimary() const {
    return (m_data.caps.dwCaps & DDSCAPS_PRIMARYSURFACE) != 0;
}

bool SurfaceImpl::IsBackBuffer() const {
    return (m_data.caps.dwCaps & DDSCAPS_BACKBUFFER) != 0;
}

void SurfaceImpl::NotifyContentChanged() {
    // TODO: Notify renderer that surface content has changed
}

// ============================================================================
// IUnknown Implementation
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }

    *ppvObj = nullptr;

    if (riid == IID_IUnknown ||
        riid == LOCAL_IID_IDirectDrawSurface ||
        riid == LOCAL_IID_IDirectDrawSurface2 ||
        riid == LOCAL_IID_IDirectDrawSurface3 ||
        riid == LOCAL_IID_IDirectDrawSurface4 ||
        riid == LOCAL_IID_IDirectDrawSurface7) {

        AddRef();
        *ppvObj = static_cast<IDirectDrawSurface7*>(this);
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE SurfaceImpl::AddRef() {
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE SurfaceImpl::Release() {
    ULONG count = --m_refCount;
    if (count == 0) {
        delete this;
    }
    return count;
}

// ============================================================================
// Core Surface Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::Lock(
    LPRECT lpDestRect,
    LPDDSURFACEDESC2 lpDDSurfaceDesc,
    DWORD dwFlags,
    HANDLE hEvent)
{
    LDC_UNUSED(hEvent);

    LOG_TRACE("Lock: flags=0x%08X", dwFlags);

    if (!lpDDSurfaceDesc) {
        return DDERR_INVALIDPARAMS;
    }

    std::lock_guard<std::mutex> lock(m_lockMutex);

    if (m_locked) {
        return DDERR_SURFACEBUSY;
    }

    // Fill in surface description
    ZeroMemory(lpDDSurfaceDesc, sizeof(DDSURFACEDESC2));
    lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC2);
    lpDDSurfaceDesc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_LPSURFACE;
    lpDDSurfaceDesc->dwWidth = m_data.width;
    lpDDSurfaceDesc->dwHeight = m_data.height;
    lpDDSurfaceDesc->lPitch = m_data.pitch;
    lpDDSurfaceDesc->ddpfPixelFormat = m_data.pixelFormat;

    // Calculate surface pointer
    if (lpDestRect) {
        // Lock specific region
        size_t offset = lpDestRect->top * m_data.pitch +
                        lpDestRect->left * (m_data.bpp / 8);
        lpDDSurfaceDesc->lpSurface = m_data.pixels.data() + offset;
        m_lockedRect = *lpDestRect;
    } else {
        // Lock entire surface
        lpDDSurfaceDesc->lpSurface = m_data.pixels.data();
        m_lockedRect = { 0, 0, static_cast<LONG>(m_data.width), static_cast<LONG>(m_data.height) };
    }

    m_locked = true;

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::Unlock(LPRECT lpRect) {
    LDC_UNUSED(lpRect);

    LOG_TRACE("Unlock");

    std::lock_guard<std::mutex> lock(m_lockMutex);

    if (!m_locked) {
        return DDERR_NOTLOCKED;
    }

    m_locked = false;
    NotifyContentChanged();

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::Blt(
    LPRECT lpDestRect,
    LPDIRECTDRAWSURFACE7 lpDDSrcSurface,
    LPRECT lpSrcRect,
    DWORD dwFlags,
    LPDDBLTFX lpDDBltFx)
{
    LOG_TRACE("Blt: flags=0x%08X", dwFlags);

    // Get source surface
    SurfaceImpl* pSrc = static_cast<SurfaceImpl*>(lpDDSrcSurface);

    // Determine destination rectangle
    RECT dstRect;
    if (lpDestRect) {
        dstRect = *lpDestRect;
    } else {
        dstRect = { 0, 0, static_cast<LONG>(m_data.width), static_cast<LONG>(m_data.height) };
    }

    // Determine source rectangle
    RECT srcRect;
    if (lpSrcRect) {
        srcRect = *lpSrcRect;
    } else if (pSrc) {
        srcRect = { 0, 0, static_cast<LONG>(pSrc->m_data.width), static_cast<LONG>(pSrc->m_data.height) };
    } else {
        srcRect = { 0, 0, 0, 0 };
    }

    return BlitInternal(dstRect, pSrc, srcRect, dwFlags, lpDDBltFx);
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::BltFast(
    DWORD dwX,
    DWORD dwY,
    LPDIRECTDRAWSURFACE7 lpDDSrcSurface,
    LPRECT lpSrcRect,
    DWORD dwTrans)
{
    LOG_TRACE("BltFast: x=%u y=%u flags=0x%08X", dwX, dwY, dwTrans);

    if (!lpDDSrcSurface) {
        return DDERR_INVALIDPARAMS;
    }

    SurfaceImpl* pSrc = static_cast<SurfaceImpl*>(lpDDSrcSurface);

    // Determine source rectangle
    RECT srcRect;
    if (lpSrcRect) {
        srcRect = *lpSrcRect;
    } else {
        srcRect = { 0, 0, static_cast<LONG>(pSrc->m_data.width), static_cast<LONG>(pSrc->m_data.height) };
    }

    // Calculate destination rectangle
    LONG width = srcRect.right - srcRect.left;
    LONG height = srcRect.bottom - srcRect.top;
    RECT dstRect = { static_cast<LONG>(dwX), static_cast<LONG>(dwY),
                     static_cast<LONG>(dwX) + width, static_cast<LONG>(dwY) + height };

    // Convert flags
    DWORD flags = 0;
    if (dwTrans & DDBLTFAST_SRCCOLORKEY) {
        flags |= DDBLT_KEYSRC;
    }
    if (dwTrans & DDBLTFAST_DESTCOLORKEY) {
        flags |= DDBLT_KEYDEST;
    }

    return BlitInternal(dstRect, pSrc, srcRect, flags, nullptr);
}

HRESULT SurfaceImpl::BlitInternal(
    const RECT& dstRect,
    SurfaceImpl* pSrc,
    const RECT& srcRect,
    DWORD flags,
    const DDBLTFX* pFx)
{
    // Color fill operation
    if (flags & DDBLT_COLORFILL) {
        if (!pFx) {
            return DDERR_INVALIDPARAMS;
        }

        uint32_t color = pFx->dwFillColor;
        uint32_t bytesPerPixel = m_data.bpp / 8;

        for (LONG y = dstRect.top; y < dstRect.bottom && y < static_cast<LONG>(m_data.height); ++y) {
            uint8_t* row = m_data.pixels.data() + y * m_data.pitch + dstRect.left * bytesPerPixel;
            for (LONG x = dstRect.left; x < dstRect.right && x < static_cast<LONG>(m_data.width); ++x) {
                if (bytesPerPixel == 1) {
                    *row = static_cast<uint8_t>(color);
                } else if (bytesPerPixel == 2) {
                    *reinterpret_cast<uint16_t*>(row) = static_cast<uint16_t>(color);
                } else if (bytesPerPixel == 4) {
                    *reinterpret_cast<uint32_t*>(row) = color;
                }
                row += bytesPerPixel;
            }
        }

        NotifyContentChanged();
        return DD_OK;
    }

    // Source blit operation
    if (pSrc) {
        // Simple blit without scaling
        uint32_t bytesPerPixel = m_data.bpp / 8;
        LONG srcWidth = srcRect.right - srcRect.left;
        LONG srcHeight = srcRect.bottom - srcRect.top;
        LONG dstWidth = dstRect.right - dstRect.left;
        LONG dstHeight = dstRect.bottom - dstRect.top;

        // For simplicity, do a direct copy if sizes match
        LONG copyWidth = (srcWidth < dstWidth) ? srcWidth : dstWidth;
        LONG copyHeight = (srcHeight < dstHeight) ? srcHeight : dstHeight;

        // Clamp to surface boundaries
        if (dstRect.left + copyWidth > static_cast<LONG>(m_data.width)) {
            copyWidth = m_data.width - dstRect.left;
        }
        if (dstRect.top + copyHeight > static_cast<LONG>(m_data.height)) {
            copyHeight = m_data.height - dstRect.top;
        }
        if (srcRect.left + copyWidth > static_cast<LONG>(pSrc->m_data.width)) {
            copyWidth = pSrc->m_data.width - srcRect.left;
        }
        if (srcRect.top + copyHeight > static_cast<LONG>(pSrc->m_data.height)) {
            copyHeight = pSrc->m_data.height - srcRect.top;
        }

        if (copyWidth <= 0 || copyHeight <= 0) {
            return DD_OK;
        }

        // Perform the copy
        bool useColorKey = (flags & DDBLT_KEYSRC) && pSrc->m_hasSrcColorKey;
        uint32_t colorKey = pSrc->m_srcColorKey.dwColorSpaceLowValue;

        for (LONG y = 0; y < copyHeight; ++y) {
            uint8_t* dstRow = m_data.pixels.data() +
                              (dstRect.top + y) * m_data.pitch +
                              dstRect.left * bytesPerPixel;
            const uint8_t* srcRow = pSrc->m_data.pixels.data() +
                                    (srcRect.top + y) * pSrc->m_data.pitch +
                                    srcRect.left * bytesPerPixel;

            if (!useColorKey) {
                memcpy(dstRow, srcRow, copyWidth * bytesPerPixel);
            } else {
                // Color key blit
                for (LONG x = 0; x < copyWidth; ++x) {
                    uint32_t pixel = 0;
                    if (bytesPerPixel == 1) {
                        pixel = srcRow[x];
                    } else if (bytesPerPixel == 2) {
                        pixel = reinterpret_cast<const uint16_t*>(srcRow)[x];
                    } else if (bytesPerPixel == 4) {
                        pixel = reinterpret_cast<const uint32_t*>(srcRow)[x];
                    }

                    if (pixel != colorKey) {
                        if (bytesPerPixel == 1) {
                            dstRow[x] = static_cast<uint8_t>(pixel);
                        } else if (bytesPerPixel == 2) {
                            reinterpret_cast<uint16_t*>(dstRow)[x] = static_cast<uint16_t>(pixel);
                        } else if (bytesPerPixel == 4) {
                            reinterpret_cast<uint32_t*>(dstRow)[x] = pixel;
                        }
                    }
                }
            }
        }

        NotifyContentChanged();
        return DD_OK;
    }

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::Flip(
    LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride,
    DWORD dwFlags)
{
    LOG_TRACE("Flip: flags=0x%08X", dwFlags);

    LDC_UNUSED(lpDDSurfaceTargetOverride);

    // If we have a back buffer, swap content
    if (m_backBuffer) {
        std::swap(m_data.pixels, m_backBuffer->m_data.pixels);
    }

    NotifyContentChanged();

    // VSync wait if requested
    if (!(dwFlags & DDFLIP_NOVSYNC)) {
        // Simple VSync simulation
        Sleep(1);
    }

    return DD_OK;
}

// ============================================================================
// Surface Information Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc) {
    LOG_TRACE("GetSurfaceDesc");

    if (!lpDDSurfaceDesc) {
        return DDERR_INVALIDPARAMS;
    }

    lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC2);
    lpDDSurfaceDesc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;
    lpDDSurfaceDesc->dwWidth = m_data.width;
    lpDDSurfaceDesc->dwHeight = m_data.height;
    lpDDSurfaceDesc->lPitch = m_data.pitch;
    lpDDSurfaceDesc->ddpfPixelFormat = m_data.pixelFormat;
    lpDDSurfaceDesc->ddsCaps = m_data.caps;

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetCaps(LPDDSCAPS2 lpDDSCaps) {
    LOG_TRACE("GetCaps");

    if (!lpDDSCaps) {
        return DDERR_INVALIDPARAMS;
    }

    *lpDDSCaps = m_data.caps;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat) {
    LOG_TRACE("GetPixelFormat");

    if (!lpDDPixelFormat) {
        return DDERR_INVALIDPARAMS;
    }

    *lpDDPixelFormat = m_data.pixelFormat;
    return DD_OK;
}

// ============================================================================
// Color Key Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey) {
    LOG_TRACE("GetColorKey: flags=0x%08X", dwFlags);

    if (!lpDDColorKey) {
        return DDERR_INVALIDPARAMS;
    }

    if (dwFlags & DDCKEY_SRCBLT) {
        if (!m_hasSrcColorKey) {
            return DDERR_NOCOLORKEY;
        }
        *lpDDColorKey = m_srcColorKey;
    } else if (dwFlags & DDCKEY_DESTBLT) {
        if (!m_hasDestColorKey) {
            return DDERR_NOCOLORKEY;
        }
        *lpDDColorKey = m_destColorKey;
    } else {
        return DDERR_INVALIDPARAMS;
    }

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey) {
    LOG_TRACE("SetColorKey: flags=0x%08X", dwFlags);

    if (dwFlags & DDCKEY_SRCBLT) {
        if (lpDDColorKey) {
            m_srcColorKey = *lpDDColorKey;
            m_hasSrcColorKey = true;
        } else {
            m_hasSrcColorKey = false;
        }
    } else if (dwFlags & DDCKEY_DESTBLT) {
        if (lpDDColorKey) {
            m_destColorKey = *lpDDColorKey;
            m_hasDestColorKey = true;
        } else {
            m_hasDestColorKey = false;
        }
    } else {
        return DDERR_INVALIDPARAMS;
    }

    return DD_OK;
}

// ============================================================================
// GDI Interop
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetDC(HDC* lphDC) {
    LOG_TRACE("GetDC");

    if (!lphDC) {
        return DDERR_INVALIDPARAMS;
    }

    if (m_hDC) {
        return DDERR_DCALREADYCREATED;
    }

    // Create a compatible DC
    HDC hScreenDC = GetDC(nullptr);
    m_hDC = CreateCompatibleDC(hScreenDC);

    // Create a DIB section
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_data.width;
    bmi.bmiHeader.biHeight = -static_cast<LONG>(m_data.height);  // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = static_cast<WORD>(m_data.bpp);
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    m_hBitmap = CreateDIBSection(m_hDC, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);

    if (!m_hBitmap || !pBits) {
        DeleteDC(m_hDC);
        m_hDC = nullptr;
        ReleaseDC(nullptr, hScreenDC);
        return DDERR_GENERIC;
    }

    // Copy current surface data to DIB
    memcpy(pBits, m_data.pixels.data(), m_data.pixels.size());

    m_hBitmapOld = static_cast<HBITMAP>(SelectObject(m_hDC, m_hBitmap));
    ReleaseDC(nullptr, hScreenDC);

    *lphDC = m_hDC;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::ReleaseDC(HDC hDC) {
    LOG_TRACE("ReleaseDC");

    if (hDC != m_hDC || !m_hDC) {
        return DDERR_INVALIDPARAMS;
    }

    // Copy DIB data back to surface
    BITMAP bm;
    GetObject(m_hBitmap, sizeof(bm), &bm);
    if (bm.bmBits) {
        memcpy(m_data.pixels.data(), bm.bmBits, m_data.pixels.size());
    }

    // Clean up
    SelectObject(m_hDC, m_hBitmapOld);
    DeleteObject(m_hBitmap);
    DeleteDC(m_hDC);

    m_hDC = nullptr;
    m_hBitmap = nullptr;
    m_hBitmapOld = nullptr;

    NotifyContentChanged();

    return DD_OK;
}

// ============================================================================
// Palette Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetPalette(LPDIRECTDRAWPALETTE* lplpDDPalette) {
    LOG_TRACE("GetPalette");

    if (!lplpDDPalette) {
        return DDERR_INVALIDPARAMS;
    }

    if (!m_palette) {
        *lplpDDPalette = nullptr;
        return DDERR_NOPALETTEATTACHED;
    }

    // TODO: Return palette
    *lplpDDPalette = nullptr;
    return DDERR_NOPALETTEATTACHED;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette) {
    LOG_TRACE("SetPalette");

    // TODO: Implement palette attachment
    LDC_UNUSED(lpDDPalette);
    return DD_OK;
}

// ============================================================================
// Clipper Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetClipper(LPDIRECTDRAWCLIPPER* lplpDDClipper) {
    LOG_TRACE("GetClipper");

    if (!lplpDDClipper) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO: Return clipper
    *lplpDDClipper = nullptr;
    return DDERR_NOCLIPPERATTACHED;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper) {
    LOG_TRACE("SetClipper");

    // TODO: Implement clipper attachment
    LDC_UNUSED(lpDDClipper);
    return DD_OK;
}

// ============================================================================
// Stub Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::AddAttachedSurface(LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface) {
    LOG_TRACE("AddAttachedSurface");
    LDC_UNUSED(lpDDSAttachedSurface);
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::AddOverlayDirtyRect(LPRECT lpRect) {
    LDC_UNUSED(lpRect);
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags) {
    LDC_UNUSED(lpDDBltBatch);
    LDC_UNUSED(dwCount);
    LDC_UNUSED(dwFlags);
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface) {
    LDC_UNUSED(dwFlags);
    LDC_UNUSED(lpDDSAttachedSurface);
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback) {
    if (m_backBuffer && lpEnumSurfacesCallback) {
        DDSURFACEDESC2 desc{};
        desc.dwSize = sizeof(DDSURFACEDESC2);
        m_backBuffer->GetSurfaceDesc(&desc);
        lpEnumSurfacesCallback(m_backBuffer, &desc, lpContext);
    }
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback) {
    LDC_UNUSED(dwFlags);
    LDC_UNUSED(lpContext);
    LDC_UNUSED(lpfnCallback);
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetAttachedSurface(LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7* lplpDDAttachedSurface) {
    if (!lplpDDAttachedSurface) {
        return DDERR_INVALIDPARAMS;
    }

    if (m_backBuffer && (lpDDSCaps->dwCaps & DDSCAPS_BACKBUFFER)) {
        m_backBuffer->AddRef();
        *lplpDDAttachedSurface = m_backBuffer;
        return DD_OK;
    }

    *lplpDDAttachedSurface = nullptr;
    return DDERR_NOTFOUND;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetBltStatus(DWORD dwFlags) {
    LDC_UNUSED(dwFlags);
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetFlipStatus(DWORD dwFlags) {
    LDC_UNUSED(dwFlags);
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetOverlayPosition(LPLONG lplX, LPLONG lplY) {
    LDC_UNUSED(lplX);
    LDC_UNUSED(lplY);
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc) {
    LDC_UNUSED(lpDD);
    LDC_UNUSED(lpDDSurfaceDesc);
    return DDERR_ALREADYINITIALIZED;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::IsLost() {
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::Restore() {
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::SetOverlayPosition(LONG lX, LONG lY) {
    LDC_UNUSED(lX);
    LDC_UNUSED(lY);
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) {
    LDC_UNUSED(lpSrcRect);
    LDC_UNUSED(lpDDDestSurface);
    LDC_UNUSED(lpDestRect);
    LDC_UNUSED(dwFlags);
    LDC_UNUSED(lpDDOverlayFx);
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::UpdateOverlayDisplay(DWORD dwFlags) {
    LDC_UNUSED(dwFlags);
    return DDERR_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference) {
    LDC_UNUSED(dwFlags);
    LDC_UNUSED(lpDDSReference);
    return DDERR_UNSUPPORTED;
}

// ============================================================================
// IDirectDrawSurface2+ Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetDDInterface(LPVOID* lplpDD) {
    if (!lplpDD) {
        return DDERR_INVALIDPARAMS;
    }
    if (m_parent) {
        m_parent->AddRef();
        *lplpDD = m_parent;
        return DD_OK;
    }
    return DDERR_INVALIDOBJECT;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::PageLock(DWORD dwFlags) {
    LDC_UNUSED(dwFlags);
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::PageUnlock(DWORD dwFlags) {
    LDC_UNUSED(dwFlags);
    return DD_OK;
}

// ============================================================================
// IDirectDrawSurface3+ Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::SetSurfaceDesc(LPDDSURFACEDESC2 lpDDSD, DWORD dwFlags) {
    LDC_UNUSED(lpDDSD);
    LDC_UNUSED(dwFlags);
    return DDERR_UNSUPPORTED;
}

// ============================================================================
// IDirectDrawSurface4+ Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::SetPrivateData(REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags) {
    LDC_UNUSED(dwFlags);

    if (!lpData || cbSize == 0) {
        m_privateData.erase(guidTag);
        return DD_OK;
    }

    std::vector<uint8_t> data(static_cast<uint8_t*>(lpData),
                               static_cast<uint8_t*>(lpData) + cbSize);
    m_privateData[guidTag] = std::move(data);
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetPrivateData(REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize) {
    auto it = m_privateData.find(guidTag);
    if (it == m_privateData.end()) {
        return DDERR_NOTFOUND;
    }

    if (!lpcbBufferSize) {
        return DDERR_INVALIDPARAMS;
    }

    if (*lpcbBufferSize < it->second.size()) {
        *lpcbBufferSize = static_cast<DWORD>(it->second.size());
        return DDERR_MOREDATA;
    }

    if (lpBuffer) {
        memcpy(lpBuffer, it->second.data(), it->second.size());
    }
    *lpcbBufferSize = static_cast<DWORD>(it->second.size());
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::FreePrivateData(REFGUID guidTag) {
    m_privateData.erase(guidTag);
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetUniquenessValue(LPDWORD lpValue) {
    if (!lpValue) {
        return DDERR_INVALIDPARAMS;
    }
    *lpValue = m_uniquenessValue;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::ChangeUniquenessValue() {
    ++m_uniquenessValue;
    return DD_OK;
}

// ============================================================================
// IDirectDrawSurface7 Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::SetPriority(DWORD dwPriority) {
    LDC_UNUSED(dwPriority);
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetPriority(LPDWORD lpdwPriority) {
    if (lpdwPriority) {
        *lpdwPriority = 0;
    }
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::SetLOD(DWORD dwMaxLOD) {
    LDC_UNUSED(dwMaxLOD);
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetLOD(LPDWORD lpdwMaxLOD) {
    if (lpdwMaxLOD) {
        *lpdwMaxLOD = 0;
    }
    return DD_OK;
}
