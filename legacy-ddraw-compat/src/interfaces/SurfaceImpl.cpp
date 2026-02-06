/**
 * @file SurfaceImpl.cpp
 * @brief IDirectDrawSurface7 interface implementation
 */

#include "interfaces/SurfaceImpl.h"
#include "interfaces/DirectDrawImpl.h"
#include "core/Common.h"

using namespace ldc;
using namespace ldc::interfaces;

// ============================================================================
// SurfaceImpl Implementation
// ============================================================================

SurfaceImpl::SurfaceImpl(DirectDrawImpl* parent, const DDSURFACEDESC2& desc)
    : m_refCount(1)
    , m_parent(parent)
    , m_width(0)
    , m_height(0)
    , m_bpp(0)
    , m_pitch(0)
    , m_caps{}
    , m_pixelFormat{}
    , m_flags(0)
    , m_backBuffer(nullptr)
    , m_palette(nullptr)
    , m_clipper(nullptr)
    , m_locked(false)
    , m_lockedRect{}
    , m_srcColorKey{}
    , m_destColorKey{}
    , m_hasSrcColorKey(false)
    , m_hasDestColorKey(false)
    , m_hDC(nullptr)
    , m_hBitmap(nullptr)
    , m_hBitmapOld(nullptr)
    , m_uniquenessValue(0)
    , m_priority(0)
    , m_lod(0)
{
    DebugLog("SurfaceImpl creating surface");

    // Copy flags and caps
    m_flags = desc.dwFlags;
    if (desc.dwFlags & DDSD_CAPS) {
        m_caps = desc.ddsCaps;
    }

    // Determine dimensions
    if (desc.dwFlags & DDSD_WIDTH) {
        m_width = desc.dwWidth;
    } else if (IsPrimary()) {
        m_width = g_state.gameWidth;
    }

    if (desc.dwFlags & DDSD_HEIGHT) {
        m_height = desc.dwHeight;
    } else if (IsPrimary()) {
        m_height = g_state.gameHeight;
    }

    // Determine pixel format
    if (desc.dwFlags & DDSD_PIXELFORMAT) {
        m_pixelFormat = desc.ddpfPixelFormat;
        m_bpp = desc.ddpfPixelFormat.dwRGBBitCount;
    } else if (IsPrimary()) {
        m_bpp = g_state.gameBpp;
    }

    // Ensure we have valid dimensions and bpp
    if (m_width == 0) m_width = 640;
    if (m_height == 0) m_height = 480;
    if (m_bpp == 0) m_bpp = 8;

    // Calculate pitch (align to 4 bytes)
    m_pitch = ((m_width * (m_bpp / 8) + 3) / 4) * 4;

    // Initialize pixel format if not set
    InitializePixelFormat();

    // Allocate pixel data
    AllocatePixelData();

    // Handle back buffer creation for flip chains
    if ((desc.dwFlags & DDSD_BACKBUFFERCOUNT) && desc.dwBackBufferCount > 0) {
        DDSURFACEDESC2 backDesc = desc;
        backDesc.ddsCaps.dwCaps = (backDesc.ddsCaps.dwCaps & ~DDSCAPS_PRIMARYSURFACE) | DDSCAPS_BACKBUFFER;
        backDesc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
        backDesc.dwBackBufferCount = 0;

        m_backBuffer = new SurfaceImpl(parent, backDesc);
        DebugLog("Created back buffer for flip chain");
    }

    DebugLog("SurfaceImpl created: %ux%u %ubpp pitch=%u caps=0x%08X",
             m_width, m_height, m_bpp, m_pitch, m_caps.dwCaps);
}

SurfaceImpl::~SurfaceImpl() {
    DebugLog("SurfaceImpl destroyed");

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

    // Release back buffer
    if (m_backBuffer) {
        m_backBuffer->Release();
        m_backBuffer = nullptr;
    }
}

void SurfaceImpl::InitializePixelFormat() {
    m_pixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    m_pixelFormat.dwRGBBitCount = m_bpp;

    if (m_bpp == 8) {
        m_pixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    } else if (m_bpp == 16) {
        m_pixelFormat.dwFlags = DDPF_RGB;
        // RGB565
        m_pixelFormat.dwRBitMask = 0xF800;
        m_pixelFormat.dwGBitMask = 0x07E0;
        m_pixelFormat.dwBBitMask = 0x001F;
    } else if (m_bpp == 24) {
        m_pixelFormat.dwFlags = DDPF_RGB;
        m_pixelFormat.dwRBitMask = 0x00FF0000;
        m_pixelFormat.dwGBitMask = 0x0000FF00;
        m_pixelFormat.dwBBitMask = 0x000000FF;
    } else {
        // 32-bit XRGB
        m_pixelFormat.dwFlags = DDPF_RGB;
        m_pixelFormat.dwRBitMask = 0x00FF0000;
        m_pixelFormat.dwGBitMask = 0x0000FF00;
        m_pixelFormat.dwBBitMask = 0x000000FF;
    }
}

