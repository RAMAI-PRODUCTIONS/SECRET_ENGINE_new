# SecretEngine
# Module: LevelSystem v7.3 Ultra Full
# Responsibility: Complete v7.3 Level Management with Advanced Streaming
# Dependencies: Core, nlohmann_json, glm

cmake_minimum_required(VERSION 3.16)

add_library(LevelSystem STATIC
    # Legacy system (for compatibility)
    src/LevelTypes.h
    src/LevelManager.h
    src/LevelManager.cpp
    src/LevelLoader.h
    src/LevelLoader.cpp
    src/LevelSystemPlugin.h
    src/LevelSystemPlugin.cpp
    
    # Modern system
    src/ModernLevelTypes.h
    src/ModernLevelLoader.h
    src/ModernLevelLoader.cpp
    
    # v7.3 Ultra Full System
    src/V73LevelSystem.h
    src/V73LevelManager.h
    src/V73LevelManager.cpp
    src/V73LevelManagerImpl.cpp
    src/V73LevelSystemPlugin.h
    src/V73LevelSystemPlugin.cpp
    src/TriggerSystem.h
    src/TriggerSystem.cpp
    
    # NEW: Scene Loader for new ECS
    include/SceneLoader.h
    src/SceneLoader.cpp
    
    # Advanced Streaming System
    src/Streaming/LevelStreamingSubsystem.h
    src/Streaming/LevelStreamingSubsystem.cpp
    src/Streaming/NetworkedLevelStreamer.h
    
    # Legacy v7.3 components
    src/v73/V73CoreTypes.h
    src/v73/GPUInstanceManager.h
    src/v73/GPUInstanceManager.cpp
    # src/v73/V73JsonParser.h  # Excluded: uses C++23 std::expected
    # src/v73/V73JsonParser.cpp  # Excluded: uses C++23 std::expected
)

target_include_directories(LevelSystem PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/v73
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Streaming
    ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(LevelSystem
    PUBLIC
    SecretEngine_Core
    nlohmann_json::nlohmann_json
    glm::glm
)

set_target_properties(LevelSystem PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Compiler definitions for v7.3 features
target_compile_definitions(LevelSystem PUBLIC
    V73_LEVEL_SYSTEM_ENABLED
    V73_STREAMING_ENABLED
    V73_NETWORKING_ENABLED
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(LevelSystem PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

# Optional: Build examples
option(BUILD_LEVEL_SYSTEM_EXAMPLES "Build Level System examples" OFF)

if(BUILD_LEVEL_SYSTEM_EXAMPLES)
    add_executable(LevelSystemExample
        examples/usage_example.cpp
    )
    
    target_link_libraries(LevelSystemExample PRIVATE
        LevelSystem
    )
    
    set_target_properties(LevelSystemExample PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
endif()
