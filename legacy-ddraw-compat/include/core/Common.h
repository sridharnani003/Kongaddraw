/**
 * @file Common.h
 * @brief Common definitions and includes for legacy-ddraw-compat
 *
 * This header provides common type definitions, macros, and includes
 * used throughout the project.
 */

#pragma once

// Windows headers - must be first
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

// DirectDraw headers
#include <ddraw.h>

// Standard library
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <mutex>
#include <atomic>
#include <functional>
#include <optional>

// Project namespace
namespace ldc {

// ============================================================================
// Version Information
// ============================================================================

constexpr uint32_t VERSION_MAJOR = 1;
constexpr uint32_t VERSION_MINOR = 0;
constexpr uint32_t VERSION_PATCH = 0;

constexpr const char* VERSION_STRING = "1.0.0";
constexpr const char* PROJECT_NAME = "legacy-ddraw-compat";

// ============================================================================
// Result Type
// ============================================================================

/**
 * @brief Result type for operations that can fail
 *
 * Wraps HRESULT but provides better semantics and error information.
 */
class Result {
public:
    Result() : m_hr(DD_OK) {}
    Result(HRESULT hr) : m_hr(hr) {}

    bool IsSuccess() const { return SUCCEEDED(m_hr); }
    bool IsFailure() const { return FAILED(m_hr); }
    HRESULT GetHResult() const { return m_hr; }

    operator HRESULT() const { return m_hr; }
    explicit operator bool() const { return IsSuccess(); }

    static Result Success() { return Result(DD_OK); }
    static Result InvalidParams() { return Result(DDERR_INVALIDPARAMS); }
    static Result OutOfMemory() { return Result(DDERR_OUTOFMEMORY); }
    static Result Unsupported() { return Result(DDERR_UNSUPPORTED); }
    static Result Generic() { return Result(DDERR_GENERIC); }

private:
    HRESULT m_hr;
};

// ============================================================================
// Utility Macros
// ============================================================================

// Safe release for COM objects
#define LDC_SAFE_RELEASE(p) \
    do { if (p) { (p)->Release(); (p) = nullptr; } } while(0)

// Array count
template<typename T, size_t N>
constexpr size_t ArrayCount(T(&)[N]) { return N; }

// Unused parameter
#define LDC_UNUSED(x) (void)(x)

// Debug break
#ifdef _DEBUG
#define LDC_DEBUG_BREAK() __debugbreak()
#else
#define LDC_DEBUG_BREAK() ((void)0)
#endif

// Assert
#ifdef _DEBUG
#define LDC_ASSERT(condition) \
    do { if (!(condition)) { LDC_DEBUG_BREAK(); } } while(0)
#else
#define LDC_ASSERT(condition) ((void)0)
#endif

// ============================================================================
// Non-copyable Base Class
// ============================================================================

/**
 * @brief Base class to make derived classes non-copyable
 */
class NonCopyable {
protected:
    NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

// ============================================================================
// Enumerations
// ============================================================================

/**
 * @brief Display mode types
 */
enum class DisplayModeType {
    Unknown,
    Windowed,
    BorderlessFullscreen,
    ExclusiveFullscreen
};

/**
 * @brief Surface types
 */
enum class SurfaceType {
    Unknown,
    Primary,
    BackBuffer,
    OffScreenPlain,
    Texture,
    ZBuffer
};

/**
 * @brief Surface memory location
 */
enum class SurfaceLocation {
    SystemMemory,
    VideoMemory  // Emulated
};

/**
 * @brief Renderer types
 */
enum class RendererType {
    None,
    GDI,
    OpenGL,
    Direct3D9,
    Auto
};

/**
 * @brief Log levels
 */
enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Off = 5
};

// ============================================================================
// String Conversion Helpers
// ============================================================================

/**
 * @brief Convert RendererType to string
 */
inline const char* RendererTypeToString(RendererType type) {
    switch (type) {
        case RendererType::None: return "None";
        case RendererType::GDI: return "GDI";
        case RendererType::OpenGL: return "OpenGL";
        case RendererType::Direct3D9: return "Direct3D9";
        case RendererType::Auto: return "Auto";
        default: return "Unknown";
    }
}

/**
 * @brief Convert LogLevel to string
 */
inline const char* LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO ";
        case LogLevel::Warn: return "WARN ";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Off: return "OFF  ";
        default: return "?????";
    }
}

/**
 * @brief Convert string to RendererType
 */
inline RendererType StringToRendererType(const std::string& str) {
    if (str == "gdi" || str == "GDI") return RendererType::GDI;
    if (str == "opengl" || str == "OpenGL") return RendererType::OpenGL;
    if (str == "d3d9" || str == "D3D9" || str == "direct3d9") return RendererType::Direct3D9;
    return RendererType::Auto;
}

/**
 * @brief Convert string to LogLevel
 */
inline LogLevel StringToLogLevel(const std::string& str) {
    if (str == "trace" || str == "TRACE") return LogLevel::Trace;
    if (str == "debug" || str == "DEBUG") return LogLevel::Debug;
    if (str == "info" || str == "INFO") return LogLevel::Info;
    if (str == "warn" || str == "WARN" || str == "warning") return LogLevel::Warn;
    if (str == "error" || str == "ERROR") return LogLevel::Error;
    if (str == "off" || str == "OFF" || str == "none") return LogLevel::Off;
    return LogLevel::Info;
}

} // namespace ldc
