/**
 * @file hooks.c
 * @brief API hooks for Windows 7 compatibility on Windows 10/11
 *
 * Uses Microsoft Detours to intercept Windows APIs that behave
 * differently between Windows 7 and Windows 10/11.
 */

#include "core/Common.h"
#include <detours.h>

#pragma comment(lib, "detours.lib")

/* ============================================================================
 * Version Spoofing - Report as Windows 7
 * ============================================================================ */

#define WIN7_MAJOR  6
#define WIN7_MINOR  1
#define WIN7_BUILD  7601

/* Original function pointers */
static BOOL (WINAPI *Real_GetVersionExA)(LPOSVERSIONINFOA) = NULL;
static BOOL (WINAPI *Real_GetVersionExW)(LPOSVERSIONINFOW) = NULL;
static LONG (WINAPI *Real_RtlGetVersion)(PRTL_OSVERSIONINFOW) = NULL;

/* Hooked GetVersionExA */
static BOOL WINAPI Hook_GetVersionExA(LPOSVERSIONINFOA lpVersionInfo)
{
    if (!lpVersionInfo) return FALSE;

    BOOL result = Real_GetVersionExA(lpVersionInfo);
    if (result) {
        lpVersionInfo->dwMajorVersion = WIN7_MAJOR;
        lpVersionInfo->dwMinorVersion = WIN7_MINOR;
        lpVersionInfo->dwBuildNumber = WIN7_BUILD;
        lpVersionInfo->dwPlatformId = VER_PLATFORM_WIN32_NT;

        if (lpVersionInfo->dwOSVersionInfoSize >= sizeof(OSVERSIONINFOEXA)) {
            LPOSVERSIONINFOEXA lpEx = (LPOSVERSIONINFOEXA)lpVersionInfo;
            lpEx->wServicePackMajor = 1;
            lpEx->wServicePackMinor = 0;
            strcpy_s(lpEx->szCSDVersion, sizeof(lpEx->szCSDVersion), "Service Pack 1");
        }
    }
    return result;
}

/* Hooked GetVersionExW */
static BOOL WINAPI Hook_GetVersionExW(LPOSVERSIONINFOW lpVersionInfo)
{
    if (!lpVersionInfo) return FALSE;

    BOOL result = Real_GetVersionExW(lpVersionInfo);
    if (result) {
        lpVersionInfo->dwMajorVersion = WIN7_MAJOR;
        lpVersionInfo->dwMinorVersion = WIN7_MINOR;
        lpVersionInfo->dwBuildNumber = WIN7_BUILD;
        lpVersionInfo->dwPlatformId = VER_PLATFORM_WIN32_NT;

        if (lpVersionInfo->dwOSVersionInfoSize >= sizeof(OSVERSIONINFOEXW)) {
            LPOSVERSIONINFOEXW lpEx = (LPOSVERSIONINFOEXW)lpVersionInfo;
            lpEx->wServicePackMajor = 1;
            lpEx->wServicePackMinor = 0;
            wcscpy_s(lpEx->szCSDVersion,
                     sizeof(lpEx->szCSDVersion)/sizeof(WCHAR),
                     L"Service Pack 1");
        }
    }
    return result;
}

/* Hooked RtlGetVersion (ntdll) */
static LONG WINAPI Hook_RtlGetVersion(PRTL_OSVERSIONINFOW lpVersionInfo)
{
    if (!lpVersionInfo) return -1;

    LONG result = Real_RtlGetVersion(lpVersionInfo);
    if (result == 0) {  /* STATUS_SUCCESS */
        lpVersionInfo->dwMajorVersion = WIN7_MAJOR;
        lpVersionInfo->dwMinorVersion = WIN7_MINOR;
        lpVersionInfo->dwBuildNumber = WIN7_BUILD;
        lpVersionInfo->dwPlatformId = VER_PLATFORM_WIN32_NT;
    }
    return result;
}

/* ============================================================================
 * VerifyVersionInfo - Make version checks pass
 * ============================================================================ */

static BOOL (WINAPI *Real_VerifyVersionInfoA)(LPOSVERSIONINFOEXA, DWORD, DWORDLONG) = NULL;
static BOOL (WINAPI *Real_VerifyVersionInfoW)(LPOSVERSIONINFOEXW, DWORD, DWORDLONG) = NULL;

static BOOL WINAPI Hook_VerifyVersionInfoA(
    LPOSVERSIONINFOEXA lpVersionInfo,
    DWORD dwTypeMask,
    DWORDLONG dwlConditionMask)
{
    /* Modify the check to use Windows 7 */
    OSVERSIONINFOEXA vi = *lpVersionInfo;

    if (dwTypeMask & VER_MAJORVERSION) {
        if (vi.dwMajorVersion > WIN7_MAJOR) {
            vi.dwMajorVersion = WIN7_MAJOR;
        }
    }
    if (dwTypeMask & VER_MINORVERSION) {
        if (vi.dwMinorVersion > WIN7_MINOR) {
            vi.dwMinorVersion = WIN7_MINOR;
        }
    }

    return Real_VerifyVersionInfoA(&vi, dwTypeMask, dwlConditionMask);
}

