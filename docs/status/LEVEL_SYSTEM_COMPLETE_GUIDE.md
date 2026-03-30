# Level System - Complete Guide

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Level Types](#level-types)
4. [Creating Levels](#creating-levels)
5. [Level Switching](#level-switching)
6. [Streaming](#streaming)
7. [Advanced Features](#advanced-features)
8. [API Reference](#api-reference)
9. [Examples](#examples)
10. [Best Practices](#best-practices)

---

## Overview

The Level System provides Unreal Engine-like level management for SecretEngine, allowing you to:
- Create multiple levels, each with its own scene.json
- Switch between levels dynamically
- Stream levels based on distance or volumes
- Organize levels with sub-levels
- Track entities per level
- Manage memory efficiently

### Key Features
✅ **Multiple Levels** - Each level has its own scene.json hierarchy
✅ **Level Switching** - Load/unload levels dynamically
✅ **Persistent Levels** - Always-loaded content (menus, UI)
✅ **Streaming Levels** - Auto-load based on distance/volumes
✅ **Sub-Levels** - Organize large levels into pieces
✅ **Entity Tracking** - Know which level entities belong to
✅ **Memory Management** - Unload levels to free memory
✅ **Transitions** - Seamless level changes with fade effects

---

## Architecture

### System Components

```
LevelSystem Plugin
├── LevelManager
│   ├── Level Loading/Unloading
│   ├── Level Streaming
│   ├── Level Transitions
│   └── Entity Tracking
├── LevelLoader
│   ├── Scene.json Parser
│   ├── Entity Creation
│   └── Component Setup
└── LevelTypes
    ├── Level Definitions
    ├── Level States
    └── Level Components
```

### Data Flow

```
1. LevelDefinitions.json → Defines all levels
2. LevelManager → Manages level lifecycle
3. LevelLoader → Loads scene.json files
4. Entities Created → With Transform, Mesh, LevelComponent
5. Level Tracking → Entities tracked per level
6. Cleanup → Entities destroyed on unload
```

### File Structure

```
SecretEngine/
├── data/
│   └── LevelDefinitions.json        ← All level definitions
├── Assets/
│   ├── main_menu.json               ← MainMenu level
│   ├── fps_arena.json               ← FPS Arena level
│   ├── racing_track.json            ← Racing level
│   └── scene.json                   ← Original scene
├── plugins/
│   └── LevelSystem/
│       ├── src/
│       │   ├── LevelTypes.h         ← Data structures
│       │   ├── LevelManager.h/cpp   ← Level management
│       │   ├── LevelLoader.h/cpp    ← Scene loading
│       │   └── LevelSystemPlugin.h/cpp
│       ├── CMakeLists.txt
│       └── plugin_manifest.json
└── docs/
    ├── LEVEL_SYSTEM.md              ← System documentation
    ├── MULTIPLE_LEVELS_GUIDE.md     ← Multiple levels guide
    ├── LEVEL_SWITCHING_EXAMPLE.md   ← Code examples
    └── SCENE_TO_LEVEL_MIGRATION.md  ← Migration guide
```

---

## Level Types

### 1. Persistent Level
Always loaded, never unloaded. Perfect for menus, UI, persistent game state.

```json
{
  "name": "MainMenu",
  "path": "Assets/main_menu.json",
  "type": "persistent",
  "streamingMethod": "always",
  "autoLoad": true
}
```

**Use Cases:**
- Main menu
- HUD/UI
- Game manager
- Persistent audio

### 2. Streaming Level
Loaded/unloaded dynamically. Perfect for game levels, zones, areas.

```json
{
  "name": "FPS_Arena",
  "path": "Assets/fps_arena.json",
  "type": "streaming",
  "streamingMethod": "manual",
  "autoLoad": false,
  "blockOnLoad": true
}
```

**Use Cases:**
- Game levels
- Open world zones
- Dungeons
- Battle arenas

### 3. Sub-Level
Part of a larger level. Perfect for organizing complex levels.

```json
{
  "name": "Arena_Lighting",
  "path": "Assets/arena_lighting.json",
  "type": "sublevel",
  "streamingMethod": "manual"
}
```

**Use Cases:**
- Lighting
- Audio
- Props
- Effects

---

## Creating Levels

### Step 1: Create Scene File

Each level needs a scene.json file with its entity hierarchy.

**Example: FPS Arena (`Assets/fps_arena.json`)**
```json
{
  "entities": [
    {
      "name": "PlayerStart",
      "transform": {
        "position": [0, 1, 10],
        "rotation": [0, 0, 0],
        "scale": [1, 1, 1]
      },
      "isPlayerStart": true
    },
    {
      "name": "ArenaFloor",
      "transform": {
        "position": [0, 0, 0],
        "rotation": [0, 0, 0],
        "scale": [100, 1, 100]
      },
      "mesh": {
        "path": "meshes/Plane.meshbin",
        "color": [0.5, 0.5, 0.5, 1.0],
        "texture": "textures/concrete.jpeg"
      }
    },
    {
      "name": "Enemy_1",
      "transform": {
        "position": [10, 0, 0],
        "rotation": [0, 180, 0],
        "scale": [15, 15, 15]
      },
      "mesh": {
        "path": "meshes/Character.meshbin",
        "color": [1.0, 0.0, 0.0, 1.0]
      }
    }
  ]
}
```

### Step 2: Add Level Definition

Add to `data/LevelDefinitions.json`:

```json
{
  "version": "1.0.0",
  "levels": [
    {
      "name": "FPS_Arena",
      "path": "Assets/fps_arena.json",
      "type": "streaming",
      "streamingMethod": "manual",
      "autoLoad": false,
      "blockOnLoad": true,
      "makeVisibleAfterLoad": true,
      "bounds": {
        "center": [0, 0, 0],
        "extents": [100, 50, 100]
      }
    }
  ]
}
```

### Step 3: Load Level in Code

```cpp
auto* levelPlugin = static_cast<Levels::LevelSystemPlugin*>(
    core->GetCapability("level_system")
);
auto* levelManager = levelPlugin->GetLevelManager();

// Load level
levelManager->LoadLevel("FPS_Arena", LoadPriority::Immediate);
```

---

## Level Switching

### Basic Switching

```cpp
// Get level manager
auto* levelManager = GetLevelManager();

// Switch from MainMenu to FPS_Arena
void StartGame() {
    levelManager->HideLevel("MainMenu");
    levelManager->LoadLevel("FPS_Arena", LoadPriority::Immediate);
}

// Return to menu
void ReturnToMenu() {
    levelManager->UnloadLevel("FPS_Arena");
    levelManager->ShowLevel("MainMenu");
}
```

### Complete Game Manager

```cpp
class GameManager {
public:
    void Initialize(ICore* core) {
        m_core = core;
        auto* levelPlugin = static_cast<Levels::LevelSystemPlugin*>(
            core->GetCapability("level_system")
        );
        m_levelManager = levelPlugin->GetLevelManager();
        strncpy(m_currentLevel, "MainMenu", 64);
    }
    
    void SwitchToLevel(const char* newLevel) {
        auto logger = m_core->GetLogger();
        
        char msg[128];
        snprintf(msg, sizeof(msg), "Switching from %s to %s", 
                 m_currentLevel, newLevel);
        logger->LogInfo("GameManager", msg);
        
        // Handle current level
        if (strcmp(m_currentLevel, "MainMenu") == 0) {
            m_levelManager->HideLevel(m_currentLevel);
        } else {
            m_levelManager->UnloadLevel(m_currentLevel);
        }
        
        // Load new level
        if (strcmp(newLevel, "MainMenu") == 0) {
            m_levelManager->ShowLevel(newLevel);
        } else {
            m_levelManager->LoadLevel(newLevel, LoadPriority::Immediate);
        }
        
        strncpy(m_currentLevel, newLevel, 64);
    }
    
    void StartFPSGame() { SwitchToLevel("FPS_Arena"); }
    void StartRacingGame() { SwitchToLevel("RacingTrack"); }
    void ReturnToMenu() { SwitchToLevel("MainMenu"); }
    
private:
    ICore* m_core;
    Levels::LevelManager* m_levelManager;
    char m_currentLevel[64];
};
```

### Level Switching with Transitions

```cpp
void SwitchLevelWithFade(const char* fromLevel, const char* toLevel) {
    Levels::LevelTransition transition;
    strncpy(transition.fromLevel, fromLevel, 64);
    strncpy(transition.toLevel, toLevel, 64);
    
    transition.transitionDuration = 2.0f;
    transition.fadeOutDuration = 0.5f;
    transition.fadeInDuration = 0.5f;
    transition.keepPersistentLevel = true;
    transition.keepPlayerState = false;
    
    transition.spawnPosition[0] = 0;
    transition.spawnPosition[1] = 1;
    transition.spawnPosition[2] = 10;
    
    m_levelManager->TransitionToLevel(toLevel, transition);
}
```

---

## Streaming

### Distance-Based Streaming

Automatically load/unload levels based on player distance.

```json
{
  "name": "OpenWorld_Zone1",
  "path": "Assets/zone1.json",
  "streamingMethod": "distance",
  "streamingDistance": 2000.0,
  "bounds": {
    "center": [0, 0, 0],
    "extents": [1000, 200, 1000]
  }
}
```

**Behavior:**
- Load when player within 2000 units
- Unload when player > 3000 units (1.5x distance)
- Automatic, no code needed

### Volume-Based Streaming

Load levels when player enters trigger volume.

```cpp
Levels::LevelStreamingVolume volume;
strncpy(volume.levelName, "Dungeon_Floor1", 64);
volume.bounds.center[0] = 100;
volume.bounds.center[1] = -50;
volume.bounds.center[2] = 100;
volume.bounds.extents[0] = 10;
volume.bounds.extents[1] = 10;
volume.bounds.extents[2] = 10;
volume.loadOnEnter = true;
volume.unloadOnExit = true;
volume.blockOnLoad = true;

levelManager->RegisterStreamingVolume(volume);
```

**Behavior:**
- Player enters volume → Load level
- Player exits volume → Unload level
- Optional blocking load

### Manual Streaming

Full control over when levels load/unload.

```cpp
// Load when needed
levelManager->LoadLevel("BossArena", LoadPriority::High);

// Unload when done
levelManager->UnloadLevel("BossArena");
```

---

## Advanced Features

### Sub-Levels

Organize large levels into smaller pieces.

```json
{
  "name": "FPS_Arena",
  "path": "Assets/fps_arena.json",
  "subLevels": [
    "Arena_Lighting",
    "Arena_Audio",
    "Arena_Props"
  ]
}
```

```cpp
// Load parent (auto-loads sub-levels)
levelManager->LoadLevel("FPS_Arena");

// Unload parent (auto-unloads sub-levels)
levelManager->UnloadLevel("FPS_Arena");

// Manual sub-level control
levelManager->LoadSubLevels("FPS_Arena");
levelManager->UnloadSubLevels("FPS_Arena");
```

### Entity Tracking

Know which level entities belong to.

```cpp
// Get entity's level
const char* levelName = levelManager->GetEntityLevel(entityId);

// Get all entities in level
Level* level = levelManager->GetLevel("FPS_Arena");
for (int i = 0; i < level->entityCount; i++) {
    uint32_t entityId = level->entities[i];
    // Do something with entity
}

// Check if entity has level component
auto* levelComp = static_cast<LevelComponent*>(
    world->GetComponent(entity, LevelComponent::TypeID)
);
if (levelComp) {
    printf("Entity belongs to: %s\n", levelComp->levelName);
}
```

### Load Priorities

Control loading order and behavior.

```cpp
// Immediate: Block until loaded
levelManager->LoadLevel("MainMenu", LoadPriority::Immediate);

// High: Load ASAP (async)
levelManager->LoadLevel("FPS_Arena", LoadPriority::High);

// Normal: Load when convenient
levelManager->LoadLevel("Zone1", LoadPriority::Normal);

// Low: Background loading
levelManager->LoadLevel("Zone2", LoadPriority::Low);

// Lazy: Load only when needed
levelManager->LoadLevel("Cutscene", LoadPriority::Lazy);
```

### Level States

Track level lifecycle.

```cpp
enum class LevelState {
    Unloaded,    // Not in memory
    Loading,     // Currently loading
    Loaded,      // In memory, not visible
    Visible,     // Active and rendering
    Unloading    // Currently unloading
};

// Check state
LevelState state = levelManager->GetLevelState("FPS_Arena");

// Check if loaded
if (levelManager->IsLevelLoaded("FPS_Arena")) {
    // Level is loaded
}

// Check if visible
if (levelManager->IsLevelVisible("FPS_Arena")) {
    // Level is rendering
}
```

---

## API Reference

### LevelManager

#### Loading
```cpp
bool LoadLevel(const char* levelName, LoadPriority priority = Normal);
bool UnloadLevel(const char* levelName);
bool ReloadLevel(const char* levelName);
```

#### Visibility
```cpp
void ShowLevel(const char* levelName);
void HideLevel(const char* levelName);
void ToggleLevelVisibility(const char* levelName);
```

#### Queries
```cpp
Level* GetLevel(const char* levelName);
LevelState GetLevelState(const char* levelName) const;
bool IsLevelLoaded(const char* levelName) const;
bool IsLevelVisible(const char* levelName) const;
```

#### Persistent Level
```cpp
void SetPersistentLevel(const char* levelName);
Level* GetPersistentLevel();
```

#### Streaming
```cpp
void UpdateStreaming(float dt, const float playerPosition[3]);
void EnableStreaming(bool enable);
bool IsStreamingEnabled() const;
```

#### Transitions
```cpp
void TransitionToLevel(const char* levelName, const LevelTransition& transition);
bool IsTransitioning() const;
float GetTransitionProgress() const;
```

#### Sub-Levels
```cpp
bool LoadSubLevels(const char* parentLevelName);
bool UnloadSubLevels(const char* parentLevelName);
```

#### Entity Management
```cpp
void AddEntityToLevel(uint32_t entityId, const char* levelName);
void RemoveEntityFromLevel(uint32_t entityId);
const char* GetEntityLevel(uint32_t entityId) const;
```

#### Utilities
```cpp
void UnloadAllLevels();
void GetLoadedLevels(char levelNames[][64], int& count, int maxCount) const;
int GetLoadedLevelCount() const;
void PrintLevelInfo() const;
```

### LevelLoader

```cpp
bool LoadLevelFromFile(const char* levelPath, Level* level);
bool LoadSceneFormat(const char* scenePath, Level* level);
bool SaveLevelToFile(const char* levelPath, const Level* level);
```

---

## Examples

### Example 1: Simple FPS Game

```cpp
// Levels
// - MainMenu (persistent)
// - Tutorial (streaming)
// - Level_1 (streaming)
// - Level_2 (streaming)
// - BossFight (streaming)

class FPSGame {
public:
    void Initialize() {
        // MainMenu auto-loads
        m_currentLevel = "MainMenu";
    }
    
    void StartNewGame() {
        SwitchToLevel("Tutorial");
    }
    
    void CompleteLevel() {
        const char* levels[] = {
            "Tutorial", "Level_1", "Level_2", "BossFight"
        };
        
        m_levelIndex++;
        if (m_levelIndex < 4) {
            SwitchToLevel(levels[m_levelIndex]);
        } else {
            // Game complete
            SwitchToLevel("MainMenu");
            ShowVictoryScreen();
        }
    }
    
private:
    void SwitchToLevel(const char* newLevel) {
        if (strcmp(m_currentLevel, "MainMenu") != 0) {
            m_levelManager->UnloadLevel(m_currentLevel);
        } else {
            m_levelManager->HideLevel(m_currentLevel);
        }
        
        if (strcmp(newLevel, "MainMenu") == 0) {
            m_levelManager->ShowLevel(newLevel);
        } else {
            m_levelManager->LoadLevel(newLevel, LoadPriority::Immediate);
        }
        
        strncpy(m_currentLevel, newLevel, 64);
    }
    
    char m_currentLevel[64];
    int m_levelIndex = 0;
};
```

### Example 2: Open World Game

```cpp
// Automatic streaming based on player position

// Level Definitions
{
  "levels": [
    {
      "name": "Zone_Forest",
      "streamingMethod": "distance",
      "streamingDistance": 2000.0,
      "bounds": {"center": [0, 0, 0], "extents": [1000, 200, 1000]}
    },
    {
      "name": "Zone_Desert",
      "streamingMethod": "distance",
      "streamingDistance": 2000.0,
      "bounds": {"center": [2000, 0, 0], "extents": [1000, 200, 1000]}
    },
    {
      "name": "Zone_Mountains",
      "streamingMethod": "distance",
      "streamingDistance": 2000.0,
      "bounds": {"center": [0, 0, 2000], "extents": [1000, 200, 1000]}
    }
  ]
}

// Code
void Update(float dt) {
    // Get player position
    float playerPos[3] = {
        playerTransform->position[0],
        playerTransform->position[1],
        playerTransform->position[2]
    };
    
    // Update streaming (automatic)
    levelManager->UpdateStreaming(dt, playerPos);
    
    // Zones load/unload automatically based on distance!
}
```

### Example 3: Racing Game

```cpp
// Multiple tracks, switch between them

class RacingGame {
public:
    void SelectTrack(int trackIndex) {
        const char* tracks[] = {
            "Track_City",
            "Track_Desert",
            "Track_Mountain",
            "Track_Beach"
        };
        
        if (trackIndex >= 0 && trackIndex < 4) {
            LoadTrack(tracks[trackIndex]);
        }
    }
    
private:
    void LoadTrack(const char* trackName) {
        // Unload current track
        if (m_currentTrack[0] != '\0') {
            m_levelManager->UnloadLevel(m_currentTrack);
        }
        
        // Load new track
        m_levelManager->LoadLevel(trackName, LoadPriority::Immediate);
        
        // Update state
        strncpy(m_currentTrack, trackName, 64);
        
        // Initialize race
        InitializeRace();
    }
    
    char m_currentTrack[64] = "";
};
```

---

## Best Practices

### 1. Use Persistent Levels for UI
```json
{
  "name": "MainMenu",
  "type": "persistent",
  "autoLoad": true
}
```

### 2. Use Streaming for Game Levels
```json
{
  "name": "Level_1",
  "type": "streaming",
  "autoLoad": false
}
```

### 3. Always Unload Before Loading
```cpp
levelManager->UnloadLevel("OldLevel");
levelManager->LoadLevel("NewLevel");
```

### 4. Use Sub-Levels for Organization
```json
{
  "name": "BigLevel",
  "subLevels": ["Lighting", "Audio", "Props"]
}
```

### 5. Enable Streaming for Open Worlds
```json
{
  "streamingMethod": "distance",
  "streamingDistance": 2000.0
}
```

### 6. Use Load Priorities
```cpp
// Critical content
levelManager->LoadLevel("MainLevel", LoadPriority::Immediate);

// Background content
levelManager->LoadLevel("OptionalLevel", LoadPriority::Low);
```

### 7. Track Current Level
```cpp
class GameState {
    char m_currentLevel[64];
    
    void SetLevel(const char* level) {
        strncpy(m_currentLevel, level, 64);
    }
};
```

### 8. Clean Up on Exit
```cpp
void Shutdown() {
    levelManager->UnloadAllLevels();
}
```

### 9. Use Loading Screens
```cpp
void SwitchLevel(const char* newLevel) {
    ShowLoadingScreen();
    levelManager->LoadLevel(newLevel, LoadPriority::High);
    // Hide loading screen when complete
}
```

### 10. Test Level Switching
```cpp
void TestLevelSwitching() {
    // Test all level transitions
    for (const char* level : allLevels) {
        levelManager->LoadLevel(level);
        // Verify entities created
        levelManager->UnloadLevel(level);
        // Verify entities destroyed
    }
}
```

---

## Performance

### Memory Usage
- **Persistent Level**: Always in memory (~100KB - 10MB)
- **Streaming Level**: Loaded on demand (~1MB - 100MB)
- **Sub-Level**: Part of parent level (~100KB - 10MB)

### Loading Times
- **Immediate**: 0-500ms (blocks)
- **High**: 100-1000ms (async)
- **Normal**: 500-2000ms (async)
- **Low**: 1000-5000ms (background)

### Optimization Tips
1. Keep persistent levels small
2. Use sub-levels for large levels
3. Stream distant content
4. Unload unused levels
5. Use async loading
6. Preload next level
7. Use level instances (future)
8. Compress level data (future)

---

## Troubleshooting

### Level Not Loading
```cpp
if (!levelManager->IsLevelLoaded("MyLevel")) {
    // Check level definitions
    // Check file path
    // Check logs
    levelManager->PrintLevelInfo();
}
```

### Entities Not Rendering
```cpp
// Make sure level is visible
levelManager->ShowLevel("MyLevel");
```

### Duplicate Entities
If renderer AND level system both load scene.json:
- Disable one of them
- Or use different scene files

### Memory Leaks
```cpp
// Always unload levels when done
levelManager->UnloadLevel("CompletedLevel");
```

### Slow Loading
```cpp
// Use async loading
levelManager->LoadLevel("BigLevel", LoadPriority::High);

// Or use sub-levels
levelManager->LoadSubLevels("BigLevel");
```

---

## Summary

✅ **Multiple Levels** - Each with its own scene.json hierarchy
✅ **Easy Switching** - Load/unload with simple API
✅ **Persistent Levels** - Always-loaded content (menus, UI)
✅ **Streaming Levels** - Auto-load based on distance/volumes
✅ **Sub-Levels** - Organize large levels
✅ **Entity Tracking** - Know which level entities belong to
✅ **Memory Management** - Efficient loading/unloading
✅ **Transitions** - Seamless level changes
✅ **Flexible** - Add new levels without code changes
✅ **Powerful** - Unreal Engine-like features

**Create any game with multiple levels, each with its own scene hierarchy!**

---

## Quick Reference

### Create Level
1. Create scene.json file
2. Add to LevelDefinitions.json
3. Load in code

### Switch Level
```cpp
levelManager->UnloadLevel("OldLevel");
levelManager->LoadLevel("NewLevel");
```

### Check Status
```cpp
if (levelManager->IsLevelLoaded("MyLevel")) {
    // Level is loaded
}
```

### Get Entities
```cpp
Level* level = levelManager->GetLevel("MyLevel");
for (int i = 0; i < level->entityCount; i++) {
    uint32_t entityId = level->entities[i];
}
```

### Enable Streaming
```json
{
  "streamingMethod": "distance",
  "streamingDistance": 2000.0
}
```

---

## Documentation Links

- **System Overview**: `docs/LEVEL_SYSTEM.md`
- **Multiple Levels**: `docs/MULTIPLE_LEVELS_GUIDE.md`
- **Code Examples**: `docs/LEVEL_SWITCHING_EXAMPLE.md`
- **Migration Guide**: `docs/SCENE_TO_LEVEL_MIGRATION.md`
- **API Reference**: `plugins/LevelSystem/src/LevelManager.h`

---

**The Level System is production-ready and fully integrated!**
