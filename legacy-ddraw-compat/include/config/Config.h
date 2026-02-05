/**
 * @file Config.h
 * @brief Configuration structures and manager for legacy-ddraw-compat
 *
 * Provides INI-based configuration with per-application overrides,
 * validation, and defaults.
 */

#pragma once

#include "core/Common.h"
#include <string>
#include <mutex>

namespace ldc::config {

// ============================================================================
// Configuration Structure
// ============================================================================

/**
 * @brief Main configuration structure
 *
 * Contains all configurable settings for the compatibility layer.
 * Settings are loaded from ddraw.ini in the application directory.
 */
struct Config {
    // ========================================================================
    // Display Settings
    // ========================================================================

    /** Window width (0 = use game's requested width) */
    uint32_t width = 0;

    /** Window height (0 = use game's requested height) */
    uint32_t height = 0;

    /** Start in fullscreen mode */
    bool fullscreen = false;

    /** Use borderless windowed mode instead of true fullscreen */
    bool borderless = true;

    /** Maintain aspect ratio when scaling */
    bool maintainAspectRatio = true;

    /** Allow window resizing */
    bool resizable = false;

    // ========================================================================
    // Rendering Settings
    // ========================================================================

    /** Renderer selection: auto, d3d9, opengl, gdi */
    std::string renderer = "auto";

    /** Enable vertical synchronization */
    bool vsync = true;

    /** Maximum frames per second (0 = unlimited, -1 = auto) */
    int maxFps = 0;

    /** Shader file path (empty = no shader) */
    std::string shader;

    // ========================================================================
    // Compatibility Settings
    // ========================================================================

    /** Maximum game ticks per second (0 = unlimited) */
    int maxGameTicks = 0;

    /** Force single CPU affinity */
    bool singleCpu = false;

    /** Hook child windows */
    bool hookChildWindows = false;

    /** Disable alt+tab while fullscreen */
    bool lockAltTab = false;

    // ========================================================================
    // Input Settings
    // ========================================================================

    /** Adjust mouse coordinates for scaled window */
    bool adjustMouse = true;

    /** Lock cursor to game window */
    bool lockCursor = false;

    /** Fix mouse cursor in windowed mode */
    bool fixMouseCursor = true;

    // ========================================================================
    // Debug Settings
    // ========================================================================

    /** Log level: trace, debug, info, warn, error, off */
    std::string logLevel = "info";

    /** Generate crash dump files on exception */
    bool crashDumps = true;

    /** Show FPS counter */
    bool showFps = false;

    // ========================================================================
    // Hotkey Settings (virtual key codes, 0 = disabled)
    // ========================================================================

    /** Fullscreen toggle hotkey (default: Alt+Enter = 0) */
    uint32_t hotkeyFullscreen = 0;

    /** Screenshot hotkey (default: PrintScreen = 0) */
    uint32_t hotkeyScreenshot = 0;

    /** Unlock cursor hotkey (default: Ctrl+Tab = 0) */
    uint32_t hotkeyUnlockCursor = 0;

    // ========================================================================
    // Methods
    // ========================================================================

    /** Get renderer type enum from string setting */
    RendererType GetRendererType() const {
        return StringToRendererType(renderer);
    }

    /** Get log level enum from string setting */
    LogLevel GetLogLevel() const {
        return StringToLogLevel(logLevel);
    }
};

// ============================================================================
// INI Parser Class
// ============================================================================

/**
 * @brief Simple INI file parser
 *
 * Parses Windows INI format configuration files with support for
 * sections, key-value pairs, and comments.
 */
class IniParser {
public:
    IniParser() = default;
    ~IniParser() = default;

    /**
     * @brief Open and parse an INI file
     * @param path Path to the INI file
     * @return true if successful, false otherwise
     */
    bool Open(const std::string& path);

    /**
     * @brief Close the parser and release resources
     */
    void Close();

    /**
     * @brief Get a string value from the INI file
     * @param section Section name
     * @param key Key name
     * @param defaultValue Value to return if key not found
     * @return The value or defaultValue if not found
     */
    std::string GetString(
        const std::string& section,
        const std::string& key,
        const std::string& defaultValue = ""
    ) const;

    /**
     * @brief Get an integer value from the INI file
     * @param section Section name
     * @param key Key name
     * @param defaultValue Value to return if key not found
     * @return The value or defaultValue if not found
     */
    int GetInt(
        const std::string& section,
        const std::string& key,
        int defaultValue = 0
    ) const;

    /**
     * @brief Get a boolean value from the INI file
     * @param section Section name
     * @param key Key name
     * @param defaultValue Value to return if key not found
     * @return The value or defaultValue if not found
     *
     * Recognizes: true/false, yes/no, 1/0, on/off
     */
    bool GetBool(
        const std::string& section,
        const std::string& key,
        bool defaultValue = false
    ) const;

    /**
     * @brief Check if a section exists
     * @param section Section name
     * @return true if section exists
     */
    bool HasSection(const std::string& section) const;

    /**
     * @brief Check if a key exists in a section
     * @param section Section name
     * @param key Key name
     * @return true if key exists in section
     */
    bool HasKey(const std::string& section, const std::string& key) const;

private:
    // Section -> (Key -> Value)
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_data;

    bool ParseLine(const std::string& line, std::string& currentSection);
    static std::string Trim(const std::string& str);
    static std::string ToLower(const std::string& str);
};

// ============================================================================
// Configuration Manager
// ============================================================================

/**
 * @brief Singleton configuration manager
 *
 * Manages loading, validation, and access to configuration settings.
 * Supports per-application overrides based on executable name.
 */
class ConfigManager : public NonCopyable {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the ConfigManager
     */
    static ConfigManager& Instance();

    /**
     * @brief Load configuration from file
     * @param path Path to the INI file
     * @return true if loaded successfully
     */
    bool Load(const std::string& path);

    /**
     * @brief Load configuration from default location
     *
     * Looks for ddraw.ini in the executable's directory.
     * @return true if loaded successfully
     */
    bool LoadFromExecutableDirectory();

    /**
     * @brief Get the current configuration
     * @return Const reference to the Config structure
     */
    const Config& Get() const { return m_config; }

    /**
     * @brief Get the loaded INI file path
     * @return Path to the INI file, or empty if not loaded
     */
    const std::string& GetIniPath() const { return m_iniPath; }

    /**
     * @brief Check if configuration was loaded
     * @return true if configuration was loaded from file
     */
    bool IsLoaded() const { return m_loaded; }

    /**
     * @brief Get current executable name
     * @return Executable filename without path
     */
    static std::string GetExecutableName();

    /**
     * @brief Get current executable directory
     * @return Directory path with trailing backslash
     */
    static std::string GetExecutableDirectory();

private:
    ConfigManager() = default;

    Config m_config;
    std::string m_iniPath;
    bool m_loaded = false;
    mutable std::mutex m_mutex;

    void LoadSection(const IniParser& parser, const std::string& section);
    void ApplyGameSpecificOverrides(const IniParser& parser, const std::string& exeName);
    void Validate();
};

// ============================================================================
// Convenience Function
// ============================================================================

/**
 * @brief Get the current configuration
 * @return Const reference to the Config structure
 *
 * Shorthand for ConfigManager::Instance().Get()
 */
inline const Config& GetConfig() {
    return ConfigManager::Instance().Get();
}

} // namespace ldc::config
