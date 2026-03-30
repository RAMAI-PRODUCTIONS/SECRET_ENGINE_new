// SecretEngine
// Module: LevelSystem
// Responsibility: Unreal-like Level Management System
// Dependencies: Core

#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>

namespace SecretEngine::Levels {

// Level State
enum class LevelState {
    Unloaded,       // Not in memory
    Loading,        // Currently loading
    Loaded,         // In memory, not visible
    Visible,        // Active and rendering
    Unloading       // Currently unloading
};

// Level Type
enum class LevelType {
    Persistent,     // Always loaded (like Unreal's Persistent Level)
    Streaming,      // Dynamically loaded/unloaded
    SubLevel        // Part of a larger level
};

// Level Load Priority
enum class LoadPriority {
    Immediate,      // Load right now (blocking)
    High,           // Load ASAP (async)
    Normal,         // Load when convenient
    Low,            // Load in background
    Lazy            // Load only when needed
};

// Level Visibility
enum class LevelVisibility {
    Hidden,         // Loaded but not rendered
    Visible,        // Rendered and active
    Editor          // Only visible in editor
};

// Level Streaming Method
enum class StreamingMethod {
    Distance,       // Load based on player distance
    Volume,         // Load when player enters volume
    Manual,         // Load via code/blueprint
    Always          // Always loaded
};

// Level Bounds (for streaming)
struct LevelBounds {
    float center[3] = {0, 0, 0};
    float extents[3] = {100, 100, 100};  // Half-size
    
    bool ContainsPoint(const float point[3]) const {
        return (point[0] >= center[0] - extents[0] && point[0] <= center[0] + extents[0]) &&
               (point[1] >= center[1] - extents[1] && point[1] <= center[1] + extents[1]) &&
               (point[2] >= center[2] - extents[2] && point[2] <= center[2] + extents[2]);
    }
    
    float DistanceToPoint(const float point[3]) const {
        float dx = point[0] - center[0];
        float dy = point[1] - center[1];
        float dz = point[2] - center[2];
        return sqrtf(dx*dx + dy*dy + dz*dz);
    }
};

// Level Definition (from JSON)
struct LevelDefinition {
    static constexpr int MAX_NAME_LENGTH = 64;
    static constexpr int MAX_PATH_LENGTH = 256;
    static constexpr int MAX_SUBLEVELS = 16;
    
    char name[MAX_NAME_LENGTH] = "";
    char path[MAX_PATH_LENGTH] = "";
    
    LevelType type = LevelType::Streaming;
    StreamingMethod streamingMethod = StreamingMethod::Manual;
    LoadPriority loadPriority = LoadPriority::Normal;
    
    LevelBounds bounds;
    float streamingDistance = 1000.0f;  // For distance-based streaming
    
    // Sub-levels
    char subLevels[MAX_SUBLEVELS][MAX_NAME_LENGTH];
    int subLevelCount = 0;
    
    // Metadata
    bool autoLoad = false;
    bool blockOnLoad = false;
    bool makeVisibleAfterLoad = true;
    
    void AddSubLevel(const char* subLevelName) {
        if (subLevelCount < MAX_SUBLEVELS) {
            strncpy(subLevels[subLevelCount++], subLevelName, MAX_NAME_LENGTH - 1);
        }
    }
};

// Level Instance (runtime)
struct Level {
    static constexpr uint32_t TypeID = 0x5001;
    static constexpr int MAX_ENTITIES = 1024;
    
    LevelDefinition definition;
    LevelState state = LevelState::Unloaded;
    LevelVisibility visibility = LevelVisibility::Hidden;
    
    // Entities in this level
    uint32_t entities[MAX_ENTITIES];
    int entityCount = 0;
    
    // Streaming data
    float loadProgress = 0.0f;
    float timeLoaded = 0.0f;
    
    // Parent/child relationships
    int parentLevelIndex = -1;
    int childLevelIndices[16];
    int childLevelCount = 0;
    
    void AddEntity(uint32_t entityId) {
        if (entityCount < MAX_ENTITIES) {
            entities[entityCount++] = entityId;
        }
    }
    
    void RemoveEntity(uint32_t entityId) {
        for (int i = 0; i < entityCount; i++) {
            if (entities[i] == entityId) {
                // Shift remaining entities
                for (int j = i; j < entityCount - 1; j++) {
                    entities[j] = entities[j + 1];
                }
                entityCount--;
                return;
            }
        }
    }
    
    bool ContainsEntity(uint32_t entityId) const {
        for (int i = 0; i < entityCount; i++) {
            if (entities[i] == entityId) return true;
        }
        return false;
    }
};

// Level Streaming Volume (trigger for loading)
struct LevelStreamingVolume {
    static constexpr uint32_t TypeID = 0x5002;
    
    char levelName[64] = "";
    LevelBounds bounds;
    
    bool loadOnEnter = true;
    bool unloadOnExit = true;
    bool blockOnLoad = false;
    
    // State
    bool playerInside = false;
    bool hasTriggered = false;
};

// Level Component (marks entity as belonging to a level)
struct LevelComponent {
    static constexpr uint32_t TypeID = 0x5003;
    
    char levelName[64] = "";
    int levelIndex = -1;
    bool isPersistent = false;
};

// Level Transition (for seamless travel)
struct LevelTransition {
    char fromLevel[64] = "";
    char toLevel[64] = "";
    
    float transitionDuration = 1.0f;
    float fadeOutDuration = 0.5f;
    float fadeInDuration = 0.5f;
    
    bool keepPersistentLevel = true;
    bool keepPlayerState = true;
    
    // Spawn point in new level
    float spawnPosition[3] = {0, 0, 0};
    float spawnRotation[3] = {0, 0, 0};
};

} // namespace SecretEngine::Levels
