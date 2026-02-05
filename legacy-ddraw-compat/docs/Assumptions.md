# Assumptions Document

## legacy-ddraw-compat: DirectDraw Compatibility Layer

**Document ID:** ASSUMP-001
**Version:** 1.0
**Date:** 2026-02-05
**Status:** Approved

---

## 1. Introduction

### 1.1 Purpose

This document records assumptions made during the design and implementation of the legacy-ddraw-compat project. Each assumption includes rationale, risk if invalid, and mitigation strategy.

### 1.2 Scope

Assumptions cover:
- Target application behavior
- System environment
- Technical constraints
- User expectations

---

## 2. Application Assumptions

### A-001: Standard DirectDraw API Usage

| Attribute | Value |
|-----------|-------|
| ID | A-001 |
| Category | Application |
| Assumption | Target applications use documented DirectDraw API only |
| Rationale | Undocumented behavior is unpredictable and varies by DirectDraw version |
| Risk if Invalid | Medium - Application may not function correctly |
| Mitigation | Add game-specific workarounds as discovered |
| Status | Active |

### A-002: Single DirectDraw Instance

| Attribute | Value |
|-----------|-------|
| ID | A-002 |
| Category | Application |
| Assumption | Applications create only one IDirectDraw instance at a time |
| Rationale | Most legacy games follow this pattern |
| Risk if Invalid | Low - Multiple instance support can be added |
| Mitigation | Track and manage multiple instances if needed |
| Status | Active |

### A-003: Sequential Surface Operations

| Attribute | Value |
|-----------|-------|
| ID | A-003 |
| Category | Application |
| Assumption | Surface Lock/Unlock are called sequentially, not from multiple threads simultaneously |
| Rationale | Original DirectDraw was not thread-safe for surface operations |
| Risk if Invalid | Medium - Race conditions possible |
| Mitigation | Add mutex protection if multi-threaded access detected |
| Status | Active |

### A-004: Common Display Modes

| Attribute | Value |
|-----------|-------|
| ID | A-004 |
| Category | Application |
| Assumption | Applications primarily use 640x480, 800x600, or 1024x768 |
| Rationale | These were standard resolutions in the DirectDraw era |
| Risk if Invalid | Low - Other resolutions will still work |
| Mitigation | Support arbitrary resolutions |
| Status | Active |

### A-005: 8-bit or 16-bit Color Depth

| Attribute | Value |
|-----------|-------|
| ID | A-005 |
| Category | Application |
| Assumption | Most target applications use 8-bit (palettized) or 16-bit color |
| Rationale | 32-bit color was less common in early DirectDraw games |
| Risk if Invalid | Low - 32-bit support included |
| Mitigation | Full 8/16/24/32-bit support implemented |
| Status | Active |

### A-006: Primary Surface with Backbuffer

| Attribute | Value |
|-----------|-------|
| ID | A-006 |
| Category | Application |
| Assumption | Most applications use double-buffered rendering (primary + backbuffer) |
| Rationale | Standard practice for flicker-free animation |
| Risk if Invalid | Low - Single and triple buffering also supported |
| Mitigation | Support all buffer configurations |
| Status | Active |

### A-007: Window Handle Provided

| Attribute | Value |
|-----------|-------|
| ID | A-007 |
| Category | Application |
| Assumption | Application provides valid HWND to SetCooperativeLevel |
| Rationale | Required for proper window management |
| Risk if Invalid | Medium - Cannot create window without valid parent context |
| Mitigation | Create own window if invalid HWND provided |
| Status | Active |

---

## 3. System Environment Assumptions

### A-101: Windows 7 or Later

| Attribute | Value |
|-----------|-------|
| ID | A-101 |
| Category | System |
| Assumption | System runs Windows 7 SP1 or later |
| Rationale | Minimum supported OS for VS2022 runtime |
| Risk if Invalid | N/A - Explicitly out of scope |
| Mitigation | Document minimum requirements |
| Status | Active |

### A-102: DirectX 9 Runtime Available

| Attribute | Value |
|-----------|-------|
| ID | A-102 |
| Category | System |
| Assumption | DirectX 9.0c runtime is installed for D3D9 renderer |
| Rationale | Windows 7+ includes DirectX 9 |
| Risk if Invalid | Low - Fall back to GDI renderer |
| Mitigation | Automatic fallback to GDI if D3D9 unavailable |
| Status | Active |

### A-103: Graphics Driver Functional

| Attribute | Value |
|-----------|-------|
| ID | A-103 |
| Category | System |
| Assumption | System has functioning graphics driver |
| Rationale | Basic requirement for any graphics operation |
| Risk if Invalid | High - No mitigation possible |
| Mitigation | Log driver info, provide troubleshooting guidance |
| Status | Active |

### A-104: Application Directory Writable

| Attribute | Value |
|-----------|-------|
| ID | A-104 |
| Category | System |
| Assumption | Process can write to application directory for config/logs |
| Rationale | Standard location for per-application files |
| Risk if Invalid | Low - Use fallback locations |
| Mitigation | Fall back to %TEMP% or %APPDATA% if needed |
| Status | Active |

### A-105: No Administrator Required

| Attribute | Value |
|-----------|-------|
| ID | A-105 |
| Category | System |
| Assumption | Normal operation does not require administrator privileges |
| Rationale | Drop-in DLL replacement should work as normal user |
| Risk if Invalid | Medium - May limit some compatibility options |
| Mitigation | Document any operations requiring elevation |
| Status | Active |

