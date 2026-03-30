SecretEngine – Build Structure (FROZEN)

Purpose

This document defines how SecretEngine is built, organized, and compiled.
Build system rules are absolute.

## 1. Build Philosophy

Builds must be:
- Fast (incremental < 10 seconds)
- Predictable (same input → same output)
- Minimal (no unnecessary dependencies)
- Android-first (mobile constraints drive design)

## 2. Build Tool: CMake

**Version**: CMake 3.20+
**Why**: Cross-platform, Android NDK support, industry standard

No other build systems. No custom scripts.

## 3. Top-Level Structure

```
SecretEngine/
├── CMakeLists.txt              # Root build file
├── core/
│   ├── CMakeLists.txt          # Core library
│   ├── include/
│   │   └── SecretEngine/       # Public headers
│   └── src/                    # Implementation
├── plugins/
│   ├── CMakeLists.txt          # Plugin registry
│   ├── VulkanRenderer/
│   │   ├── CMakeLists.txt
│   │   └── src/
│   ├── AndroidInput/
│   │   ├── CMakeLists.txt
│   │   └── src/
│   └── ...
├── tools/
│   ├── CMakeLists.txt
│   ├── AssetCooker/
│   │   ├── CMakeLists.txt
│   │   └── src/
│   └── ...
├── Assets/                      # Authoring assets (not built)
├── tests/
│   ├── CMakeLists.txt
│   └── ...
└── build/                       # Generated (not in VCS)
```

## 4. Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(SecretEngine VERSION 0.1.0 LANGUAGES CXX)

# Global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection
if(ANDROID)
    set(PLATFORM_ANDROID ON)
elseif(WIN32)
    set(PLATFORM_WINDOWS ON)
endif()

# Build configuration
option(SE_BUILD_TESTS "Build tests" ON)
option(SE_BUILD_TOOLS "Build tools" ON)
option(SE_ENABLE_VALIDATION "Enable validation layers" OFF)

# Subdirectories
add_subdirectory(core)
add_subdirectory(plugins)

