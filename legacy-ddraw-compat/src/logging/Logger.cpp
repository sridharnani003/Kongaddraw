/**
 * @file Logger.cpp
 * @brief Logging system implementation
 */

#include "logging/Logger.h"
#include <cstdarg>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <filesystem>

using namespace ldc;
using namespace ldc::logging;

// ============================================================================
// Logger Implementation
// ============================================================================

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    Shutdown();
}

bool Logger::Initialize(const std::string& logPath, LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        return true;
    }

    m_basePath = logPath;
    m_level = level;

    // Open log file
    m_file.open(logPath, std::ios::out | std::ios::trunc);
    if (!m_file.is_open()) {
        return false;
    }

    m_currentSize = 0;
    m_initialized = true;

    return true;
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return;
    }

    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
    }

    m_initialized = false;
}

void Logger::Log(LogLevel level, const char* file, int line, const char* fmt, ...) {
    if (level < m_level || !m_initialized) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    LogV(level, file, line, fmt, args);
    va_end(args);
}

void Logger::LogV(LogLevel level, const char* file, int line, const char* fmt, va_list args) {
    if (level < m_level || !m_initialized) {
        return;
    }

    // Format the message
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    buffer[sizeof(buffer) - 1] = '\0';

    // Create log entry
    std::string entry = FormatEntry(level, file, line, buffer);

    // Write to file
    WriteEntry(entry);
}

void Logger::LogMessage(LogLevel level, const std::string& message) {
    if (level < m_level || !m_initialized) {
        return;
    }

    std::string entry = FormatEntry(level, nullptr, 0, message);
    WriteEntry(entry);
}

void Logger::Flush() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_file.is_open()) {
        m_file.flush();
    }
}

std::string Logger::FormatEntry(LogLevel level, const char* file, int line, const std::string& message) {
    std::ostringstream oss;

    // Timestamp
    oss << "[" << FormatTimestamp() << "] ";

    // Level
    oss << "[" << LogLevelToString(level) << "] ";

    // Thread ID
    oss << "[" << std::setw(5) << GetCurrentThreadId() << "] ";

    // Message
    oss << message;

    // Source location (debug builds only)
#ifdef _DEBUG
    if (file && line > 0) {
        oss << " (" << GetBaseName(file) << ":" << line << ")";
    }
#else
    LDC_UNUSED(file);
    LDC_UNUSED(line);
#endif

    oss << "\n";

    return oss.str();
}

void Logger::WriteEntry(const std::string& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_file.is_open()) {
        return;
    }

    // Check if rotation needed
    RotateIfNeeded();

    // Write entry
    m_file << entry;
    m_file.flush();

    m_currentSize += entry.size();

    // Also output to debug console in debug builds
#ifdef _DEBUG
    OutputDebugStringA(entry.c_str());
#endif
}

void Logger::RotateIfNeeded() {
    if (m_currentSize < m_maxFileSize) {
        return;
    }

    // Close current file
    m_file.close();

    // Rotate files
    for (int i = m_maxFiles - 1; i >= 1; --i) {
        std::string oldPath = m_basePath + "." + std::to_string(i);
        std::string newPath = m_basePath + "." + std::to_string(i + 1);

        // Delete oldest if exists
        if (i == m_maxFiles - 1) {
            std::remove(newPath.c_str());
        }

        // Rename file
        std::rename(oldPath.c_str(), newPath.c_str());
    }

    // Rename current to .1
    std::string rotatedPath = m_basePath + ".1";
    std::rename(m_basePath.c_str(), rotatedPath.c_str());

    // Open new log file
    m_file.open(m_basePath, std::ios::out | std::ios::trunc);
    m_currentSize = 0;
}

std::string Logger::FormatTimestamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    return std::string(buffer);
}

std::string Logger::GetBaseName(const char* path) {
    if (!path) {
        return "";
    }

    const char* lastSlash = strrchr(path, '\\');
    if (!lastSlash) {
        lastSlash = strrchr(path, '/');
    }

    if (lastSlash) {
        return std::string(lastSlash + 1);
    }

    return std::string(path);
}

// ============================================================================
// ScopedTimer Implementation
// ============================================================================

ScopedTimer::ScopedTimer(const char* name, LogLevel level)
    : m_name(name)
    , m_level(level)
{
    QueryPerformanceFrequency(&m_frequency);
    QueryPerformanceCounter(&m_start);
}

ScopedTimer::~ScopedTimer() {
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);

    double elapsed = static_cast<double>(end.QuadPart - m_start.QuadPart) /
                     static_cast<double>(m_frequency.QuadPart) * 1000.0;

    Logger::Instance().Log(m_level, nullptr, 0, "%s took %.3f ms", m_name, elapsed);
}