### A-106: 64-bit Windows for x64 Builds

| Attribute | Value |
|-----------|-------|
| ID | A-106 |
| Category | System |
| Assumption | x64 builds only run on 64-bit Windows |
| Rationale | 64-bit code cannot run on 32-bit OS |
| Risk if Invalid | N/A - Technical impossibility |
| Mitigation | Provide both x86 and x64 builds |
| Status | Active |

---

## 4. Technical Assumptions

### A-201: IAT Hooking Sufficient

| Attribute | Value |
|-----------|-------|
| ID | A-201 |
| Category | Technical |
| Assumption | Import Address Table hooking is sufficient for API interception |
| Rationale | Most DirectDraw calls go through imported functions |
| Risk if Invalid | Medium - Some calls may be missed |
| Mitigation | Add inline hooking (Detours) if needed |
| Status | Active |

### A-202: Single Monitor Primary

| Attribute | Value |
|-----------|-------|
| ID | A-202 |
| Category | Technical |
| Assumption | Application targets primary monitor |
| Rationale | Legacy games typically used single monitor |
| Risk if Invalid | Low - Multi-monitor support can be added |
| Mitigation | Configuration option for monitor selection |
| Status | Active |

### A-203: VSync Available

| Attribute | Value |
|-----------|-------|
| ID | A-203 |
| Category | Technical |
| Assumption | VSync is available through graphics APIs |
| Rationale | Standard feature of D3D9 and OpenGL |
| Risk if Invalid | Low - Implement software frame timing |
| Mitigation | Frame limiter fallback |
| Status | Active |

### A-204: Standard Pixel Formats

| Attribute | Value |
|-----------|-------|
| ID | A-204 |
| Category | Technical |
| Assumption | Applications use standard pixel formats (RGB565, RGB555, XRGB8888) |
| Rationale | These were the common DirectDraw formats |
| Risk if Invalid | Medium - Non-standard formats may render incorrectly |
| Mitigation | Support additional formats as discovered |
| Status | Active |

### A-205: Cooperative Multitasking Model

| Attribute | Value |
|-----------|-------|
| ID | A-205 |
| Category | Technical |
| Assumption | Render thread and game thread cooperate, not heavily parallel |
| Rationale | Legacy games were typically single-threaded |
| Risk if Invalid | Low - Add synchronization if needed |
| Mitigation | Proper mutex/event usage in design |
| Status | Active |

---

## 5. User Assumptions

### A-301: Basic Configuration Understanding

| Attribute | Value |
|-----------|-------|
| ID | A-301 |
| Category | User |
| Assumption | Users can edit INI files if needed |
| Rationale | Standard configuration method for compatibility tools |
| Risk if Invalid | Low - Defaults work for most cases |
| Mitigation | Comprehensive default configuration, optional GUI tool |
| Status | Active |

### A-302: Drop-in Replacement Expected

| Attribute | Value |
|-----------|-------|
| ID | A-302 |
| Category | User |
| Assumption | Users expect to copy ddraw.dll to game folder and run |
| Rationale | Standard workflow for DirectDraw wrappers |
| Risk if Invalid | Low - This is the implemented approach |
| Mitigation | Clear installation documentation |
| Status | Active |

### A-303: English Primary Language

| Attribute | Value |
|-----------|-------|
| ID | A-303 |
| Category | User |
| Assumption | Documentation and logs are in English |
| Rationale | International developer/user base expects English |
| Risk if Invalid | Low - Logs are technical, minimal localization needed |
| Mitigation | Use clear, simple English |
| Status | Active |

---

## 6. Licensing Assumptions

### A-401: MIT License Compatibility

| Attribute | Value |
|-----------|-------|
| ID | A-401 |
| Category | Licensing |
| Assumption | MIT license from cnc-ddraw allows derivative works |
| Rationale | MIT is permissive, allows modification and redistribution |
| Risk if Invalid | High - Would require removing reused code |
| Mitigation | License verified as MIT; attribution maintained |
| Status | Verified |

### A-402: Windows SDK Usage Permitted

| Attribute | Value |
|-----------|-------|
| ID | A-402 |
| Category | Licensing |
| Assumption | Windows SDK headers and libraries can be used |
| Rationale | Standard development practice, Microsoft permits this use |
| Risk if Invalid | N/A - Fundamental requirement |
| Mitigation | N/A |
| Status | Active |

---

## 7. Assumption Tracking

### 7.1 Summary by Category

| Category | Total | Active | Verified | Retired |
|----------|-------|--------|----------|---------|
| Application | 7 | 7 | 0 | 0 |
| System | 6 | 6 | 0 | 0 |
| Technical | 5 | 5 | 0 | 0 |
| User | 3 | 3 | 0 | 0 |
| Licensing | 2 | 1 | 1 | 0 |
| **Total** | **23** | **22** | **1** | **0** |

### 7.2 Risk Summary

| Risk Level | Count |
|------------|-------|
| High | 2 |
| Medium | 5 |
| Low | 14 |
| N/A | 2 |

---

## 8. Review and Update

This document should be reviewed:
- At project milestones
- When assumptions are validated or invalidated
- When new assumptions are identified

### 8.1 Change Log

| Date | Version | Changes |
|------|---------|---------|
| 2026-02-05 | 1.0 | Initial document |

---

*End of Document*
