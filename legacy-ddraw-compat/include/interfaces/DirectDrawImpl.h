/**
 * @file DirectDrawImpl.h
 * @brief IDirectDraw interface implementation
 *
 * Implements the IDirectDraw through IDirectDraw7 COM interfaces.
 */

#pragma once

#include "core/Common.h"
#include <atomic>

// Forward declarations
namespace ldc::core {
    class SurfaceManager;
    class DisplayManager;
    class PaletteManager;
}

namespace ldc::interfaces {

// Forward declarations
class SurfaceImpl;
class PaletteImpl;
class ClipperImpl;

/**
 * @brief IDirectDraw7 implementation
 *
 * Implements all versions of the IDirectDraw interface (1 through 7).
 * Uses internal managers for actual functionality.
 */
class DirectDrawImpl : public IDirectDraw7 {
public:
    DirectDrawImpl();
    virtual ~DirectDrawImpl();

    // ========================================================================
    // IUnknown Methods
    // ========================================================================

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    // ========================================================================
    // IDirectDraw Methods
    // ========================================================================

    HRESULT STDMETHODCALLTYPE Compact() override;
    HRESULT STDMETHODCALLTYPE CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpDDClipper, IUnknown* pUnkOuter) override;
    HRESULT STDMETHODCALLTYPE CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE* lplpDDPalette, IUnknown* pUnkOuter) override;
    HRESULT STDMETHODCALLTYPE CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter) override;
    HRESULT STDMETHODCALLTYPE DuplicateSurface(LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE* lplpDupDDSurface) override;
    HRESULT STDMETHODCALLTYPE EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback) override;
    HRESULT STDMETHODCALLTYPE EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback) override;
    HRESULT STDMETHODCALLTYPE FlipToGDISurface() override;
    HRESULT STDMETHODCALLTYPE GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps) override;
    HRESULT STDMETHODCALLTYPE GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc) override;
    HRESULT STDMETHODCALLTYPE GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes) override;
    HRESULT STDMETHODCALLTYPE GetGDISurface(LPDIRECTDRAWSURFACE* lplpGDIDDSSurface) override;
    HRESULT STDMETHODCALLTYPE GetMonitorFrequency(LPDWORD lpdwFrequency) override;
    HRESULT STDMETHODCALLTYPE GetScanLine(LPDWORD lpdwScanLine) override;
    HRESULT STDMETHODCALLTYPE GetVerticalBlankStatus(LPBOOL lpbIsInVB) override;
    HRESULT STDMETHODCALLTYPE Initialize(GUID* lpGUID) override;
    HRESULT STDMETHODCALLTYPE RestoreDisplayMode() override;
    HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND hWnd, DWORD dwFlags) override;
    HRESULT STDMETHODCALLTYPE SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP) override;
    HRESULT STDMETHODCALLTYPE WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent) override;

    // ========================================================================
    // IDirectDraw2 Methods
    // ========================================================================

    HRESULT STDMETHODCALLTYPE GetAvailableVidMem(LPDDSCAPS lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree) override;

    // ========================================================================
    // IDirectDraw4 Methods (overlapping with IDirectDraw)
    // ========================================================================

    // CreateSurface, EnumDisplayModes, EnumSurfaces with DDSURFACEDESC2
    HRESULT STDMETHODCALLTYPE CreateSurface(LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7* lplpDDSurface, IUnknown* pUnkOuter) override;
    HRESULT STDMETHODCALLTYPE EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback) override;
    HRESULT STDMETHODCALLTYPE EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback) override;

    HRESULT STDMETHODCALLTYPE GetDisplayMode(LPDDSURFACEDESC2 lpDDSurfaceDesc2) override;
    HRESULT STDMETHODCALLTYPE GetSurfaceFromDC(HDC hdc, LPDIRECTDRAWSURFACE7* lpDDS) override;
    HRESULT STDMETHODCALLTYPE RestoreAllSurfaces() override;
    HRESULT STDMETHODCALLTYPE TestCooperativeLevel() override;
    HRESULT STDMETHODCALLTYPE GetDeviceIdentifier(LPDDDEVICEIDENTIFIER2 lpdddi, DWORD dwFlags) override;

    // ========================================================================
    // IDirectDraw7 Methods
    // ========================================================================

    HRESULT STDMETHODCALLTYPE SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags) override;
    HRESULT STDMETHODCALLTYPE GetAvailableVidMem(LPDDSCAPS2 lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree) override;
    HRESULT STDMETHODCALLTYPE StartModeTest(LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags) override;
    HRESULT STDMETHODCALLTYPE EvaluateMode(DWORD dwFlags, DWORD* pSecondsUntilTimeout) override;

    // ========================================================================
    // Internal Methods
    // ========================================================================

    /** Get the display manager */
    core::DisplayManager* GetDisplayManager() { return m_displayMgr.get(); }

    /** Get the surface manager */
    core::SurfaceManager* GetSurfaceManager() { return m_surfaceMgr.get(); }

    /** Get the palette manager */
    core::PaletteManager* GetPaletteManager() { return m_paletteMgr.get(); }

    /** Get interface version (1-7) */
    int GetInterfaceVersion() const { return m_interfaceVersion; }

    /** Set interface version for QueryInterface */
    void SetInterfaceVersion(int version) { m_interfaceVersion = version; }

private:
    std::atomic<ULONG> m_refCount{1};

    std::unique_ptr<core::DisplayManager> m_displayMgr;
    std::unique_ptr<core::SurfaceManager> m_surfaceMgr;
    std::unique_ptr<core::PaletteManager> m_paletteMgr;

    int m_interfaceVersion = 7;
    bool m_initialized = false;

    // Helper methods
    HRESULT CreateSurfaceInternal(const DDSURFACEDESC2& desc, SurfaceImpl** ppSurface);
    void FillCaps(DDCAPS* pCaps);
    void FillDeviceIdentifier(DDDEVICEIDENTIFIER2* pDDDI);
};

} // namespace ldc::interfaces