if(SE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(SE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

## 5. Core Library Build

**File**: `core/CMakeLists.txt`

```cmake
# SecretEngine Core Library
add_library(SecretEngine_Core STATIC
    src/Core.cpp
    src/PluginManager.cpp
    src/Allocator.cpp
    src/Logger.cpp
    src/Entity.cpp
    src/World.cpp
    # ... more files
)

target_include_directories(SecretEngine_Core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(SecretEngine_Core PUBLIC cxx_std_20)

# Platform-specific
if(PLATFORM_ANDROID)
    target_link_libraries(SecretEngine_Core PRIVATE log android)
endif()
```

### Core Rules
- Core is STATIC library
- No external dependencies (except STL)
- Header-only interfaces
- Single public include: `#include <SecretEngine/Core.h>`

## 6. Plugin Build Template

**File**: `plugins/VulkanRenderer/CMakeLists.txt`

```cmake
# VulkanRenderer Plugin
add_library(VulkanRenderer SHARED
    src/RendererPlugin.cpp
    src/VulkanDevice.cpp
    src/Swapchain.cpp
    # ... more files
)

target_link_libraries(VulkanRenderer
    PRIVATE
        SecretEngine_Core  # Link to core
        Vulkan::Vulkan     # External dependency
)

target_include_directories(VulkanRenderer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Plugin metadata
set_target_properties(VulkanRenderer PROPERTIES
    VERSION 1.0.0
    OUTPUT_NAME "VulkanRenderer"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)

# Copy manifest
configure_file(
    plugin_manifest.json
    ${CMAKE_BINARY_DIR}/plugins/VulkanRenderer/plugin_manifest.json
    COPYONLY
)
```

### Plugin Rules
- Plugins are SHARED libraries (.dll, .so)
- Plugins link to Core (privately)
- Plugins export only `CreatePlugin()` and `DestroyPlugin()`
- Plugins output to `build/plugins/`

## 7. Tool Build Template

**File**: `tools/AssetCooker/CMakeLists.txt`

```cmake
# Asset Cooker Tool
add_executable(AssetCooker
    src/main.cpp
    src/Cooker.cpp
    src/MeshProcessor.cpp
    src/TextureProcessor.cpp
)

target_link_libraries(AssetCooker
    PRIVATE
        SecretEngine_Core
        nlohmann_json::nlohmann_json  # JSON parsing
        stb::stb                      # Image loading
)

# Install to tools/
install(TARGETS AssetCooker
    RUNTIME DESTINATION tools
)
```

### Tool Rules
- Tools are executables
- Tools link to Core
- Tools may have external dependencies
- Tools never ship with game

## 8. Dependency Management

### External Dependencies

```cmake
# Root CMakeLists.txt

include(FetchContent)

# Vulkan (system)
find_package(Vulkan REQUIRED)

# JSON (fetch)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)
```

### Allowed Dependencies (Core)
- **None** (only STL)

### Allowed Dependencies (Plugins)
- Core
- Platform SDKs (Vulkan, Android, Windows)
- Single-header libraries (STB, etc.)

### Allowed Dependencies (Tools)
- Core
- JSON parsers
- Image loaders
- Mesh importers (Assimp, etc.)

## 9. Android Build (NDK)

### Android-Specific CMake

```cmake
# Set in toolchain or command line
set(ANDROID_PLATFORM android-24)
set(ANDROID_ABI arm64-v8a)
set(ANDROID_STL c++_shared)

# Core + plugins build as normal
# Output: build/plugins/*.so

# Example build command
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-24 \
      -DCMAKE_BUILD_TYPE=Release \
      ..
```

### Android Packaging
- Core linked into APK
- Plugins copied to `lib/arm64-v8a/`
- Assets cooked offline, copied to `assets/`

## 10. Windows Build

```bash
# Configure
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build
cmake --build build --config Release

# Output
build/
├── Release/
│   └── SecretEngine_Core.lib
└── plugins/
    ├── VulkanRenderer.dll
    └── ...
```

## 11. Compile Flags (Strict)

### Global Flags

```cmake
if(MSVC)
    target_compile_options(SecretEngine_Core PRIVATE
        /W4           # Warning level 4
        /WX           # Warnings as errors
        /permissive-  # Standards compliance
        /MP           # Multi-processor build
    )
else()
    target_compile_options(SecretEngine_Core PRIVATE
        -Wall
        -Wextra
        -Werror
        -pedantic
    )
endif()
```

### Optimization Flags

```cmake
# Debug
target_compile_options(SecretEngine_Core PRIVATE
    $<$<CONFIG:Debug>:-O0 -g -DSE_DEBUG>
)

# Release
target_compile_options(SecretEngine_Core PRIVATE
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)
```

### Mobile-Specific Flags

```cmake
if(ANDROID)
    target_compile_options(SecretEngine_Core PRIVATE
        -fno-exceptions     # No exceptions
        -fno-rtti           # No RTTI
        -fvisibility=hidden # Hide symbols
    )
endif()
```

## 12. Precompiled Headers (PCH)

### Core PCH

```cmake
target_precompile_headers(SecretEngine_Core
    PRIVATE
        <cstdint>
        <cstring>
        <vector>
        <memory>
)
```

### Plugin PCH

```cmake
target_precompile_headers(VulkanRenderer
    PRIVATE
        <vulkan/vulkan.h>
        <SecretEngine/Core.h>
)
```

PCH speeds up incremental builds.

## 13. Symbol Visibility

### Core Exports

```cpp
// Core.h
#ifdef _WIN32
    #define SE_API __declspec(dllexport)
#else
    #define SE_API __attribute__((visibility("default")))
#endif
```

### Plugin Exports

```cpp
// Plugin entry points
extern "C" {
    SE_PLUGIN_API IPlugin* CreatePlugin();
    SE_PLUGIN_API void DestroyPlugin(IPlugin* plugin);
}
```

Only necessary symbols are exported.

## 14. Build Configurations

### Debug
- No optimizations
- Validation enabled
- Logging verbose
- Asserts enabled

### Release
- Full optimizations
- Validation disabled
- Logging minimal
- Asserts disabled

### RelWithDebInfo
- Optimizations enabled
- Debug symbols
- Asserts enabled
- Used for profiling

## 15. Incremental Build Strategy

### Rules for Fast Builds
1. Core rarely changes → stable
2. Plugins change independently
3. Header changes minimize
4. Forward declarations preferred
5. PIMPL pattern where appropriate

### Build Times (Target)
- Full build (clean): < 2 minutes
- Incremental (plugin): < 10 seconds
- Incremental (core): < 30 seconds

## 16. Testing Integration

```cmake
# tests/CMakeLists.txt

include(GoogleTest)

add_executable(CoreTests
    test_entity.cpp
    test_allocator.cpp
    test_plugin_manager.cpp
)

target_link_libraries(CoreTests
    PRIVATE
        SecretEngine_Core
        GTest::gtest_main
)

gtest_discover_tests(CoreTests)
```

Tests run automatically in CI.

## 17. Code Coverage (Debug)

```cmake
option(SE_ENABLE_COVERAGE "Enable code coverage" OFF)

if(SE_ENABLE_COVERAGE)
    target_compile_options(SecretEngine_Core PRIVATE
        --coverage
    )
    target_link_options(SecretEngine_Core PRIVATE
        --coverage
    )
endif()
```

Coverage reports generated post-test.

## 18. Static Analysis

```cmake
option(SE_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)

if(SE_ENABLE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY
        clang-tidy;
        -checks=*,-readability-*;
    )
endif()
```

Static analysis runs in CI, not locally.

## 19. Build Output Structure

```
build/
├── core/
│   ├── SecretEngine_Core.lib
│   └── ...
├── plugins/
│   ├── VulkanRenderer/
│   │   ├── VulkanRenderer.dll
│   │   └── plugin_manifest.json
│   └── ...
├── tools/
│   ├── AssetCooker.exe
│   └── ...
└── tests/
    └── CoreTests.exe
```

## 20. Packaging for Distribution

### Game Package Structure
```
MyGame/
├── MyGame.exe                  # Game entry point
├── SecretEngine_Core.dll       # Core runtime
├── plugins/
│   ├── VulkanRenderer/
│   │   ├── VulkanRenderer.dll
│   │   └── plugin_manifest.json
│   └── ...
├── Assets/                     # Cooked assets (.bin)
└── engine_config.json
```

### Android APK Structure
```
MyGame.apk
├── lib/arm64-v8a/
│   ├── libSecretEngine_Core.so
│   ├── libVulkanRenderer.so
│   └── ...
├── assets/
│   ├── meshes/
│   ├── textures/
│   └── scenes/
└── AndroidManifest.xml
```

## 21. Build Scripts

### Windows Build Script

```batch
@echo off
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

### Android Build Script

```bash
#!/bin/bash
cmake -B build-android \
      -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-24 \
      -DCMAKE_BUILD_TYPE=Release

cmake --build build-android
```

## 22. CI/CD Integration

### GitHub Actions Example

```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Configure
        run: cmake -B build
      - name: Build
        run: cmake --build build
      - name: Test
        run: ctest --test-dir build
```

## 23. Forbidden Build Practices

❌ Custom build scripts (use CMake)
❌ Checked-in binaries
❌ Unversioned dependencies
❌ Platform-specific build files in VCS
❌ Build outputs in source tree
❌ Global compiler flags (use target-specific)

## 24. Build Troubleshooting

### Common Issues
1. **Long build times** → Check PCH, reduce includes
2. **Link errors** → Verify symbol visibility
3. **Android build fails** → Check NDK version
4. **Incremental broken** → Clean build folder

Status

✅ FROZEN
This is the build system. All projects follow these rules.
