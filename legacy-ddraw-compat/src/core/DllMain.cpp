/**
 * @file DllMain.cpp
 * @brief DLL entry point for legacy-ddraw-compat
 *
 * Handles DLL attachment/detachment, initialization of subsystems,
 * and cleanup.
 */

#include "core/Common.h"
#include "config/Config.h"
#include "logging/Logger.h"

using namespace ldc;
using namespace ldc::config;
using namespace ldc::logging;

// ============================================================================
// Global State
// ============================================================================

namespace {
    HMODULE g_hModule = nullptr;
    bool g_initialized = false;
}

// ============================================================================
// Initialization
// ============================================================================

/**
 * @brief Initialize all subsystems
 * @return true if successful
 */
static bool Initialize() {
    if (g_initialized) {
        return true;
    }

    // Get executable directory for log and config files
    std::string exeDir = ConfigManager::GetExecutableDirectory();

    // Initialize logging first (before config, so we can log config errors)
    std::string logPath = exeDir + "ddraw.log";
    if (!Logger::Instance().Initialize(logPath, LogLevel::Info)) {
        // Can't log, but continue anyway
        OutputDebugStringA("[legacy-ddraw-compat] Failed to initialize logging\n");
    }

    LOG_INFO("=== legacy-ddraw-compat v%s initializing ===", VERSION_STRING);
    LOG_INFO("Executable: %s", ConfigManager::GetExecutableName().c_str());
    LOG_INFO("Directory: %s", exeDir.c_str());

    // Load configuration
    if (ConfigManager::Instance().LoadFromExecutableDirectory()) {
        LOG_INFO("Configuration loaded from: %s",
                 ConfigManager::Instance().GetIniPath().c_str());

        // Update log level from config
        LogLevel configLevel = GetConfig().GetLogLevel();
        Logger::Instance().SetLevel(configLevel);
        LOG_DEBUG("Log level set to: %s", LogLevelToString(configLevel));
    } else {
        LOG_WARN("No configuration file found, using defaults");
    }

    // Log configuration summary
    const auto& cfg = GetConfig();
    LOG_INFO("Configuration summary:");
    LOG_INFO("  Renderer: %s", cfg.renderer.c_str());
    LOG_INFO("  VSync: %s", cfg.vsync ? "enabled" : "disabled");
    LOG_INFO("  Fullscreen: %s", cfg.fullscreen ? "enabled" : "disabled");
    LOG_INFO("  Borderless: %s", cfg.borderless ? "enabled" : "disabled");

    // Set timer resolution for better frame timing
    timeBeginPeriod(1);

    g_initialized = true;
    LOG_INFO("Initialization complete");

    return true;
}

/**
 * @brief Cleanup all subsystems
 */
static void Cleanup() {
    if (!g_initialized) {
        return;
    }

    LOG_INFO("=== legacy-ddraw-compat shutting down ===");

    // Restore timer resolution
    timeEndPeriod(1);

    // Shutdown logging last
    Logger::Instance().Shutdown();

    g_initialized = false;
}

// ============================================================================
// DLL Entry Point
// ============================================================================

/**
 * @brief DLL entry point
 *
 * Called by Windows when the DLL is loaded or unloaded.
 */
BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    LDC_UNUSED(lpReserved);

    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            g_hModule = hModule;

            // Disable thread attach/detach notifications for performance
            DisableThreadLibraryCalls(hModule);

            // Initialize subsystems
            if (!Initialize()) {
                return FALSE;
            }
            break;

        case DLL_PROCESS_DETACH:
            // Cleanup subsystems
            Cleanup();
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            // Disabled via DisableThreadLibraryCalls
            break;
    }

    return TRUE;
}

// ============================================================================
// Module Handle Access
// ============================================================================

namespace ldc {

/**
 * @brief Get the DLL module handle
 * @return HMODULE for this DLL
 */
HMODULE GetModuleHandle() {
    return g_hModule;
}

} // namespace ldc
