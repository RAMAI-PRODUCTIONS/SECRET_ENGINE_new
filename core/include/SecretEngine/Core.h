// SecretEngine
// Module: core
// Responsibility: Master include - version, platform detection, API exports

#pragma once

#define SE_VERSION_MAJOR 0
#define SE_VERSION_MINOR 1
#define SE_VERSION_PATCH 0

// FIX: Wrap in #ifndef to prevent redefinition from CMake
#ifndef SE_PLATFORM_WINDOWS
    #if defined(_WIN32)
        #define SE_PLATFORM_WINDOWS
    #endif
#endif

#ifndef SE_PLATFORM_ANDROID
    #if defined(__ANDROID__)
        #define SE_PLATFORM_ANDROID
    #endif
#endif

#if defined(SE_PLATFORM_WINDOWS)
    #define SE_API __declspec(dllexport)
#else
    #define SE_API __attribute__((visibility("default")))
#endif

namespace SecretEngine {
    class IPlugin;
    class ICore;
    class IAllocator;
    class ILogger;
    class IRenderer;
    class IInputSystem;
    class IWorld;
    struct Entity;

    SE_API ICore* GetEngineCore();
}