static BOOL WINAPI Hook_VerifyVersionInfoW(
    LPOSVERSIONINFOEXW lpVersionInfo,
    DWORD dwTypeMask,
    DWORDLONG dwlConditionMask)
{
    OSVERSIONINFOEXW vi = *lpVersionInfo;

    if (dwTypeMask & VER_MAJORVERSION) {
        if (vi.dwMajorVersion > WIN7_MAJOR) {
            vi.dwMajorVersion = WIN7_MAJOR;
        }
    }
    if (dwTypeMask & VER_MINORVERSION) {
        if (vi.dwMinorVersion > WIN7_MINOR) {
            vi.dwMinorVersion = WIN7_MINOR;
        }
    }

    return Real_VerifyVersionInfoW(&vi, dwTypeMask, dwlConditionMask);
}

/* ============================================================================
 * DPI Awareness - Disable for legacy apps
 * ============================================================================ */

static BOOL (WINAPI *Real_SetProcessDPIAware)(void) = NULL;
static BOOL (WINAPI *Real_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT) = NULL;

static BOOL WINAPI Hook_SetProcessDPIAware(void)
{
    LDC_LOG("SetProcessDPIAware blocked");
    return TRUE;  /* Pretend success but don't actually set */
}

static BOOL WINAPI Hook_SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT value)
{
    LDC_UNUSED(value);
    LDC_LOG("SetProcessDpiAwarenessContext blocked");
    return TRUE;
}

/* ============================================================================
 * Hook Installation
 * ============================================================================ */

static BOOL s_hooksInstalled = FALSE;

BOOL LDC_InstallHooks(void)
{
    if (s_hooksInstalled) return TRUE;

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    HMODULE hUser32 = GetModuleHandleA("user32.dll");

    if (!hKernel32 || !hNtdll) {
        LDC_LOG("Failed to get module handles");
        return FALSE;
    }

    /* Get original function addresses */
    Real_GetVersionExA = (void*)GetProcAddress(hKernel32, "GetVersionExA");
    Real_GetVersionExW = (void*)GetProcAddress(hKernel32, "GetVersionExW");
    Real_RtlGetVersion = (void*)GetProcAddress(hNtdll, "RtlGetVersion");
    Real_VerifyVersionInfoA = (void*)GetProcAddress(hKernel32, "VerifyVersionInfoA");
    Real_VerifyVersionInfoW = (void*)GetProcAddress(hKernel32, "VerifyVersionInfoW");

    if (hUser32) {
        Real_SetProcessDPIAware = (void*)GetProcAddress(hUser32, "SetProcessDPIAware");
        Real_SetProcessDpiAwarenessContext = (void*)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
    }

    /* Install hooks using Detours */
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (Real_GetVersionExA)
        DetourAttach((PVOID*)&Real_GetVersionExA, Hook_GetVersionExA);
    if (Real_GetVersionExW)
        DetourAttach((PVOID*)&Real_GetVersionExW, Hook_GetVersionExW);
    if (Real_RtlGetVersion)
        DetourAttach((PVOID*)&Real_RtlGetVersion, Hook_RtlGetVersion);
    if (Real_VerifyVersionInfoA)
        DetourAttach((PVOID*)&Real_VerifyVersionInfoA, Hook_VerifyVersionInfoA);
    if (Real_VerifyVersionInfoW)
        DetourAttach((PVOID*)&Real_VerifyVersionInfoW, Hook_VerifyVersionInfoW);
    if (Real_SetProcessDPIAware)
        DetourAttach((PVOID*)&Real_SetProcessDPIAware, Hook_SetProcessDPIAware);
    if (Real_SetProcessDpiAwarenessContext)
        DetourAttach((PVOID*)&Real_SetProcessDpiAwarenessContext, Hook_SetProcessDpiAwarenessContext);

    LONG error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        LDC_LOG("DetourTransactionCommit failed: %ld", error);
        return FALSE;
    }

    s_hooksInstalled = TRUE;
    LDC_LOG("Hooks installed successfully");
    return TRUE;
}

void LDC_RemoveHooks(void)
{
    if (!s_hooksInstalled) return;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (Real_GetVersionExA)
        DetourDetach((PVOID*)&Real_GetVersionExA, Hook_GetVersionExA);
    if (Real_GetVersionExW)
        DetourDetach((PVOID*)&Real_GetVersionExW, Hook_GetVersionExW);
    if (Real_RtlGetVersion)
        DetourDetach((PVOID*)&Real_RtlGetVersion, Hook_RtlGetVersion);
    if (Real_VerifyVersionInfoA)
        DetourDetach((PVOID*)&Real_VerifyVersionInfoA, Hook_VerifyVersionInfoA);
    if (Real_VerifyVersionInfoW)
        DetourDetach((PVOID*)&Real_VerifyVersionInfoW, Hook_VerifyVersionInfoW);
    if (Real_SetProcessDPIAware)
        DetourDetach((PVOID*)&Real_SetProcessDPIAware, Hook_SetProcessDPIAware);
    if (Real_SetProcessDpiAwarenessContext)
        DetourDetach((PVOID*)&Real_SetProcessDpiAwarenessContext, Hook_SetProcessDpiAwarenessContext);

    DetourTransactionCommit();
    s_hooksInstalled = FALSE;

    LDC_LOG("Hooks removed");
}
