/**
 * @file Logger.h
 * @brief Logging system for legacy-ddraw-compat
 *
 * Provides thread-safe logging with multiple levels, timestamps,
 * file output, and log rotation.
 */

#pragma once

#include "core/Common.h"
#include <fstream>
#include <mutex>
#include <sstream>

namespace ldc::logging {

// ============================================================================
// Logger Class
// ============================================================================

/**
 * @brief Singleton logger with file output and rotation
 *
 * Features:
 * - Multiple log levels (Trace, Debug, Info, Warn, Error)
 * - Thread-safe logging
 * - Timestamps with millisecond precision
 * - Thread ID in log entries
 * - File rotation at configurable size
 * - Configurable maximum number of rotated files
 */
class Logger : public NonCopyable {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the Logger
     */
    static Logger& Instance();

    /**
     * @brief Initialize the logging system
     * @param logPath Path to the log file
     * @param level Minimum log level to record
     * @return true if successful
     */
    bool Initialize(const std::string& logPath, LogLevel level = LogLevel::Info);

    /**
     * @brief Shutdown the logging system
     *
     * Flushes any pending output and closes the log file.
     */
    void Shutdown();

    /**
     * @brief Check if logger is initialized
     * @return true if initialized
     */
    bool IsInitialized() const { return m_initialized; }

    /**
     * @brief Set the minimum log level
     * @param level Minimum level to record
     */
    void SetLevel(LogLevel level) { m_level = level; }

    /**
     * @brief Get the current log level
     * @return Current minimum log level
     */
    LogLevel GetLevel() const { return m_level; }

    /**
     * @brief Set maximum log file size before rotation
     * @param bytes Maximum size in bytes (default: 50MB)
     */
    void SetMaxFileSize(size_t bytes) { m_maxFileSize = bytes; }

    /**
     * @brief Set maximum number of rotated log files
     * @param count Maximum file count (default: 3)
     */
    void SetMaxFiles(int count) { m_maxFiles = count; }

    /**
     * @brief Log a message
     * @param level Log level
     * @param file Source file name
     * @param line Source line number
     * @param fmt Format string (printf-style)
     * @param ... Format arguments
     */
    void Log(LogLevel level, const char* file, int line, const char* fmt, ...);

    /**
     * @brief Log a message with va_list
     * @param level Log level
     * @param file Source file name
     * @param line Source line number
     * @param fmt Format string
     * @param args Format arguments
     */
    void LogV(LogLevel level, const char* file, int line, const char* fmt, va_list args);

    /**
     * @brief Log a pre-formatted message
     * @param level Log level
     * @param message Pre-formatted message
     */
    void LogMessage(LogLevel level, const std::string& message);

    /**
     * @brief Flush log output
     */
    void Flush();

private:
    Logger() = default;
    ~Logger();

    std::ofstream m_file;
    std::string m_basePath;
    LogLevel m_level = LogLevel::Info;
    size_t m_maxFileSize = 50 * 1024 * 1024;  // 50 MB
    int m_maxFiles = 3;
    size_t m_currentSize = 0;
    bool m_initialized = false;
    mutable std::mutex m_mutex;

    void RotateIfNeeded();
    void WriteEntry(const std::string& entry);
    std::string FormatEntry(LogLevel level, const char* file, int line, const std::string& message);

    static std::string FormatTimestamp();
    static std::string GetBaseName(const char* path);
};

// ============================================================================
// Logging Macros
// ============================================================================

/**
 * @brief Log a trace-level message
 *
 * Use for very detailed debugging information that would normally
 * be too verbose for regular debug builds.
 */
#define LOG_TRACE(fmt, ...) \
    ldc::logging::Logger::Instance().Log( \
        ldc::LogLevel::Trace, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief Log a debug-level message
 *
 * Use for debugging information useful during development.
 */
#define LOG_DEBUG(fmt, ...) \
    ldc::logging::Logger::Instance().Log( \
        ldc::LogLevel::Debug, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief Log an info-level message
 *
 * Use for normal operational information.
 */
#define LOG_INFO(fmt, ...) \
    ldc::logging::Logger::Instance().Log( \
        ldc::LogLevel::Info, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief Log a warning-level message
 *
 * Use for warnings about potential issues.
 */
#define LOG_WARN(fmt, ...) \
    ldc::logging::Logger::Instance().Log( \
        ldc::LogLevel::Warn, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief Log an error-level message
 *
 * Use for errors that may cause malfunction.
 */
#define LOG_ERROR(fmt, ...) \
    ldc::logging::Logger::Instance().Log( \
        ldc::LogLevel::Error, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief Log HRESULT with description
 */
#define LOG_HRESULT(hr, context) \
    LOG_ERROR("%s: HRESULT 0x%08X", context, static_cast<unsigned int>(hr))

// ============================================================================
// Scoped Log Timer
// ============================================================================

/**
 * @brief RAII class for timing operations
 *
 * Logs the duration of a scope when it ends.
 */
class ScopedTimer {
public:
    ScopedTimer(const char* name, LogLevel level = LogLevel::Debug);
    ~ScopedTimer();

private:
    const char* m_name;
    LogLevel m_level;
    LARGE_INTEGER m_start;
    LARGE_INTEGER m_frequency;
};

/**
 * @brief Create a scoped timer for the current function
 */
#define LOG_SCOPED_TIMER() \
    ldc::logging::ScopedTimer _scopedTimer(__FUNCTION__)

} // namespace ldc::logging