void SurfaceImpl::AllocatePixelData() {
    size_t size = static_cast<size_t>(m_pitch) * m_height;
    m_pixels.resize(size, 0);
    DebugLog("Allocated %zu bytes for surface pixels", size);
}

void SurfaceImpl::NotifyContentChanged() {
    // If this is the primary surface, copy to global state and present
    if (IsPrimary()) {
        std::lock_guard<std::recursive_mutex> lock(g_state.renderMutex);

        // Copy pixel data to global state
        if (g_state.primaryPixels.size() == m_pixels.size()) {
            memcpy(g_state.primaryPixels.data(), m_pixels.data(), m_pixels.size());
        } else {
            g_state.primaryPixels = m_pixels;
            g_state.primaryPitch = m_pitch;
        }

        // Present to screen
        PresentPrimaryToScreen();
    }

    m_uniquenessValue++;
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
        riid == IID_IDirectDrawSurface ||
        riid == IID_IDirectDrawSurface2 ||
        riid == IID_IDirectDrawSurface3 ||
        riid == IID_IDirectDrawSurface4 ||
        riid == IID_IDirectDrawSurface7) {

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
    LDC_UNUSED(dwFlags);

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
    lpDDSurfaceDesc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_LPSURFACE | DDSD_CAPS;
    lpDDSurfaceDesc->dwWidth = m_width;
    lpDDSurfaceDesc->dwHeight = m_height;
    lpDDSurfaceDesc->lPitch = m_pitch;
    lpDDSurfaceDesc->ddpfPixelFormat = m_pixelFormat;
    lpDDSurfaceDesc->ddsCaps = m_caps;

    // Calculate surface pointer
    if (lpDestRect) {
        // Lock specific region
        size_t offset = lpDestRect->top * m_pitch +
                        lpDestRect->left * (m_bpp / 8);
        lpDDSurfaceDesc->lpSurface = m_pixels.data() + offset;
        m_lockedRect = *lpDestRect;
    } else {
        // Lock entire surface
        lpDDSurfaceDesc->lpSurface = m_pixels.data();
        m_lockedRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    }

    m_locked = true;

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::Unlock(LPRECT lpRect) {
    LDC_UNUSED(lpRect);

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
    SurfaceImpl* pSrc = static_cast<SurfaceImpl*>(lpDDSrcSurface);

    // Determine destination rectangle
    RECT dstRect;
    if (lpDestRect) {
        dstRect = *lpDestRect;
    } else {
        dstRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    }

    // Color fill operation
    if (dwFlags & DDBLT_COLORFILL) {
        if (!lpDDBltFx) {
            return DDERR_INVALIDPARAMS;
        }

        DWORD color = lpDDBltFx->dwFillColor;
        DWORD bytesPerPixel = m_bpp / 8;

        for (LONG y = dstRect.top; y < dstRect.bottom && y < static_cast<LONG>(m_height); ++y) {
            uint8_t* row = m_pixels.data() + y * m_pitch + dstRect.left * bytesPerPixel;
            for (LONG x = dstRect.left; x < dstRect.right && x < static_cast<LONG>(m_width); ++x) {
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
        RECT srcRect;
        if (lpSrcRect) {
            srcRect = *lpSrcRect;
        } else {
            srcRect = { 0, 0, static_cast<LONG>(pSrc->m_width), static_cast<LONG>(pSrc->m_height) };
        }

        DWORD bytesPerPixel = m_bpp / 8;
        LONG srcWidth = srcRect.right - srcRect.left;
        LONG srcHeight = srcRect.bottom - srcRect.top;
        LONG dstWidth = dstRect.right - dstRect.left;
        LONG dstHeight = dstRect.bottom - dstRect.top;

        LONG copyWidth = (srcWidth < dstWidth) ? srcWidth : dstWidth;
        LONG copyHeight = (srcHeight < dstHeight) ? srcHeight : dstHeight;

        // Clamp to surface boundaries
        if (dstRect.left + copyWidth > static_cast<LONG>(m_width)) {
            copyWidth = m_width - dstRect.left;
        }
        if (dstRect.top + copyHeight > static_cast<LONG>(m_height)) {
            copyHeight = m_height - dstRect.top;
        }
        if (srcRect.left + copyWidth > static_cast<LONG>(pSrc->m_width)) {
            copyWidth = pSrc->m_width - srcRect.left;
        }
        if (srcRect.top + copyHeight > static_cast<LONG>(pSrc->m_height)) {
            copyHeight = pSrc->m_height - srcRect.top;
        }

        if (copyWidth <= 0 || copyHeight <= 0) {
            return DD_OK;
        }

        // Perform the copy
        bool useColorKey = (dwFlags & DDBLT_KEYSRC) && pSrc->m_hasSrcColorKey;
        DWORD colorKey = pSrc->m_srcColorKey.dwColorSpaceLowValue;

        for (LONG y = 0; y < copyHeight; ++y) {
            uint8_t* dstRow = m_pixels.data() +
                              (dstRect.top + y) * m_pitch +
                              dstRect.left * bytesPerPixel;
            const uint8_t* srcRow = pSrc->m_pixels.data() +
                                    (srcRect.top + y) * pSrc->m_pitch +
                                    srcRect.left * bytesPerPixel;

            if (!useColorKey) {
                memcpy(dstRow, srcRow, copyWidth * bytesPerPixel);
            } else {
                // Color key blit
                for (LONG x = 0; x < copyWidth; ++x) {
                    DWORD pixel = 0;
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

HRESULT STDMETHODCALLTYPE SurfaceImpl::BltFast(
    DWORD dwX,
    DWORD dwY,
    LPDIRECTDRAWSURFACE7 lpDDSrcSurface,
    LPRECT lpSrcRect,
    DWORD dwTrans)
{
    if (!lpDDSrcSurface) {
        return DDERR_INVALIDPARAMS;
    }

    SurfaceImpl* pSrc = static_cast<SurfaceImpl*>(lpDDSrcSurface);

    RECT srcRect;
    if (lpSrcRect) {
        srcRect = *lpSrcRect;
    } else {
        srcRect = { 0, 0, static_cast<LONG>(pSrc->m_width), static_cast<LONG>(pSrc->m_height) };
    }

    LONG width = srcRect.right - srcRect.left;
    LONG height = srcRect.bottom - srcRect.top;
    RECT dstRect = { static_cast<LONG>(dwX), static_cast<LONG>(dwY),
                     static_cast<LONG>(dwX) + width, static_cast<LONG>(dwY) + height };

    DWORD flags = 0;
    if (dwTrans & DDBLTFAST_SRCCOLORKEY) {
        flags |= DDBLT_KEYSRC;
    }
    if (dwTrans & DDBLTFAST_DESTCOLORKEY) {
        flags |= DDBLT_KEYDEST;
    }

    return Blt(&dstRect, lpDDSrcSurface, &srcRect, flags, nullptr);
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::Flip(
    LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride,
    DWORD dwFlags)
{
    LDC_UNUSED(lpDDSurfaceTargetOverride);

    // If we have a back buffer, swap content
    if (m_backBuffer) {
        std::swap(m_pixels, m_backBuffer->m_pixels);
    }

    // Present to screen
    NotifyContentChanged();

    // VSync wait if requested
    if (!(dwFlags & DDFLIP_NOVSYNC)) {
        Sleep(1);
    }

    return DD_OK;
}

// ============================================================================
// Surface Information Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc) {
    if (!lpDDSurfaceDesc) {
        return DDERR_INVALIDPARAMS;
    }

    ZeroMemory(lpDDSurfaceDesc, sizeof(DDSURFACEDESC2));
    lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC2);
    lpDDSurfaceDesc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;
    lpDDSurfaceDesc->dwWidth = m_width;
    lpDDSurfaceDesc->dwHeight = m_height;
    lpDDSurfaceDesc->lPitch = m_pitch;
    lpDDSurfaceDesc->ddpfPixelFormat = m_pixelFormat;
    lpDDSurfaceDesc->ddsCaps = m_caps;

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetCaps(LPDDSCAPS2 lpDDSCaps) {
    if (!lpDDSCaps) {
        return DDERR_INVALIDPARAMS;
    }
    *lpDDSCaps = m_caps;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat) {
    if (!lpDDPixelFormat) {
        return DDERR_INVALIDPARAMS;
    }
    *lpDDPixelFormat = m_pixelFormat;
    return DD_OK;
}

// ============================================================================
// Color Key Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey) {
    if (!lpDDColorKey) {
        return DDERR_INVALIDPARAMS;
    }

    if (dwFlags & DDCKEY_SRCBLT) {
        if (!m_hasSrcColorKey) return DDERR_NOCOLORKEY;
        *lpDDColorKey = m_srcColorKey;
    } else if (dwFlags & DDCKEY_DESTBLT) {
        if (!m_hasDestColorKey) return DDERR_NOCOLORKEY;
        *lpDDColorKey = m_destColorKey;
    } else {
        return DDERR_INVALIDPARAMS;
    }

    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey) {
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
    if (!lphDC) {
        return DDERR_INVALIDPARAMS;
    }

    if (m_hDC) {
        return DDERR_DCALREADYCREATED;
    }

    HDC hScreenDC = GetDC(nullptr);
    m_hDC = CreateCompatibleDC(hScreenDC);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_width;
    bmi.bmiHeader.biHeight = -static_cast<LONG>(m_height);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = static_cast<WORD>(m_bpp);
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    m_hBitmap = CreateDIBSection(m_hDC, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);

    if (!m_hBitmap || !pBits) {
        DeleteDC(m_hDC);
        m_hDC = nullptr;
        ReleaseDC(nullptr, hScreenDC);
        return DDERR_GENERIC;
    }

    memcpy(pBits, m_pixels.data(), m_pixels.size());
    m_hBitmapOld = static_cast<HBITMAP>(SelectObject(m_hDC, m_hBitmap));
    ReleaseDC(nullptr, hScreenDC);

    *lphDC = m_hDC;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::ReleaseDC(HDC hDC) {
    if (hDC != m_hDC || !m_hDC) {
        return DDERR_INVALIDPARAMS;
    }

    BITMAP bm;
    GetObject(m_hBitmap, sizeof(bm), &bm);
    if (bm.bmBits) {
        memcpy(m_pixels.data(), bm.bmBits, m_pixels.size());
    }

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
// Palette/Clipper Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetPalette(LPDIRECTDRAWPALETTE* lplpDDPalette) {
    if (!lplpDDPalette) return DDERR_INVALIDPARAMS;
    *lplpDDPalette = nullptr;
    return DDERR_NOPALETTEATTACHED;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette) {
    LDC_UNUSED(lpDDPalette);
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetClipper(LPDIRECTDRAWCLIPPER* lplpDDClipper) {
    if (!lplpDDClipper) return DDERR_INVALIDPARAMS;
    *lplpDDClipper = nullptr;
    return DDERR_NOCLIPPERATTACHED;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper) {
    LDC_UNUSED(lpDDClipper);
    return DD_OK;
}

// ============================================================================
// Stub/Simple Methods
// ============================================================================

HRESULT STDMETHODCALLTYPE SurfaceImpl::AddAttachedSurface(LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface) {
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
    if (!lplpDDAttachedSurface) return DDERR_INVALIDPARAMS;

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

// IDirectDrawSurface2+ Methods
HRESULT STDMETHODCALLTYPE SurfaceImpl::GetDDInterface(LPVOID* lplpDD) {
    if (!lplpDD) return DDERR_INVALIDPARAMS;
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

// IDirectDrawSurface3+ Methods
HRESULT STDMETHODCALLTYPE SurfaceImpl::SetSurfaceDesc(LPDDSURFACEDESC2 lpDDSD, DWORD dwFlags) {
    LDC_UNUSED(lpDDSD);
    LDC_UNUSED(dwFlags);
    return DDERR_UNSUPPORTED;
}

// IDirectDrawSurface4+ Methods
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
    if (it == m_privateData.end()) return DDERR_NOTFOUND;
    if (!lpcbBufferSize) return DDERR_INVALIDPARAMS;
    if (*lpcbBufferSize < it->second.size()) {
        *lpcbBufferSize = static_cast<DWORD>(it->second.size());
        return DDERR_MOREDATA;
    }
    if (lpBuffer) memcpy(lpBuffer, it->second.data(), it->second.size());
    *lpcbBufferSize = static_cast<DWORD>(it->second.size());
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::FreePrivateData(REFGUID guidTag) {
    m_privateData.erase(guidTag);
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetUniquenessValue(LPDWORD lpValue) {
    if (!lpValue) return DDERR_INVALIDPARAMS;
    *lpValue = m_uniquenessValue;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::ChangeUniquenessValue() {
    ++m_uniquenessValue;
    return DD_OK;
}

// IDirectDrawSurface7 Methods
HRESULT STDMETHODCALLTYPE SurfaceImpl::SetPriority(DWORD dwPriority) {
    m_priority = dwPriority;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetPriority(LPDWORD lpdwPriority) {
    if (lpdwPriority) *lpdwPriority = m_priority;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::SetLOD(DWORD dwMaxLOD) {
    m_lod = dwMaxLOD;
    return DD_OK;
}

HRESULT STDMETHODCALLTYPE SurfaceImpl::GetLOD(LPDWORD lpdwMaxLOD) {
    if (lpdwMaxLOD) *lpdwMaxLOD = m_lod;
    return DD_OK;
}
