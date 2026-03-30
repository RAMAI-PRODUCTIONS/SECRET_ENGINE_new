// SecretEngine
// Module: core
// Responsibility: Pure virtual interface for engine-wide logging
// Dependencies: none

#pragma once

namespace SecretEngine {

    class ILogger {
    public:
        virtual ~ILogger() = default;

        // Structured logging with categories
        virtual void LogInfo(const char* category, const char* message) = 0;
        virtual void LogWarning(const char* category, const char* message) = 0;
        virtual void LogError(const char* category, const char* message) = 0;
    };

}