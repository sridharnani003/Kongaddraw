/**
 * @file ConfigManager.cpp
 * @brief Configuration loading and management implementation
 */

#include "config/Config.h"
#include "logging/Logger.h"
#include <fstream>
#include <algorithm>
#include <cctype>

using namespace ldc;
using namespace ldc::config;
using namespace ldc::logging;

// ============================================================================
// IniParser Implementation
// ============================================================================

bool IniParser::Open(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    m_data.clear();
    std::string currentSection = "";
    std::string line;

    while (std::getline(file, line)) {
        if (!ParseLine(line, currentSection)) {
            // Parse error, but continue
        }
    }

    return true;
}

void IniParser::Close() {
    m_data.clear();
}

bool IniParser::ParseLine(const std::string& line, std::string& currentSection) {
    std::string trimmed = Trim(line);

    // Empty line or comment
    if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') {
        return true;
    }

    // Section header
    if (trimmed[0] == '[') {
        size_t end = trimmed.find(']');
        if (end != std::string::npos) {
            currentSection = ToLower(trimmed.substr(1, end - 1));
            return true;
        }
        return false;  // Malformed section
    }

    // Key=value pair
    size_t eq = trimmed.find('=');
    if (eq != std::string::npos) {
        std::string key = ToLower(Trim(trimmed.substr(0, eq)));
        std::string value = Trim(trimmed.substr(eq + 1));

        // Remove quotes if present
        if (value.size() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }

        m_data[currentSection][key] = value;
        return true;
    }

    return false;  // Malformed line
}

std::string IniParser::GetString(
    const std::string& section,
    const std::string& key,
    const std::string& defaultValue
) const {
    std::string lowerSection = ToLower(section);
    std::string lowerKey = ToLower(key);

    auto sectionIt = m_data.find(lowerSection);
    if (sectionIt == m_data.end()) {
        return defaultValue;
    }

    auto keyIt = sectionIt->second.find(lowerKey);
    if (keyIt == sectionIt->second.end()) {
        return defaultValue;
    }

    return keyIt->second;
}

int IniParser::GetInt(
    const std::string& section,
    const std::string& key,
    int defaultValue
) const {
    std::string value = GetString(section, key, "");
    if (value.empty()) {
        return defaultValue;
    }

    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

bool IniParser::GetBool(
    const std::string& section,
    const std::string& key,
    bool defaultValue
) const {
    std::string value = ToLower(GetString(section, key, ""));
    if (value.empty()) {
        return defaultValue;
    }

    // True values
    if (value == "true" || value == "yes" || value == "1" || value == "on") {
        return true;
    }

    // False values
    if (value == "false" || value == "no" || value == "0" || value == "off") {
        return false;
    }

    return defaultValue;
}

bool IniParser::HasSection(const std::string& section) const {
    return m_data.find(ToLower(section)) != m_data.end();
}

bool IniParser::HasKey(const std::string& section, const std::string& key) const {
    auto sectionIt = m_data.find(ToLower(section));
    if (sectionIt == m_data.end()) {
        return false;
    }
    return sectionIt->second.find(ToLower(key)) != sectionIt->second.end();
}

std::string IniParser::Trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }

    return str.substr(start, end - start);
}

std::string IniParser::ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

// ============================================================================
// ConfigManager Implementation
// ============================================================================

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::Load(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);

    IniParser parser;
    if (!parser.Open(path)) {
        return false;
    }

    m_iniPath = path;

    // Load from [ddraw] section
    LoadSection(parser, "ddraw");

    // Apply game-specific overrides
    std::string exeName = GetExecutableName();
    ApplyGameSpecificOverrides(parser, exeName);

    // Validate and clamp values
    Validate();

    m_loaded = true;
    return true;
}

bool ConfigManager::LoadFromExecutableDirectory() {
    std::string path = GetExecutableDirectory() + "ddraw.ini";
    return Load(path);
}

