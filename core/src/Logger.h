// SecretEngine
// Module: core
// Responsibility: Platform-aware console logger
// Dependencies: SecretEngine/ILogger.h, <mutex>

#pragma once
#include <SecretEngine/ILogger.h>
#include <mutex>

namespace SecretEngine {

    class Logger : public ILogger {
    public:
        void LogInfo(const char* category, const char* message) override;
        void LogWarning(const char* category, const char* message) override;
        void LogError(const char* category, const char* message) override;

    private:
        void InternalLog(const char* level, const char* category, const char* message);
        std::mutex m_mutex;
    };

}