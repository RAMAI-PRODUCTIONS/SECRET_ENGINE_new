# SecretEngine
# Module: build
# Responsibility: Root CMake configuration - Cross-platform
# Targets: Windows PC, Android Physical Devices, Android Emulators

cmake_minimum_required(VERSION 3.20)

# FIX: Add 'C' to LANGUAGES so CMake can compile .c files from the NDK
project(SecretEngine VERSION 0.1.0 LANGUAGES C CXX)

# --- HYBRID STANDARD STRATEGY ---
# Note: Using C++20 for now as CMake 3.22.1 doesn't recognize C++26
# std::span is available in C++20, and we have fallbacks for C++26-only features
if(ANDROID)
    set(CMAKE_CXX_STANDARD 20)
    add_definitions(-DSE_PLATFORM_ANDROID)
    message(STATUS "Target: ANDROID - Standard: C++20 (with C++26 features via fallbacks)")
elseif(WIN32)
    set(CMAKE_CXX_STANDARD 20)
    add_definitions(-DSE_PLATFORM_WINDOWS)
    message(STATUS "Target: WINDOWS - Standard: C++20 (with C++26 features via fallbacks)")
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- BUILD ARTIFACT LOCATIONS ---
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- GLOBAL INCLUDES ---
include_directories(core/include)

# --- DEPENDENCIES ---
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.8.21
)
FetchContent_MakeAvailable(json tinygltf)

# --- CORE ARCHITECTURE ---
add_subdirectory(core)
add_subdirectory(plugins)

# --- DESKTOP-ONLY COMPONENTS ---
if(NOT ANDROID)
    option(SE_BUILD_TOOLS "Build offline asset tools" ON)
    option(SE_BUILD_TESTS "Build internal test suite" ON)

    if(SE_BUILD_TOOLS)
        add_subdirectory(tools)
    endif()

    if(SE_BUILD_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()

# --- ANDROID ENTRY POINT ---
if(ANDROID)
    # Define the path to the NDK's native_app_glue
    set(NATIVE_APP_GLUE_DIR "${ANDROID_NDK}/sources/android/native_app_glue")

    # 1. Build our Main Shared Library (includes native_app_glue directly)
    add_library(SecretEngine SHARED 
        core/src/platform/android/AndroidMain.cpp
        "${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c"
    )
    
    # 2. Add include directories
    target_include_directories(SecretEngine PRIVATE "${NATIVE_APP_GLUE_DIR}")

    # 3. Link everything together
    target_link_libraries(SecretEngine 
        PRIVATE 
        SecretEngine_Core 
        VulkanRenderer 
        AndroidInput   
        GameLogic      
        CameraPlugin   
        DebugPlugin
        PhysicsPlugin      # NEW: FPS Physics
        GameplayTagSystem  # NEW: Gameplay Tag System
        LevelSystem        # NEW: Level System
        FPSGameLogic       # NEW: FPS Game Logic
        FPSUIPlugin        # NEW: FPS UI
        android 
        log
        vulkan 
    )

    # FIX: Enforce 16KB page alignment for Android 15 compatibility
    target_link_options(SecretEngine PRIVATE "-Wl,-z,max-page-size=16384")
endif()