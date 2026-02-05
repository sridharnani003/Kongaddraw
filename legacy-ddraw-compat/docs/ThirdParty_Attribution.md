# Third-Party Attribution Ledger

## legacy-ddraw-compat: DirectDraw Compatibility Layer

**Document ID:** ATTR-001
**Version:** 1.0
**Date:** 2026-02-05
**Status:** Active

---

## 1. Introduction

This document maintains a ledger of all third-party code incorporated into the legacy-ddraw-compat project. Each entry includes:

- Source file identification
- Origin and commit reference
- License information
- Summary of modifications
- Location in this project

---

## 2. License Summary

| Project | License | URL |
|---------|---------|-----|
| cnc-ddraw | MIT | https://github.com/FunkyFr3sh/cnc-ddraw |

---

## 3. Attribution Entries

### 3.1 INI Parser

#### 3.1.1 Entry Details

| Attribute | Value |
|-----------|-------|
| **Entry ID** | TP-001 |
| **Origin Project** | cnc-ddraw |
| **Origin File** | `src/ini.c` |
| **Commit Hash** | (to be filled when code is reused) |
| **License** | MIT |
| **Target Location** | `src/config/IniParser.cpp` |

#### 3.1.2 Modifications

1. Converted from C to C++
2. Added RAII pattern for file handle management
3. Added `std::string` return types instead of C strings
4. Added error code support
5. Renamed functions with `ldc_` prefix
6. Added const correctness
7. Added UTF-8 BOM detection and handling
8. Added comprehensive unit tests

#### 3.1.3 Attribution Header

```cpp
// =============================================================================
// Derived from cnc-ddraw (https://github.com/FunkyFr3sh/cnc-ddraw)
// Original file: src/ini.c
// Commit: [HASH]
// License: MIT
//
// Modifications by legacy-ddraw-compat project:
// - Converted to C++ with RAII patterns
// - Added std::string return types
// - Added error handling
// - Renamed functions with ldc_ prefix
// - Added unit tests
// =============================================================================
```

---

### 3.2 CRC32 Implementation

#### 3.2.1 Entry Details

| Attribute | Value |
|-----------|-------|
| **Entry ID** | TP-002 |
| **Origin Project** | cnc-ddraw |
| **Origin File** | `src/crc32.c` |
| **Commit Hash** | (to be filled when code is reused) |
| **License** | MIT (algorithm is public domain) |
| **Target Location** | `src/diagnostics/Crc32.cpp` |

#### 3.2.2 Modifications

1. Converted to C++ class
2. Added streaming interface for incremental calculation
3. Added compile-time lookup table generation (constexpr)
4. Added unit tests with known test vectors
5. Documented algorithm variant (ISO 3309 polynomial)

#### 3.2.3 Attribution Header

```cpp
// =============================================================================
// Derived from cnc-ddraw (https://github.com/FunkyFr3sh/cnc-ddraw)
// Original file: src/crc32.c
// Commit: [HASH]
// License: MIT
//
// CRC32 algorithm: ISO 3309 / ITU-T V.42 polynomial (0xEDB88320)
// Algorithm is public domain; implementation derived from cnc-ddraw.
//
// Modifications by legacy-ddraw-compat project:
// - Converted to C++ class
// - Added streaming interface
// - Added constexpr lookup table
// - Added unit tests
// =============================================================================
```

---

### 3.3 Debug Timestamp Formatting

#### 3.3.1 Entry Details

| Attribute | Value |
|-----------|-------|
| **Entry ID** | TP-003 |
| **Origin Project** | cnc-ddraw |
| **Origin File** | `src/debug.c` |
| **Commit Hash** | (to be filled when code is reused) |
| **License** | MIT |
| **Target Location** | `src/logging/LogFormatter.cpp` |

#### 3.3.2 Modifications

1. Converted to C++
2. Used safe string formatting
3. Made thread ID optional
4. Added ISO 8601 format option
5. Added custom format string support

#### 3.3.3 Attribution Header

```cpp
// =============================================================================
// Derived from cnc-ddraw (https://github.com/FunkyFr3sh/cnc-ddraw)
// Original file: src/debug.c (timestamp formatting)
// Commit: [HASH]
// License: MIT
//
// Modifications by legacy-ddraw-compat project:
// - Converted to C++
// - Safe string formatting
// - Added format options
// =============================================================================
```

---

### 3.4 Flag-to-String Converters

#### 3.4.1 Entry Details

| Attribute | Value |
|-----------|-------|
| **Entry ID** | TP-004 |
| **Origin Project** | cnc-ddraw |
| **Origin File** | `src/debug.c` |
| **Commit Hash** | (to be filled when code is reused) |
| **License** | MIT |
| **Target Location** | `src/diagnostics/FlagStrings.cpp` |

#### 3.4.2 Modifications

1. Converted to C++
2. Removed static buffers (thread-safety)
3. Returns `std::string` instead of `char*`
4. Uses `std::ostringstream` for building
5. Completed flag coverage from Windows SDK
6. Added unit tests

#### 3.4.3 Attribution Header

```cpp
// =============================================================================
// Derived from cnc-ddraw (https://github.com/FunkyFr3sh/cnc-ddraw)
// Original file: src/debug.c (flag conversion functions)
// Commit: [HASH]
// License: MIT
//
// Modifications by legacy-ddraw-compat project:
// - Converted to C++ with thread-safe implementation
// - Returns std::string
// - Extended flag coverage
// - Added unit tests
// =============================================================================
```

---

## 4. Pending Reuse Candidates

The following code sections are identified for potential reuse but have not yet been incorporated:

| ID | Component | Status | Notes |
|----|-----------|--------|-------|
| TP-P01 | IAT Hook Pattern | Pending | Will be reimplemented with reference to pattern |
| TP-P02 | FPS Limiter Logic | Pending | Will study and reimplement |
| TP-P03 | Palette Conversion | Pending | Will implement standard conversion |

---

## 5. Excluded Components

The following components from cnc-ddraw are explicitly NOT reused:

| Component | Reason |
|-----------|--------|
| Detours library | Too large; will use official package if needed |
| Full renderer implementations | Will implement fresh |
| Game-specific hacks | Out of initial scope |
| Configuration GUI | Out of scope |

---

## 6. License Text

### 6.1 MIT License (cnc-ddraw)

```
MIT License

Copyright (c) FunkyFr3sh

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## 7. Compliance Verification

### 7.1 Checklist

For each reused component, verify:

- [ ] Attribution header added to source file
- [ ] Entry added to this ledger
- [ ] License file included in distribution
- [ ] Unit tests added
- [ ] Code review completed

### 7.2 Distribution Requirements

Release packages must include:

1. `LICENSE` - Project license
2. `LICENSE-THIRD-PARTY.md` - Third-party license notices
3. This attribution information (or reference to it)

---

## 8. Change Log

| Date | Version | Changes |
|------|---------|---------|
| 2026-02-05 | 1.0 | Initial document with identified reuse candidates |

---

*End of Document*
