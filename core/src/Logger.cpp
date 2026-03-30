// SecretEngine
// Module: core
// Responsibility: Implementation of platform-specific logging
// Dependencies: Logger.h, <cstdio>

#include "Logger.h"
#include <cstdio>
#include <cstring>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

namespace SecretEngine {

    void Logger::LogInfo(const char* cat, const char* msg)    { InternalLog("INFO", cat, msg); }
    void Logger::LogWarning(const char* cat, const char* msg) { InternalLog("WARN", cat, msg); }
    void Logger::LogError(const char* cat, const char* msg)   { InternalLog("ERROR", cat, msg); }

    void Logger::InternalLog(const char* level, const char* category, const char* message) {
        std::lock_guard<std::mutex> lock(m_mutex);

#if defined(__ANDROID__)
        // Route to Android Logcat with correct priority
        int priority = ANDROID_LOG_INFO;
        if (strcmp(level, "ERROR") == 0) {
            priority = ANDROID_LOG_ERROR;
        } else if (strcmp(level, "WARN") == 0) {
            priority = ANDROID_LOG_WARN;
        }
        __android_log_print(priority, category, "[%s] %s", level, message);
#else
        // Route to Windows Console
        printf("[%s][%s] %s\n", level, category, message);
#endif
    }
}