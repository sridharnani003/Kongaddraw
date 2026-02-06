/**
 * @file DirectDrawImpl.h
 * @brief IDirectDraw7 interface implementation
 *
 * Implements the IDirectDraw7 COM interface for the compatibility layer.
 */

#pragma once

#include "core/Common.h"

namespace ldc::interfaces {

// Forward declarations
class SurfaceImpl;
class PaletteImpl;

/**
 * @brief IDirectDraw7 implementation
 *
 * Provides compatibility layer for DirectDraw applications.
 * All older interface versions (IDirectDraw through IDirectDraw4)
 * are handled via QueryInterface returning this same object.
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
    // IDirectDraw7 Methods
    // ========================================================================

    HRESULT STDMETHODCALLTYPE Compact() override;

    HRESULT STDMETHODCALLTYPE CreateClipper(
        DWORD dwFlags,
        LPDIRECTDRAWCLIPPER* lplpDDClipper,
        IUnknown* pUnkOuter) override;

    HRESULT STDMETHODCALLTYPE CreatePalette(
        DWORD dwFlags,
        LPPALETTEENTRY lpDDColorArray,
        LPDIRECTDRAWPALETTE* lplpDDPalette,
        IUnknown* pUnkOuter) override;

    HRESULT STDMETHODCALLTYPE CreateSurface(
        LPDDSURFACEDESC2 lpDDSurfaceDesc,
        LPDIRECTDRAWSURFACE7* lplpDDSurface,
        IUnknown* pUnkOuter) override;

    HRESULT STDMETHODCALLTYPE DuplicateSurface(
        LPDIRECTDRAWSURFACE7 lpDDSurface,
        LPDIRECTDRAWSURFACE7* lplpDupDDSurface) override;

    HRESULT STDMETHODCALLTYPE EnumDisplayModes(
        DWORD dwFlags,
        LPDDSURFACEDESC2 lpDDSurfaceDesc,
        LPVOID lpContext,
        LPDDENUMMODESCALLBACK2 lpEnumModesCallback) override;

    HRESULT STDMETHODCALLTYPE EnumSurfaces(
        DWORD dwFlags,
        LPDDSURFACEDESC2 lpDDSD,
        LPVOID lpContext,
        LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback) override;

    HRESULT STDMETHODCALLTYPE FlipToGDISurface() override;

    HRESULT STDMETHODCALLTYPE GetCaps(
        LPDDCAPS lpDDDriverCaps,
        LPDDCAPS lpDDHELCaps) override;

    HRESULT STDMETHODCALLTYPE GetDisplayMode(
        LPDDSURFACEDESC2 lpDDSurfaceDesc) override;

    HRESULT STDMETHODCALLTYPE GetFourCCCodes(
        LPDWORD lpNumCodes,
        LPDWORD lpCodes) override;

    HRESULT STDMETHODCALLTYPE GetGDISurface(
        LPDIRECTDRAWSURFACE7* lplpGDIDDSSurface) override;

    HRESULT STDMETHODCALLTYPE GetMonitorFrequency(
        LPDWORD lpdwFrequency) override;

    HRESULT STDMETHODCALLTYPE GetScanLine(
        LPDWORD lpdwScanLine) override;

    HRESULT STDMETHODCALLTYPE GetVerticalBlankStatus(
        LPBOOL lpbIsInVB) override;

    HRESULT STDMETHODCALLTYPE Initialize(
        GUID* lpGUID) override;

    HRESULT STDMETHODCALLTYPE RestoreDisplayMode() override;

    HRESULT STDMETHODCALLTYPE SetCooperativeLevel(
        HWND hWnd,
        DWORD dwFlags) override;

    HRESULT STDMETHODCALLTYPE SetDisplayMode(
        DWORD dwWidth,
        DWORD dwHeight,
        DWORD dwBPP,
        DWORD dwRefreshRate,
        DWORD dwFlags) override;

    HRESULT STDMETHODCALLTYPE WaitForVerticalBlank(
        DWORD dwFlags,
        HANDLE hEvent) override;

    // IDirectDraw2 methods
    HRESULT STDMETHODCALLTYPE GetAvailableVidMem(
        LPDDSCAPS2 lpDDSCaps,
        LPDWORD lpdwTotal,
        LPDWORD lpdwFree) override;

    // IDirectDraw4 methods
    HRESULT STDMETHODCALLTYPE GetSurfaceFromDC(
        HDC hdc,
        LPDIRECTDRAWSURFACE7* lpDDS) override;

    HRESULT STDMETHODCALLTYPE RestoreAllSurfaces() override;

    HRESULT STDMETHODCALLTYPE TestCooperativeLevel() override;

    HRESULT STDMETHODCALLTYPE GetDeviceIdentifier(
        LPDDDEVICEIDENTIFIER2 lpdddi,
        DWORD dwFlags) override;

    // IDirectDraw7 methods
    HRESULT STDMETHODCALLTYPE StartModeTest(
        LPSIZE lpModesToTest,
        DWORD dwNumEntries,
        DWORD dwFlags) override;

    HRESULT STDMETHODCALLTYPE EvaluateMode(
        DWORD dwFlags,
        DWORD* pSecondsUntilTimeout) override;

    // ========================================================================
    // Internal Methods
    // ========================================================================

    /** Get interface version (1-7) */
    int GetInterfaceVersion() const { return m_interfaceVersion; }

    /** Set interface version for QueryInterface */
    void SetInterfaceVersion(int version) { m_interfaceVersion = version; }

    /** Get the window handle */
    HWND GetHWnd() const { return m_hWnd; }

    /** Get primary surface */
    SurfaceImpl* GetPrimarySurface() const { return m_primarySurface; }

    /** Set primary surface */
    void SetPrimarySurface(SurfaceImpl* pSurface) { m_primarySurface = pSurface; }

private:
    std::atomic<ULONG> m_refCount{1};
    int m_interfaceVersion = 7;
    bool m_initialized = false;

    // Window and display state
    HWND m_hWnd = nullptr;
    DWORD m_coopFlags = 0;

    // Display mode
    DWORD m_displayWidth = 0;
    DWORD m_displayHeight = 0;
    DWORD m_displayBpp = 0;
    DWORD m_displayRefresh = 0;
    bool m_displayModeChanged = false;

    // Primary surface reference
    SurfaceImpl* m_primarySurface = nullptr;

    // Helper methods
    void FillCaps(LPDDCAPS pCaps);
    void FillDeviceIdentifier(LPDDDEVICEIDENTIFIER2 pDDDI);
};

} // namespace ldc::interfaces