void ConfigManager::LoadSection(const IniParser& parser, const std::string& section) {
    // Display settings
    m_config.width = parser.GetInt(section, "width", m_config.width);
    m_config.height = parser.GetInt(section, "height", m_config.height);
    m_config.fullscreen = parser.GetBool(section, "fullscreen", m_config.fullscreen);
    m_config.borderless = parser.GetBool(section, "borderless", m_config.borderless);
    m_config.maintainAspectRatio = parser.GetBool(section, "maintainaspectratio", m_config.maintainAspectRatio);
    m_config.resizable = parser.GetBool(section, "resizable", m_config.resizable);

    // Rendering settings
    m_config.renderer = parser.GetString(section, "renderer", m_config.renderer);
    m_config.vsync = parser.GetBool(section, "vsync", m_config.vsync);
    m_config.maxFps = parser.GetInt(section, "maxfps", m_config.maxFps);
    m_config.shader = parser.GetString(section, "shader", m_config.shader);

    // Compatibility settings
    m_config.maxGameTicks = parser.GetInt(section, "maxgameticks", m_config.maxGameTicks);
    m_config.singleCpu = parser.GetBool(section, "singlecpu", m_config.singleCpu);
    m_config.hookChildWindows = parser.GetBool(section, "hookchildwindows", m_config.hookChildWindows);

    // Input settings
    m_config.adjustMouse = parser.GetBool(section, "adjustmouse", m_config.adjustMouse);
    m_config.lockCursor = parser.GetBool(section, "lockcursor", m_config.lockCursor);

    // Debug settings
    m_config.logLevel = parser.GetString(section, "loglevel", m_config.logLevel);
    m_config.crashDumps = parser.GetBool(section, "crashdumps", m_config.crashDumps);
    m_config.showFps = parser.GetBool(section, "showfps", m_config.showFps);

    // Hotkeys
    m_config.hotkeyFullscreen = parser.GetInt(section, "hotkey_fullscreen", m_config.hotkeyFullscreen);
    m_config.hotkeyScreenshot = parser.GetInt(section, "hotkey_screenshot", m_config.hotkeyScreenshot);
}

void ConfigManager::ApplyGameSpecificOverrides(const IniParser& parser, const std::string& exeName) {
    // Check for game-specific section
    if (parser.HasSection(exeName)) {
        LOG_DEBUG("Applying game-specific settings for: %s", exeName.c_str());
        LoadSection(parser, exeName);
    }
}

void ConfigManager::Validate() {
    // Clamp width/height
    if (m_config.width > 8192) m_config.width = 8192;
    if (m_config.height > 8192) m_config.height = 8192;

    // Clamp FPS
    if (m_config.maxFps > 1000) m_config.maxFps = 1000;
    if (m_config.maxFps < -1) m_config.maxFps = -1;

    // Clamp game ticks
    if (m_config.maxGameTicks > 1000) m_config.maxGameTicks = 1000;
    if (m_config.maxGameTicks < 0) m_config.maxGameTicks = 0;

    // Validate renderer string
    std::string renderer = m_config.renderer;
    std::transform(renderer.begin(), renderer.end(), renderer.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (renderer != "auto" && renderer != "gdi" &&
        renderer != "opengl" && renderer != "d3d9" && renderer != "direct3d9") {
        LOG_WARN("Invalid renderer '%s', using 'auto'", m_config.renderer.c_str());
        m_config.renderer = "auto";
    }
}

std::string ConfigManager::GetExecutableName() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);

    // Find last backslash
    const char* name = strrchr(path, '\\');
    if (name) {
        return std::string(name + 1);
    }
    return std::string(path);
}

std::string ConfigManager::GetExecutableDirectory() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);

    // Find last backslash and truncate
    char* lastSlash = strrchr(path, '\\');
    if (lastSlash) {
        lastSlash[1] = '\0';
    }

    return std::string(path);
}
