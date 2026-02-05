# legacy-ddraw-compat

A DirectDraw compatibility layer for running legacy Windows games and applications on modern systems.

## Overview

legacy-ddraw-compat is a drop-in replacement DLL (`ddraw.dll`) that intercepts DirectDraw API calls from legacy applications and translates them to modern graphics APIs (GDI, OpenGL, or Direct3D 9).

This project is inspired by and partially derived from [cnc-ddraw](https://github.com/FunkyFr3sh/cnc-ddraw) (MIT License).

## Features

- **Multi-renderer support**: GDI, OpenGL, and Direct3D 9 backends with automatic selection
- **Display modes**: Windowed, borderless fullscreen, and exclusive fullscreen
- **Resolution scaling**: Automatic upscaling with aspect ratio preservation
- **VSync support**: Reduce screen tearing
- **FPS limiting**: Configurable frame rate limiting
- **Configuration system**: INI-based configuration with per-game overrides
- **Logging**: Comprehensive diagnostic logging

## Supported Platforms

- Windows 7 SP1 through Windows 11
- Both x86 (32-bit) and x64 (64-bit) builds

## Installation

1. Download the appropriate DLL for your game:
   - `ddraw.dll` (x86) for 32-bit games
   - `ddraw.dll` (x64) for 64-bit games

2. Copy `ddraw.dll` to the game's directory (same folder as the game executable)

3. Optionally copy `ddraw.ini` for configuration

4. Run the game normally

## Configuration

Configuration is done through `ddraw.ini`. Place this file in the same directory as the game executable.

```ini
[ddraw]
; Window settings
width=0          ; 0 = use game's width
height=0         ; 0 = use game's height
fullscreen=false
borderless=true

; Rendering
renderer=auto    ; auto, gdi, opengl, d3d9
vsync=true
maxfps=0         ; 0 = unlimited

; Input
adjustmouse=true
lockcursor=false

; Debug
loglevel=info    ; error, warn, info, debug, trace
```

### Per-Game Overrides

You can create sections for specific games:

```ini
[game.exe]
maxfps=60
fullscreen=true
```

## Building

### Requirements

- Visual Studio 2022
- Windows SDK 10.0 or later
- C++17 compiler support

### Build Steps

1. Open `legacy-ddraw-compat.sln` in Visual Studio 2022

2. Select configuration:
   - Debug|Win32 or Debug|x64 for development
   - Release|Win32 or Release|x64 for distribution

3. Build the solution (F7 or Build → Build Solution)

4. Output DLL is in `bin/<Platform>/<Configuration>/ddraw.dll`

### Build from Command Line

```batch
msbuild legacy-ddraw-compat.sln /p:Configuration=Release /p:Platform=Win32
msbuild legacy-ddraw-compat.sln /p:Configuration=Release /p:Platform=x64
```

## Project Structure

```
legacy-ddraw-compat/
├── docs/                    # Documentation
│   ├── SRS.md              # Software Requirements Specification
│   ├── SSD.md              # Software System Description
│   ├── SDD.md              # Software Design Document
│   ├── ICD.md              # Interface Control Document
│   ├── Test_Plan.md        # Test Plan
│   └── ...
├── include/                 # Header files
│   ├── core/               # Core definitions
│   ├── interfaces/         # DirectDraw interface implementations
│   ├── renderer/           # Rendering backends
│   ├── config/             # Configuration system
│   └── logging/            # Logging system
├── src/                     # Source files
│   ├── core/               # Core implementation
│   ├── interfaces/         # Interface implementations
│   ├── renderer/           # Renderer implementations
│   ├── config/             # Configuration implementation
│   └── logging/            # Logging implementation
├── tests/                   # Test code
├── configs/                 # Default configuration files
└── tools/                   # Build and utility tools
```

## Documentation

- [Software Requirements Specification](docs/SRS.md)
- [Software System Description](docs/SSD.md)
- [Software Design Document](docs/SDD.md)
- [Interface Control Document](docs/ICD.md)
- [Test Plan](docs/Test_Plan.md)
- [Build and Release](docs/Build_and_Release.md)

## Contributing

Contributions are welcome. Please ensure:

1. Code follows the existing style
2. New code is properly documented
3. Tests are added for new functionality
4. Third-party code is properly attributed

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

### Third-Party Attributions

This project contains code derived from:

- **cnc-ddraw** (https://github.com/FunkyFr3sh/cnc-ddraw) - MIT License

See [docs/ThirdParty_Attribution.md](docs/ThirdParty_Attribution.md) for detailed attributions.

## Acknowledgments

Special thanks to [FunkyFr3sh](https://github.com/FunkyFr3sh) and the cnc-ddraw project for inspiration and reference implementation.
