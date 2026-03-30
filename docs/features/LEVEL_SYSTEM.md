# Level System - Unreal Engine-like Level Management

## Overview
Complete level management system inspired by Unreal Engine's level streaming, persistent levels, and sub-levels.

## Features

### ✅ Level Types
- **Persistent Level**: Always loaded (like Unreal's Persistent Level)
- **Streaming Level**: Dynamically loaded/unloaded based on rules
- **Sub-Level**: Part of a larger level (like Unreal's sub-levels)

### ✅ Streaming Methods
- **Distance-based**: Load when player is within distance
- **Volume-based**: Load when player enters trigger volume
- **Manual**: Load via code
- **Always**: Always loaded

### ✅ Level States
- **Unloaded**: Not in memory
- **Loading**: Currently loading (async)
- **Loaded**: In memory, not visible
- **Visible**: Active and rendering
- **Unloading**: Currently unloading

### ✅ Level Visibility
- **Hidden**: Loaded but not rendered
- **Visible**: Rendered and active
- **Editor**: Only visible in editor

## Level Definitions

### Single JSON File: `data/LevelDefinitions.json`

```json
{
  "levels": [
    {
      "name": "MainMenu",
      "path": "levels/MainMenu.json",
      "type": "persistent",
      "streamingMethod": "always",
      "autoLoad": true
    },
    {
      "name": "FPS_Arena",
      "path": "levels/FPS_Arena.json",
      "type": "streaming",
      "streamingMethod": "manual",
      "blockOnLoad": true,
      "subLevels": ["FPS_Arena_Lighting", "FPS_Arena_Audio"]
    },
    {
      "name": "OpenWorld_Zone1",
      "path": "levels/OpenWorld_Zone1.json",
      "type": "streaming",
      "streamingMethod": "distance",
      "streamingDistance": 2000.0,
      "bounds": {
        "center": [0, 0, 0],
        "extents": [1000, 200, 1000]
      }
    }
  ]
}
```

## Usage

### Load Level System
```cpp
auto* levelPlugin = static_cast<Levels::LevelSystemPlugin*>(
    core->GetCapability("level_system")
);
auto* levelManager = levelPlugin->GetLevelManager();
```

### Load a Level
```cpp
// Manual load
levelManager->LoadLevel("FPS_Arena", LoadPriority::Immediate);

// Load with sub-levels
levelManager->LoadLevel("FPS_Arena");  // Auto-loads sub-levels

// Async load
levelManager->LoadLevel("OpenWorld_Zone1", LoadPriority::Normal);
```

### Unload a Level
```cpp
levelManager->UnloadLevel("FPS_Arena");
```

### Show/Hide Levels
```cpp
// Hide level (keep in memory, stop rendering)
levelManager->HideLevel("FPS_Arena");

// Show level (start rendering)
levelManager->ShowLevel("FPS_Arena");
```

### Check Level State
```cpp
if (levelManager->IsLevelLoaded("FPS_Arena")) {
    // Level is loaded
}

LevelState state = levelManager->GetLevelState("FPS_Arena");
```

### Set Persistent Level
```cpp
// Persistent level never unloads
levelManager->SetPersistentLevel("MainMenu");
```

## Distance-Based Streaming

Levels automatically load/unload based on player distance:

```json
{
  "name": "OpenWorld_Zone1",
  "streamingMethod": "distance",
  "streamingDistance": 2000.0,
  "bounds": {
    "center": [0, 0, 0],
    "extents": [1000, 200, 1000]
  }
}
```

**Behavior**:
- Load when player within 2000 units
- Unload when player > 3000 units (1.5x distance)
- Automatic, no code needed

## Volume-Based Streaming

Load levels when player enters trigger volume:

```cpp
LevelStreamingVolume volume;
strncpy(volume.levelName, "Dungeon_Entrance", 64);
volume.bounds.center[0] = 500;
volume.bounds.center[1] = -50;
volume.bounds.center[2] = 500;
volume.bounds.extents[0] = 100;
volume.bounds.extents[1] = 50;
volume.bounds.extents[2] = 100;
volume.loadOnEnter = true;
volume.unloadOnExit = true;
volume.blockOnLoad = true;

levelManager->RegisterStreamingVolume(volume);
```

**Behavior**:
- Player enters volume → Load level
- Player exits volume → Unload level
- Optional blocking load

## Sub-Levels

Organize large levels into smaller pieces:

```json
{
  "name": "FPS_Arena",
  "subLevels": [
    "FPS_Arena_Lighting",
    "FPS_Arena_Audio",
    "FPS_Arena_Props"
  ]
}
```

**Usage**:
```cpp
// Load parent (auto-loads sub-levels)
levelManager->LoadLevel("FPS_Arena");

// Unload parent (auto-unloads sub-levels)
levelManager->UnloadLevel("FPS_Arena");

// Manual sub-level control
levelManager->LoadSubLevels("FPS_Arena");
levelManager->UnloadSubLevels("FPS_Arena");
```

## Level Transitions

Seamless travel between levels:

```cpp
LevelTransition transition;
strncpy(transition.fromLevel, "MainMenu", 64);
strncpy(transition.toLevel, "FPS_Arena", 64);
transition.transitionDuration = 2.0f;
transition.fadeOutDuration = 0.5f;
transition.fadeInDuration = 0.5f;
transition.keepPersistentLevel = true;
transition.keepPlayerState = true;
transition.spawnPosition[0] = 0;
transition.spawnPosition[1] = 1;
transition.spawnPosition[2] = 0;

levelManager->TransitionToLevel("FPS_Arena", transition);
```

## Entity Management

Track which level entities belong to:

```cpp
// Add entity to level
levelManager->AddEntityToLevel(entityId, "FPS_Arena");

// Get entity's level
const char* levelName = levelManager->GetEntityLevel(entityId);

// Remove entity from level
levelManager->RemoveEntityFromLevel(entityId);
```

**Automatic cleanup**: When level unloads, all entities are destroyed.

## Load Priorities

Control loading order:

```cpp
// Immediate: Block until loaded
levelManager->LoadLevel("MainMenu", LoadPriority::Immediate);

// High: Load ASAP (async)
levelManager->LoadLevel("FPS_Arena", LoadPriority::High);

// Normal: Load when convenient
levelManager->LoadLevel("OpenWorld_Zone1", LoadPriority::Normal);

// Low: Background loading
levelManager->LoadLevel("OpenWorld_Zone2", LoadPriority::Low);

// Lazy: Load only when needed
levelManager->LoadLevel("Cutscene", LoadPriority::Lazy);
```

## Example Use Cases

### FPS Game
```json
{
  "levels": [
    {
      "name": "MainMenu",
      "type": "persistent",
      "autoLoad": true
    },
    {
      "name": "FPS_Arena",
      "type": "streaming",
      "streamingMethod": "manual",
      "blockOnLoad": true
    }
  ]
}
```

```cpp
// Start game
levelManager->LoadLevel("FPS_Arena", LoadPriority::Immediate);
levelManager->ShowLevel("FPS_Arena");
levelManager->HideLevel("MainMenu");
```

### Open World Game
```json
{
  "levels": [
    {
      "name": "OpenWorld_Zone1",
      "streamingMethod": "distance",
      "streamingDistance": 2000.0
    },
    {
      "name": "OpenWorld_Zone2",
      "streamingMethod": "distance",
      "streamingDistance": 2000.0
    },
    {
      "name": "OpenWorld_Zone3",
      "streamingMethod": "distance",
      "streamingDistance": 2000.0
    }
  ]
}
```

**Automatic streaming** as player moves!

### Dungeon Crawler
```json
{
  "levels": [
    {
      "name": "Overworld",
      "type": "persistent"
    },
    {
      "name": "Dungeon_Floor1",
      "streamingMethod": "volume"
    },
    {
      "name": "Dungeon_Floor2",
      "streamingMethod": "volume"
    }
  ]
}
```

```cpp
// Register entrance volume
LevelStreamingVolume entrance;
strncpy(entrance.levelName, "Dungeon_Floor1", 64);
entrance.bounds.center[0] = 100;
entrance.bounds.extents[0] = 10;
entrance.loadOnEnter = true;
entrance.blockOnLoad = true;

levelManager->RegisterStreamingVolume(entrance);
```

## Debug & Utilities

```cpp
// Print all levels
levelManager->PrintLevelInfo();

// Get loaded levels
char levelNames[32][64];
int count = 0;
levelManager->GetLoadedLevels(levelNames, count, 32);

// Get level count
int loadedCount = levelManager->GetLoadedLevelCount();

// Check if transitioning
if (levelManager->IsTransitioning()) {
    float progress = levelManager->GetTransitionProgress();
}
```

## Architecture

```
LevelSystem Plugin
  └─> LevelManager
        ├─> Level Definitions (JSON)
        ├─> Level Instances (runtime)
        ├─> Streaming Volumes
        ├─> Distance Streaming
        ├─> Volume Streaming
        └─> Level Transitions
```

## Comparison to Unreal

| Feature | Unreal Engine | SecretEngine |
|---------|---------------|--------------|
| Persistent Level | ✅ | ✅ |
| Streaming Levels | ✅ | ✅ |
| Sub-Levels | ✅ | ✅ |
| Distance Streaming | ✅ | ✅ |
| Volume Streaming | ✅ | ✅ |
| Level Transitions | ✅ | ✅ |
| Async Loading | ✅ | ✅ (simplified) |
| World Composition | ✅ | ❌ (future) |
| Level Instances | ✅ | ❌ (future) |

## Benefits

✅ **Unreal-like workflow** - Familiar to Unreal developers
✅ **Automatic streaming** - Distance and volume-based
✅ **Sub-levels** - Organize large levels
✅ **Persistent level** - Always-loaded content
✅ **Seamless transitions** - Smooth level changes
✅ **Entity tracking** - Know which level entities belong to
✅ **JSON-driven** - No hardcoding
✅ **Mobile-optimized** - Efficient memory management

## Next Steps

1. **Create level files** (JSON format for level data)
2. **Implement level serialization** (save/load level state)
3. **Add level instances** (multiple instances of same level)
4. **Add world composition** (large world management)
5. **Add level blueprints** (level-specific logic)

## Files

- `plugins/LevelSystem/src/LevelTypes.h` - Level data structures
- `plugins/LevelSystem/src/LevelManager.h` - Level management API
- `plugins/LevelSystem/src/LevelManager.cpp` - Implementation
- `plugins/LevelSystem/src/LevelSystemPlugin.h/cpp` - Plugin wrapper
- `data/LevelDefinitions.json` - Level definitions
