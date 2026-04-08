# Building legacy-ddraw-compat

## Requirements

- Visual Studio 2022 (v143 toolset)
- Windows SDK 10.0
- Microsoft Detours library

## Setting up Microsoft Detours

1. **Download Detours** from GitHub:
   ```
   git clone https://github.com/microsoft/Detours.git
   ```

2. **Build Detours**:
   ```
   cd Detours
   nmake
   ```

3. **Copy files to project**:
   Create the following directory structure in the project:
   ```
   legacy-ddraw-compat/
   └── external/
       └── detours/
           ├── include/
           │   └── detours.h
           ├── lib.X86/
           │   └── detours.lib
           └── lib.X64/
               └── detours.lib
   ```

   Copy from Detours build:
   - `Detours/include/detours.h` → `external/detours/include/`
   - `Detours/lib.X86/detours.lib` → `external/detours/lib.X86/`
   - `Detours/lib.X64/detours.lib` → `external/detours/lib.X64/`

## Building

1. Open `legacy-ddraw-compat.sln` in Visual Studio 2022
2. Select configuration (Debug/Release) and platform (Win32 for 32-bit apps)
3. Build → Build Solution

Output: `bin\Win32\Release\ddraw.dll`

## Usage

1. Copy `ddraw.dll` to the application's folder (next to the .exe)
2. Copy `ddraw.ini` to the same folder
3. Edit `ddraw.ini` to configure options
4. Run the application

## Configuration (ddraw.ini)

```ini
[ddraw]
windowed=true      ; Run in window
maintas=true       ; Maintain aspect ratio
smooth=true        ; Smooth scaling
adjmouse=true      ; Mouse coordinate transform

[compatibility]
fixpitch=true      ; Surface pitch alignment
hooks=true         ; Win7 API hooks (GetVersionEx, etc.)

[debug]
log=false          ; Debug logging
```

## What the hooks do

When `hooks=true`, the DLL intercepts these Windows APIs to report as Windows 7:

- `GetVersionExA/W` - Returns Windows 7 SP1 (6.1.7601)
- `RtlGetVersion` - Returns Windows 7 version
- `VerifyVersionInfoA/W` - Makes version checks pass
- `SetProcessDPIAware` - Blocked (prevents DPI issues)
- `SetProcessDpiAwarenessContext` - Blocked

This allows Windows 7 applications that check OS version to run on Windows 10/11.
