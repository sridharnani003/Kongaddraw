# Build and Release Guide

## legacy-ddraw-compat: DirectDraw Compatibility Layer

**Document ID:** BUILD-001
**Version:** 1.0
**Date:** 2026-02-05
**Status:** Approved

---

## Table of Contents

1. [Prerequisites](#1-prerequisites)
2. [Build Environment Setup](#2-build-environment-setup)
3. [Building the Project](#3-building-the-project)
4. [Configuration Options](#4-configuration-options)
5. [Testing](#5-testing)
6. [Release Process](#6-release-process)
7. [Troubleshooting](#7-troubleshooting)

---

## 1. Prerequisites

### 1.1 Required Software

| Software | Version | Purpose |
|----------|---------|---------|
| Visual Studio | 2022 (17.0+) | IDE and compiler |
| Windows SDK | 10.0.19041.0+ | Windows headers and libraries |
| Git | 2.30+ | Version control |
| CMake | 3.20+ | Build system (optional) |

### 1.2 Visual Studio Workloads

Install via Visual Studio Installer:

- **Desktop development with C++**
  - MSVC v143 - VS 2022 C++ x64/x86 build tools
  - Windows 10/11 SDK
  - C++ CMake tools (optional)

### 1.3 Optional Tools

| Tool | Purpose |
|------|---------|
| Google Test | Unit testing |
| Doxygen | Documentation generation |
| OpenCppCoverage | Code coverage analysis |

---

## 2. Build Environment Setup

### 2.1 Clone Repository

```bash
git clone https://github.com/yourorg/legacy-ddraw-compat.git
cd legacy-ddraw-compat
```

### 2.2 Directory Structure

```
legacy-ddraw-compat/
├── docs/                   # Documentation
├── include/                # Header files
│   ├── core/
│   ├── interfaces/
│   ├── renderer/
│   ├── config/
│   ├── logging/
│   ├── diagnostics/
│   └── hooks/
├── src/                    # Source files
│   ├── core/
│   ├── interfaces/
│   ├── renderer/
│   ├── config/
│   ├── logging/
│   ├── diagnostics/
│   ├── hooks/
│   └── third_party_derived/
├── tests/                  # Test files
│   ├── unit/
│   └── integration/
├── configs/                # Configuration templates
├── tools/                  # Build/test tools
├── legacy-ddraw-compat.sln # Visual Studio solution
└── CMakeLists.txt         # CMake build (optional)
```

### 2.3 Environment Variables (Optional)

```batch
:: Set for custom SDK location
set WINDOWSSDKDIR=C:\Program Files (x86)\Windows Kits\10

:: Set for custom VS location
set VSINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Professional
```

---

## 3. Building the Project

### 3.1 Visual Studio Build

#### 3.1.1 Open Solution

1. Launch Visual Studio 2022
2. Open `legacy-ddraw-compat.sln`

#### 3.1.2 Select Configuration

| Configuration | Use Case |
|---------------|----------|
| Debug | Development, debugging |
| Release | Production builds |
| RelWithDebInfo | Release with debug symbols |

#### 3.1.3 Select Platform

| Platform | Output |
|----------|--------|
| x86 | 32-bit DLL |
| x64 | 64-bit DLL |

#### 3.1.4 Build

- **Build All:** Build > Build Solution (Ctrl+Shift+B)
- **Rebuild:** Build > Rebuild Solution
- **Clean:** Build > Clean Solution

### 3.2 Command Line Build (MSBuild)

```batch
:: Open Developer Command Prompt for VS 2022

:: Build Debug x86
msbuild legacy-ddraw-compat.sln /p:Configuration=Debug /p:Platform=x86

:: Build Release x86
msbuild legacy-ddraw-compat.sln /p:Configuration=Release /p:Platform=x86

:: Build Release x64
msbuild legacy-ddraw-compat.sln /p:Configuration=Release /p:Platform=x64

:: Build all configurations
msbuild legacy-ddraw-compat.sln /p:Configuration=Debug /p:Platform=x86
msbuild legacy-ddraw-compat.sln /p:Configuration=Debug /p:Platform=x64
msbuild legacy-ddraw-compat.sln /p:Configuration=Release /p:Platform=x86
msbuild legacy-ddraw-compat.sln /p:Configuration=Release /p:Platform=x64
```

### 3.3 CMake Build (Alternative)

```bash
# Create build directory
mkdir build && cd build

# Configure for x86
cmake -G "Visual Studio 17 2022" -A Win32 ..

# Configure for x64
cmake -G "Visual Studio 17 2022" -A x64 ..

# Build
cmake --build . --config Release
```

### 3.4 Build Output

| Configuration | Platform | Output Path |
|---------------|----------|-------------|
| Debug | x86 | `out/Debug/x86/ddraw.dll` |
| Debug | x64 | `out/Debug/x64/ddraw.dll` |
| Release | x86 | `out/Release/x86/ddraw.dll` |
| Release | x64 | `out/Release/x64/ddraw.dll` |

---

## 4. Configuration Options

### 4.1 Preprocessor Definitions

| Definition | Effect |
|------------|--------|
| `_DEBUG` | Enable debug logging and assertions |
| `NDEBUG` | Disable assertions (Release) |
| `LDC_ENABLE_D3D9` | Enable Direct3D 9 renderer |
| `LDC_ENABLE_OPENGL` | Enable OpenGL renderer |
| `LDC_ENABLE_GDI` | Enable GDI renderer (always on) |
| `LDC_VERSION_MAJOR` | Major version number |
| `LDC_VERSION_MINOR` | Minor version number |
| `LDC_VERSION_PATCH` | Patch version number |

### 4.2 Project Settings

#### 4.2.1 General

| Setting | Debug | Release |
|---------|-------|---------|
| Configuration Type | Dynamic Library (.dll) | Dynamic Library (.dll) |
| Character Set | Unicode | Unicode |
| Whole Program Optimization | No | Yes |

#### 4.2.2 C/C++

| Setting | Debug | Release |
|---------|-------|---------|
| Optimization | Disabled (/Od) | Maximum (/O2) |
| Runtime Library | Multi-threaded Debug DLL (/MDd) | Multi-threaded DLL (/MD) |
| Warning Level | Level 4 (/W4) | Level 4 (/W4) |
| Treat Warnings as Errors | Yes (/WX) | Yes (/WX) |
| C++ Language Standard | C++17 | C++17 |

#### 4.2.3 Linker

| Setting | Value |
|---------|-------|
| Module Definition File | exports.def |
| Additional Dependencies | d3d9.lib, opengl32.lib, user32.lib, gdi32.lib |

---

## 5. Testing

### 5.1 Build Tests

```batch
:: Build test project
msbuild legacy-ddraw-compat.sln /t:Tests /p:Configuration=Debug /p:Platform=x86
```

### 5.2 Run Unit Tests

```batch
:: Run all tests
.\out\Debug\x86\Tests.exe

:: Run with verbose output
.\out\Debug\x86\Tests.exe --gtest_output=xml:results.xml

:: Run specific test suite
.\out\Debug\x86\Tests.exe --gtest_filter=IniParserTest.*

:: Run with code coverage
OpenCppCoverage --sources src\ --modules ddraw.dll -- .\out\Debug\x86\Tests.exe
```

### 5.3 Integration Tests

```batch
:: Build and run integration tests
msbuild legacy-ddraw-compat.sln /t:IntegrationTests /p:Configuration=Debug
.\out\Debug\x86\IntegrationTests.exe
```

### 5.4 Test Harness

```batch
:: Build test harness
msbuild tools\TestHarness\TestHarness.sln /p:Configuration=Release

:: Run test harness
copy out\Release\x86\ddraw.dll tools\TestHarness\Release\
.\tools\TestHarness\Release\TestHarness.exe
```

---

## 6. Release Process

### 6.1 Version Numbering

Format: `MAJOR.MINOR.PATCH`

| Component | Increment When |
|-----------|---------------|
| MAJOR | Breaking changes |
| MINOR | New features, backward compatible |
| PATCH | Bug fixes |

### 6.2 Pre-Release Checklist

- [ ] All tests pass
- [ ] Version number updated in:
  - [ ] `include/core/Version.h`
  - [ ] `legacy-ddraw-compat.rc`
  - [ ] `CMakeLists.txt`
- [ ] CHANGELOG.md updated
- [ ] Documentation reviewed
- [ ] Code reviewed
- [ ] Release branch created

### 6.3 Build Release

```batch
:: Build all release configurations
msbuild legacy-ddraw-compat.sln /p:Configuration=Release /p:Platform=x86
msbuild legacy-ddraw-compat.sln /p:Configuration=Release /p:Platform=x64

:: Verify builds
dir out\Release\x86\ddraw.dll
dir out\Release\x64\ddraw.dll
```

### 6.4 Package Release

```batch
:: Create release directory
mkdir release\v1.0.0

:: Copy x86 files
mkdir release\v1.0.0\x86
copy out\Release\x86\ddraw.dll release\v1.0.0\x86\
copy configs\ddraw.ini.default release\v1.0.0\x86\ddraw.ini

:: Copy x64 files
mkdir release\v1.0.0\x64
copy out\Release\x64\ddraw.dll release\v1.0.0\x64\
copy configs\ddraw.ini.default release\v1.0.0\x64\ddraw.ini

:: Copy documentation
copy README.md release\v1.0.0\
copy LICENSE release\v1.0.0\
copy LICENSE-THIRD-PARTY.md release\v1.0.0\
copy CHANGELOG.md release\v1.0.0\

:: Create archive
cd release
tar -czvf legacy-ddraw-compat-v1.0.0.zip v1.0.0
```

### 6.5 Release Contents

```
legacy-ddraw-compat-v1.0.0/
├── x86/
│   ├── ddraw.dll
│   └── ddraw.ini
├── x64/
│   ├── ddraw.dll
│   └── ddraw.ini
├── README.md
├── LICENSE
├── LICENSE-THIRD-PARTY.md
└── CHANGELOG.md
```

### 6.6 Post-Release

1. Create Git tag: `git tag -a v1.0.0 -m "Release v1.0.0"`
2. Push tag: `git push origin v1.0.0`
3. Create GitHub release
4. Upload release archive
5. Update documentation site

---

## 7. Troubleshooting

### 7.1 Build Errors

#### Error: Cannot find Windows SDK

**Symptom:**
```
fatal error C1083: Cannot open include file: 'windows.h'
```

**Solution:**
1. Install Windows 10 SDK via Visual Studio Installer
2. Or set `WINDOWSSDKDIR` environment variable

#### Error: LNK2019 Unresolved External

**Symptom:**
```
error LNK2019: unresolved external symbol _Direct3DCreate9@4
```

**Solution:**
1. Verify d3d9.lib is in Additional Dependencies
2. Verify Windows SDK paths are correct

#### Error: Module Definition File Not Found

**Symptom:**
```
LNK1104: cannot open file 'exports.def'
```

**Solution:**
1. Verify exports.def exists in project root
2. Check Module Definition File path in Linker settings

### 7.2 Runtime Issues

#### DLL Not Loading

**Symptom:** Game crashes immediately or ignores ddraw.dll

**Diagnosis:**
1. Check architecture matches (x86 game needs x86 DLL)
2. Use Dependency Walker to check for missing dependencies
3. Check Windows Event Viewer for loader errors

#### Missing Visual C++ Runtime

**Symptom:**
```
The code execution cannot proceed because VCRUNTIME140.dll was not found
```

**Solution:**
1. Install Visual C++ Redistributable 2022
2. Or use static runtime (/MT) build

### 7.3 Debug Tips

#### Enable Debug Logging

In `ddraw.ini`:
```ini
[ddraw]
loglevel=debug
```

#### Attach Debugger

1. Set breakpoint in Visual Studio
2. Debug > Attach to Process
3. Select game executable

#### Generate Crash Dump

In `ddraw.ini`:
```ini
[ddraw]
crashdumps=true
```

Dump files saved to application directory as `ddraw_crash.dmp`

---

## Appendix A: Build Scripts

### A.1 Full Build Script (build_all.bat)

```batch
@echo off
setlocal

echo Building legacy-ddraw-compat...
echo.

:: Set paths
set SOLUTION=legacy-ddraw-compat.sln
set OUTDIR=out

:: Clean
echo Cleaning...
if exist %OUTDIR% rmdir /s /q %OUTDIR%

:: Build Debug x86
echo Building Debug x86...
msbuild %SOLUTION% /p:Configuration=Debug /p:Platform=x86 /v:minimal
if errorlevel 1 goto :error

:: Build Debug x64
echo Building Debug x64...
msbuild %SOLUTION% /p:Configuration=Debug /p:Platform=x64 /v:minimal
if errorlevel 1 goto :error

:: Build Release x86
echo Building Release x86...
msbuild %SOLUTION% /p:Configuration=Release /p:Platform=x86 /v:minimal
if errorlevel 1 goto :error

:: Build Release x64
echo Building Release x64...
msbuild %SOLUTION% /p:Configuration=Release /p:Platform=x64 /v:minimal
if errorlevel 1 goto :error

echo.
echo Build successful!
goto :end

:error
echo.
echo Build failed!
exit /b 1

:end
endlocal
```

### A.2 Test Script (run_tests.bat)

```batch
@echo off
setlocal

echo Running tests...
echo.

:: Run unit tests
echo Unit Tests:
.\out\Debug\x86\Tests.exe --gtest_output=xml:test_results.xml
if errorlevel 1 (
    echo Unit tests FAILED
    exit /b 1
)

echo.
echo All tests passed!
endlocal
```

---

## Appendix B: CI/CD Configuration

### B.1 GitHub Actions (`.github/workflows/build.yml`)

```yaml
name: Build

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build:
    runs-on: windows-latest

    strategy:
      matrix:
        configuration: [Debug, Release]
        platform: [x86, x64]

    steps:
    - uses: actions/checkout@v3

    - name: Add MSBuild to PATH
      uses: microsoft/setup-msbuild@v1

    - name: Build
      run: msbuild legacy-ddraw-compat.sln /p:Configuration=${{ matrix.configuration }} /p:Platform=${{ matrix.platform }}

    - name: Run Tests
      if: matrix.configuration == 'Debug'
      run: .\out\${{ matrix.configuration }}\${{ matrix.platform }}\Tests.exe

    - name: Upload Artifacts
      uses: actions/upload-artifact@v3
      with:
        name: ddraw-${{ matrix.configuration }}-${{ matrix.platform }}
        path: out/${{ matrix.configuration }}/${{ matrix.platform }}/ddraw.dll
```

---

*End of Document*
