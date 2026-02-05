# Interface Control Document (ICD)

## legacy-ddraw-compat: DirectDraw Compatibility Layer

**Document ID:** ICD-001
**Version:** 1.0
**Date:** 2026-02-05
**Status:** Approved

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [External Interfaces](#2-external-interfaces)
3. [Internal Interfaces](#3-internal-interfaces)
4. [Data Formats](#4-data-formats)
5. [Error Codes](#5-error-codes)

---

## 1. Introduction

### 1.1 Purpose

This Interface Control Document (ICD) defines all interfaces exposed by and consumed by the legacy-ddraw-compat system. It serves as the authoritative reference for interface contracts.

### 1.2 Scope

This document covers:
- DirectDraw API interface (external, to applications)
- Graphics API interfaces (external, to system)
- Configuration file interface (external, to users)
- Internal module interfaces

### 1.3 Interface Categories

| Category | Direction | Description |
|----------|-----------|-------------|
| DirectDraw API | Inbound | Calls from legacy applications |
| Graphics API | Outbound | Calls to D3D9/OpenGL/GDI |
| Configuration | Inbound | Configuration file parsing |
| Logging | Outbound | Log file output |
| Windows API | Bidirectional | System calls and hooks |

---

## 2. External Interfaces

### 2.1 DirectDraw API Interface

#### 2.1.1 Exported Functions

| Function | Ordinal | Description |
|----------|---------|-------------|
| `DirectDrawCreate` | @1 | Create IDirectDraw object |
| `DirectDrawCreateClipper` | @2 | Create standalone clipper |
| `DirectDrawCreateEx` | @3 | Create IDirectDraw with GUID |
| `DirectDrawEnumerateA` | @4 | Enumerate drivers (ANSI) |
| `DirectDrawEnumerateExA` | @5 | Extended enumerate (ANSI) |
| `DirectDrawEnumerateExW` | @6 | Extended enumerate (Unicode) |
| `DirectDrawEnumerateW` | @7 | Enumerate drivers (Unicode) |

#### 2.1.2 DirectDrawCreate

```cpp
HRESULT WINAPI DirectDrawCreate(
    GUID* lpGUID,               // [in] Driver GUID or NULL for default
    LPDIRECTDRAW* lplpDD,       // [out] Receives IDirectDraw pointer
    IUnknown* pUnkOuter         // [in] Must be NULL (aggregation not supported)
);
```

**Parameters:**

| Parameter | Type | Direction | Description |
|-----------|------|-----------|-------------|
| lpGUID | GUID* | IN | Driver identifier. NULL = default display driver |
| lplpDD | LPDIRECTDRAW* | OUT | Receives pointer to IDirectDraw interface |
| pUnkOuter | IUnknown* | IN | Outer IUnknown for aggregation (must be NULL) |

**Return Values:**

| Value | Description |
|-------|-------------|
| DD_OK | Success |
| DDERR_INVALIDPARAMS | Invalid parameter |
| DDERR_OUTOFMEMORY | Insufficient memory |
| DDERR_NODIRECTDRAWHW | No hardware support |

**Behavior:**
1. If lpGUID is NULL, use default display driver
2. Create DirectDrawImpl instance
3. Return IDirectDraw interface

#### 2.1.3 DirectDrawCreateEx

```cpp
HRESULT WINAPI DirectDrawCreateEx(
    GUID* lpGuid,               // [in] Driver GUID or NULL
    LPVOID* lplpDD,             // [out] Receives interface pointer
    REFIID iid,                 // [in] Requested interface IID
    IUnknown* pUnkOuter         // [in] Must be NULL
);
```

**Supported Interface IDs:**

| IID | Interface |
|-----|-----------|
| IID_IDirectDraw | IDirectDraw |
| IID_IDirectDraw2 | IDirectDraw2 |
| IID_IDirectDraw4 | IDirectDraw4 |
| IID_IDirectDraw7 | IDirectDraw7 |

#### 2.1.4 DirectDrawEnumerateA/W

```cpp
HRESULT WINAPI DirectDrawEnumerateA(
    LPDDENUMCALLBACKA lpCallback,   // [in] Callback function
    LPVOID lpContext                // [in] User-defined context
);

// Callback signature
typedef BOOL (FAR PASCAL* LPDDENUMCALLBACKA)(
    GUID FAR* lpGUID,
    LPSTR lpDriverDescription,
    LPSTR lpDriverName,
    LPVOID lpContext
);
```

**Behavior:**
- Calls callback once with NULL GUID for default display
- Returns DD_OK after enumeration completes
- Stops if callback returns FALSE

### 2.2 IDirectDraw Interface

#### 2.2.1 SetCooperativeLevel

```cpp
HRESULT SetCooperativeLevel(
    HWND hWnd,          // [in] Window handle
    DWORD dwFlags       // [in] Cooperative level flags
);
```

**Flags:**

| Flag | Value | Description |
|------|-------|-------------|
| DDSCL_NORMAL | 0x00000008 | Normal windowed mode |
| DDSCL_FULLSCREEN | 0x00000001 | Exclusive fullscreen |
| DDSCL_EXCLUSIVE | 0x00000010 | Exclusive access |
| DDSCL_ALLOWREBOOT | 0x00000002 | Allow Ctrl+Alt+Del |
| DDSCL_NOWINDOWCHANGES | 0x00000004 | Don't modify window |
| DDSCL_ALLOWMODEX | 0x00000040 | Allow ModeX modes |

**Valid Combinations:**
- `DDSCL_NORMAL` alone (windowed)
- `DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE` (exclusive fullscreen)

#### 2.2.2 SetDisplayMode

```cpp
HRESULT SetDisplayMode(
    DWORD dwWidth,          // [in] Width in pixels
    DWORD dwHeight,         // [in] Height in pixels
    DWORD dwBPP,            // [in] Bits per pixel
    DWORD dwRefreshRate,    // [in] Refresh rate (0 = default)
    DWORD dwFlags           // [in] Flags (0)
);
```

**Behavior:**
1. Validate parameters
2. Configure window for requested mode
3. Initialize renderer if not already done
4. Store mode for GetDisplayMode queries

#### 2.2.3 CreateSurface

```cpp
HRESULT CreateSurface(
    LPDDSURFACEDESC2 lpDDSurfaceDesc,   // [in] Surface description
    LPDIRECTDRAWSURFACE7* lplpDDSurface, // [out] Created surface
    IUnknown* pUnkOuter                  // [in] Must be NULL
);
```

**Surface Description Flags:**

| Flag | Description |
|------|-------------|
| DDSD_CAPS | dwCaps is valid |
| DDSD_WIDTH | dwWidth is valid |
| DDSD_HEIGHT | dwHeight is valid |
| DDSD_PIXELFORMAT | ddpfPixelFormat is valid |
| DDSD_BACKBUFFERCOUNT | dwBackBufferCount is valid |

**Surface Capabilities:**

| Cap | Description |
|-----|-------------|
| DDSCAPS_PRIMARYSURFACE | Primary display surface |
| DDSCAPS_BACKBUFFER | Back buffer surface |
| DDSCAPS_OFFSCREENPLAIN | Off-screen surface |
| DDSCAPS_SYSTEMMEMORY | Store in system memory |
| DDSCAPS_VIDEOMEMORY | Store in video memory |
| DDSCAPS_FLIP | Part of flip chain |
| DDSCAPS_COMPLEX | Complex surface (has attached) |

### 2.3 IDirectDrawSurface Interface

#### 2.3.1 Lock

```cpp
HRESULT Lock(
    LPRECT lpDestRect,          // [in] Region to lock (NULL = entire)
    LPDDSURFACEDESC2 lpDDSD,    // [in/out] Receives surface info
    DWORD dwFlags,              // [in] Lock flags
    HANDLE hEvent               // [in] Not used (NULL)
);
```

**Lock Flags:**

| Flag | Description |
|------|-------------|
| DDLOCK_WAIT | Wait if surface busy |
| DDLOCK_READONLY | Read-only access |
| DDLOCK_WRITEONLY | Write-only access |
| DDLOCK_SURFACEMEMORYPTR | Return surface memory pointer |
| DDLOCK_NOSYSLOCK | Don't take system lock |

**Output:**
- `lpDDSD->lpSurface`: Pointer to pixel data
- `lpDDSD->lPitch`: Bytes per row

#### 2.3.2 Unlock

```cpp
HRESULT Unlock(
    LPRECT lpRect   // [in] Region to unlock (NULL = entire)
);
```

#### 2.3.3 Blt

```cpp
HRESULT Blt(
    LPRECT lpDestRect,              // [in] Destination rectangle
    LPDIRECTDRAWSURFACE7 lpSrc,     // [in] Source surface
    LPRECT lpSrcRect,               // [in] Source rectangle
    DWORD dwFlags,                  // [in] Blit flags
    LPDDBLTFX lpDDBltFx             // [in] Effects structure
);
```

**Blit Flags:**

| Flag | Description |
|------|-------------|
| DDBLT_COLORFILL | Fill with color |
| DDBLT_KEYSRC | Source color key |
| DDBLT_KEYDEST | Destination color key |
| DDBLT_WAIT | Wait for completion |
| DDBLT_ASYNC | Asynchronous blit |
| DDBLT_DDFX | Use effects structure |

#### 2.3.4 BltFast

```cpp
HRESULT BltFast(
    DWORD dwX,                      // [in] Destination X
    DWORD dwY,                      // [in] Destination Y
    LPDIRECTDRAWSURFACE7 lpSrc,     // [in] Source surface
    LPRECT lpSrcRect,               // [in] Source rectangle
    DWORD dwFlags                   // [in] BltFast flags
);
```

**BltFast Flags:**

| Flag | Description |
|------|-------------|
| DDBLTFAST_NOCOLORKEY | No color key |
| DDBLTFAST_SRCCOLORKEY | Source color key |
| DDBLTFAST_DESTCOLORKEY | Destination color key |
| DDBLTFAST_WAIT | Wait for completion |

#### 2.3.5 Flip

```cpp
HRESULT Flip(
    LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride,  // [in] Override target
    DWORD dwFlags                                     // [in] Flip flags
);
```

**Flip Flags:**

| Flag | Description |
|------|-------------|
| DDFLIP_WAIT | Wait for VSync |
| DDFLIP_NOVSYNC | Don't wait for VSync |
| DDFLIP_INTERVAL2 | Flip every 2 VSyncs |
| DDFLIP_INTERVAL3 | Flip every 3 VSyncs |
| DDFLIP_INTERVAL4 | Flip every 4 VSyncs |

### 2.4 IDirectDrawPalette Interface

#### 2.4.1 SetEntries

```cpp
HRESULT SetEntries(
    DWORD dwFlags,          // [in] Flags (0)
    DWORD dwStartingEntry,  // [in] First entry to set
    DWORD dwCount,          // [in] Number of entries
    LPPALETTEENTRY lpEntries // [in] Palette entries
);
```

**PALETTEENTRY Structure:**
```cpp
typedef struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} PALETTEENTRY;
```

#### 2.4.2 GetEntries

```cpp
HRESULT GetEntries(
    DWORD dwFlags,          // [in] Flags (0)
    DWORD dwStartingEntry,  // [in] First entry to get
    DWORD dwCount,          // [in] Number of entries
    LPPALETTEENTRY lpEntries // [out] Receives entries
);
```

### 2.5 Configuration File Interface

#### 2.5.1 File Format

**File:** `ddraw.ini` (Windows INI format)

```ini
; Comment lines start with ; or #
[section]
key=value
key2 = value with spaces
boolkey=true
intkey=123
```

#### 2.5.2 Global Section

**Section:** `[ddraw]`

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| width | int | 0 | Window width (0 = game's width) |
| height | int | 0 | Window height (0 = game's height) |
| fullscreen | bool | false | Start in fullscreen |
| borderless | bool | true | Use borderless windowed |
| maintainaspectratio | bool | true | Preserve aspect ratio |
| renderer | string | "auto" | Renderer: auto, d3d9, opengl, gdi |
| vsync | bool | true | Enable VSync |
| maxfps | int | 0 | Max FPS (0 = unlimited) |
| adjustmouse | bool | true | Scale mouse coordinates |
| lockcursor | bool | false | Confine cursor to window |
| loglevel | string | "info" | Log level: error, warn, info, debug, trace |
| crashdumps | bool | true | Generate crash dumps |

#### 2.5.3 Per-Application Section

**Section:** `[executable.exe]`

Same keys as `[ddraw]`, override global settings for specific executable.

```ini
[game.exe]
maxfps=60
fullscreen=true
```

### 2.6 Log File Interface

#### 2.6.1 File Format

**File:** `ddraw.log`

```
[2026-02-05 14:32:15.123] [INFO ] [12345] Initializing legacy-ddraw-compat v1.0.0
[2026-02-05 14:32:15.125] [DEBUG] [12345] Loading configuration from ddraw.ini
[2026-02-05 14:32:15.130] [INFO ] [12345] Renderer: Direct3D 9
```

**Format:** `[timestamp] [level] [thread_id] message`

#### 2.6.2 Log Levels

| Level | Description |
|-------|-------------|
| ERROR | Errors that may cause malfunction |
| WARN | Warnings about potential issues |
| INFO | Normal operational information |
| DEBUG | Detailed debugging information |
| TRACE | Very detailed trace information |

---

## 3. Internal Interfaces

### 3.1 Module Interfaces

#### 3.1.1 Core Module

```cpp
namespace ldc::core {
    // Factory singleton
    DirectDrawFactory& GetFactory();

    // DirectDraw object access (single instance)
    DirectDrawImpl* GetCurrentDirectDraw();
}
```

#### 3.1.2 Config Module

```cpp
namespace ldc::config {
    // Initialize configuration from file
    bool Initialize(const std::string& path);

    // Get configuration singleton
    const Config& Get();

    // Individual value access
    template<typename T>
    T GetValue(const char* section, const char* key, T defaultValue);
}
```

#### 3.1.3 Logging Module

```cpp
namespace ldc::logging {
    // Initialize logging
    bool Initialize(const std::string& path, LogLevel level);

    // Shutdown logging
    void Shutdown();

    // Log message
    void Log(LogLevel level, const char* fmt, ...);
}
```

#### 3.1.4 Renderer Module

```cpp
namespace ldc::renderer {
    // Create renderer by type
    std::unique_ptr<IRenderer> Create(RendererType type);

    // Create best available renderer
    std::unique_ptr<IRenderer> CreateBest();
}
```

### 3.2 IRenderer Interface

```cpp
class IRenderer {
public:
    // Lifecycle
    virtual bool Initialize(HWND hWnd, uint32_t w, uint32_t h, uint32_t bpp) = 0;
    virtual void Shutdown() = 0;

    // Rendering
    virtual void Present(const void* pixels, uint32_t pitch,
                         uint32_t w, uint32_t h, uint32_t bpp) = 0;
    virtual void SetPalette(const uint32_t* palette256) = 0;

    // Configuration
    virtual void SetVSync(bool enabled) = 0;
    virtual void OnResize(uint32_t w, uint32_t h) = 0;

    // Information
    virtual RendererType GetType() const = 0;
    virtual bool IsAvailable() const = 0;
};
```

### 3.3 Manager Interfaces

#### 3.3.1 SurfaceManager

```cpp
class SurfaceManager {
public:
    HRESULT CreateSurface(const DDSURFACEDESC2& desc, SurfaceImpl** ppSurf);
    HRESULT DestroySurface(SurfaceImpl* pSurf);
    SurfaceImpl* GetPrimary() const;

    // Blit utilities (static for testing)
    static void Blit(const BlitParams& params);
    static void BlitColorKey(const BlitParams& params);
    static void ColorFill(const BlitParams& params);
};
```

#### 3.3.2 DisplayManager

```cpp
class DisplayManager {
public:
    HRESULT SetCooperativeLevel(HWND hWnd, DWORD flags);
    HRESULT SetDisplayMode(uint32_t w, uint32_t h, uint32_t bpp, uint32_t refresh);
    HRESULT RestoreDisplayMode();
    HRESULT GetDisplayMode(DDSURFACEDESC2* pDesc);

    HWND GetWindow() const;
    void SetRenderer(IRenderer* pRenderer);
    IRenderer* GetRenderer() const;

    POINT ClientToGame(POINT pt) const;
    POINT GameToClient(POINT pt) const;
};
```

---

## 4. Data Formats

### 4.1 Pixel Formats

#### 4.1.1 8-bit Paletted

| Field | Value |
|-------|-------|
| dwSize | 32 |
| dwFlags | DDPF_PALETTEINDEXED8 |
| dwRGBBitCount | 8 |

#### 4.1.2 16-bit RGB565

| Field | Value |
|-------|-------|
| dwSize | 32 |
| dwFlags | DDPF_RGB |
| dwRGBBitCount | 16 |
| dwRBitMask | 0xF800 |
| dwGBitMask | 0x07E0 |
| dwBBitMask | 0x001F |

#### 4.1.3 16-bit RGB555

| Field | Value |
|-------|-------|
| dwSize | 32 |
| dwFlags | DDPF_RGB |
| dwRGBBitCount | 16 |
| dwRBitMask | 0x7C00 |
| dwGBitMask | 0x03E0 |
| dwBBitMask | 0x001F |

#### 4.1.4 24-bit RGB888

| Field | Value |
|-------|-------|
| dwSize | 32 |
| dwFlags | DDPF_RGB |
| dwRGBBitCount | 24 |
| dwRBitMask | 0xFF0000 |
| dwGBitMask | 0x00FF00 |
| dwBBitMask | 0x0000FF |

#### 4.1.5 32-bit XRGB8888

| Field | Value |
|-------|-------|
| dwSize | 32 |
| dwFlags | DDPF_RGB |
| dwRGBBitCount | 32 |
| dwRBitMask | 0x00FF0000 |
| dwGBitMask | 0x0000FF00 |
| dwBBitMask | 0x000000FF |

### 4.2 Surface Memory Layout

**Linear layout:** Pixels stored row by row, top to bottom.

```
Pitch = (width * bpp / 8) aligned to 4/8/16 bytes
Total size = pitch * height

Address of pixel(x, y) = base + y * pitch + x * (bpp / 8)
```

### 4.3 Palette Format

256 entries, each 4 bytes (RGBX):

```cpp
struct PaletteEntry {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t flags;  // Usually 0
};
```

---

## 5. Error Codes

### 5.1 Success Codes

| Code | Value | Description |
|------|-------|-------------|
| DD_OK | 0 | Operation successful |
| DD_FALSE | 1 | Operation returned FALSE |

### 5.2 Error Codes

| Code | Value | Description |
|------|-------|-------------|
| DDERR_ALREADYINITIALIZED | 0x88760005 | Already initialized |
| DDERR_CANNOTATTACHSURFACE | 0x88760006 | Cannot attach surface |
| DDERR_CANNOTDETACHSURFACE | 0x88760007 | Cannot detach surface |
| DDERR_CURRENTLYNOTAVAIL | 0x88760008 | Currently not available |
| DDERR_EXCEPTION | 0x88760009 | Exception occurred |
| DDERR_GENERIC | 0x8876000A | Undefined error |
| DDERR_HEIGHTALIGN | 0x8876000B | Height alignment error |
| DDERR_INCOMPATIBLEPRIMARY | 0x8876000C | Incompatible primary |
| DDERR_INVALIDCAPS | 0x8876000D | Invalid capabilities |
| DDERR_INVALIDCLIPLIST | 0x8876000E | Invalid clip list |
| DDERR_INVALIDMODE | 0x8876000F | Invalid display mode |
| DDERR_INVALIDOBJECT | 0x88760010 | Invalid object |
| DDERR_INVALIDPARAMS | 0x88760011 | Invalid parameters |
| DDERR_INVALIDPIXELFORMAT | 0x88760012 | Invalid pixel format |
| DDERR_INVALIDRECT | 0x88760013 | Invalid rectangle |
| DDERR_LOCKEDSURFACES | 0x88760014 | Surfaces are locked |
| DDERR_NO3D | 0x88760015 | No 3D support |
| DDERR_NOALPHAHW | 0x88760016 | No alpha hardware |
| DDERR_NOCLIPLIST | 0x88760017 | No clip list |
| DDERR_NOCOLORCONVHW | 0x88760018 | No color conversion HW |
| DDERR_NOCOOPERATIVELEVELSET | 0x88760019 | Cooperative level not set |
| DDERR_NOCOLORKEY | 0x8876001A | No color key |
| DDERR_NODIRECTDRAWHW | 0x8876001B | No DirectDraw hardware |
| DDERR_NOEXCLUSIVEMODE | 0x8876001C | Not in exclusive mode |
| DDERR_NOFLIPHW | 0x8876001D | No flip hardware |
| DDERR_NOTFOUND | 0x8876001E | Not found |
| DDERR_NOTFLIPPABLE | 0x8876001F | Surface not flippable |
| DDERR_OUTOFMEMORY | 0x88760020 | Out of memory |
| DDERR_OUTOFVIDEOMEMORY | 0x88760021 | Out of video memory |
| DDERR_SURFACEBUSY | 0x88760022 | Surface is busy |
| DDERR_SURFACELOST | 0x88760023 | Surface was lost |
| DDERR_UNSUPPORTED | 0x88760024 | Operation not supported |
| DDERR_WASSTILLDRAWING | 0x88760025 | Was still drawing |
| DDERR_WRONGMODE | 0x88760026 | Wrong mode |

### 5.3 Error Handling Guidelines

1. **Validation errors** (DDERR_INVALIDPARAMS): Check inputs immediately
2. **State errors** (DDERR_NOCOOPERATIVELEVELSET): Verify prerequisites
3. **Resource errors** (DDERR_OUTOFMEMORY): Attempt cleanup and retry
4. **Lost surface** (DDERR_SURFACELOST): Call Restore() and retry

---

## Appendix A: GUID Definitions

```cpp
// IDirectDraw GUIDs
DEFINE_GUID(IID_IDirectDraw,  0x6C14DB80,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
DEFINE_GUID(IID_IDirectDraw2, 0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_GUID(IID_IDirectDraw4, 0x9c59509a,0x39bd,0x11d1,0x8c,0x4a,0x00,0xc0,0x4f,0xd9,0x30,0xc5);
DEFINE_GUID(IID_IDirectDraw7, 0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b);

// IDirectDrawSurface GUIDs
DEFINE_GUID(IID_IDirectDrawSurface,  0x6C14DB81,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
DEFINE_GUID(IID_IDirectDrawSurface2, 0x57805885,0x6eec,0x11cf,0x94,0x41,0xa8,0x23,0x03,0xc1,0x0e,0x27);
DEFINE_GUID(IID_IDirectDrawSurface3, 0xDA044E00,0x69B2,0x11D0,0xA1,0xD5,0x00,0xAA,0x00,0xB8,0xDF,0xBB);
DEFINE_GUID(IID_IDirectDrawSurface4, 0x0B2B8630,0xAD35,0x11D0,0x8E,0xA6,0x00,0x60,0x97,0x97,0xEA,0x5B);
DEFINE_GUID(IID_IDirectDrawSurface7, 0x06675a80,0x3b9b,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b);

// IDirectDrawPalette GUID
DEFINE_GUID(IID_IDirectDrawPalette, 0x6C14DB84,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);

// IDirectDrawClipper GUID
DEFINE_GUID(IID_IDirectDrawClipper, 0x6C14DB85,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
```

---

*End of Document*
