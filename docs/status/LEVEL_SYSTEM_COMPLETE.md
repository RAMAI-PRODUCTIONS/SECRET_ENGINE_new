# Level System - Complete ✅

## What Was Created

### Unreal Engine-like Level Management System
Complete level streaming, persistent levels, and sub-levels inspired by Unreal Engine.

## Features Implemented

### ✅ Level Types
- **Persistent Level**: Always loaded (never unloads)
- **Streaming Level**: Dynamically loaded/unloaded
- **Sub-Level**: Part of a larger level

### ✅ Streaming Methods
- **Distance-based**: Auto-load when player within distance
- **Volume-based**: Load when player enters trigger volume
- **Manual**: Load via code
- **Always**: Always loaded

### ✅ Level States
- Unloaded → Loading → Loaded → Visible → Unloading

### ✅ Level Management
- Load/unload levels
- Show/hide levels (visibility control)
- Sub-level hierarchies
- Entity tracking per level
- Level transitions

### ✅ Automatic Streaming
- Distance-based streaming (player proximity)
- Volume-based streaming (trigger volumes)
- Configurable streaming distances
- Automatic load/unload

## Files Created

```
plugins/LevelSystem/
├── src/
│   ├── LevelTypes.h              ← Level data structures
│   ├── LevelManager.h            ← Level management API
│   ├── LevelManager.cpp          ← Implementation
│   ├── LevelSystemPlugin.h       ← Plugin wrapper
│   └── LevelSystemPlugin.cpp     ← Plugin implementation
├── CMakeLists.txt
└── plugin_manifest.json

data/
└── LevelDefinitions.json         ← Level definitions

docs/
└── LEVEL_SYSTEM.md               ← Complete documentation
```

## Level Definitions Example

```json
{
  "levels": [
    {
      "name": "MainMenu",
      "type": "persistent",
      "streamingMethod": "always",
      "autoLoad": true
    },
    {
      "name": "FPS_Arena",
      "type": "streaming",
      "streamingMethod": "manual",
      "blockOnLoad": true,
      "subLevels": ["FPS_Arena_Lighting", "FPS_Arena_Audio"]
    },
    {
      "name": "OpenWorld_Zone1",
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

## Usage Examples

### Load a Level
```cpp
auto* levelPlugin = static_cast<Levels::LevelSystemPlugin*>(
    core->GetCapability("level_system")
);
auto* levelManager = levelPlugin->GetLevelManager();

// Load level
levelManager->LoadLevel("FPS_Arena", LoadPriority::Immediate);

// Load with sub-levels
levelManager->LoadLevel("FPS_Arena");  // Auto-loads sub-levels
```

### Unload a Level
```cpp
levelManager->UnloadLevel("FPS_Arena");
```

### Show/Hide Levels
```cpp
// Hide (keep in memory, stop rendering)
levelManager->HideLevel("FPS_Arena");

// Show (start rendering)
levelManager->ShowLevel("FPS_Arena");
```

### Set Persistent Level
```cpp
// Never unloads
levelManager->SetPersistentLevel("MainMenu");
```

### Distance-Based Streaming
```json
{
  "name": "OpenWorld_Zone1",
  "streamingMethod": "distance",
  "streamingDistance": 2000.0
}
```

**Automatic**: Loads when player within 2000 units, unloads when > 3000 units.

### Volume-Based Streaming
```cpp
LevelStreamingVolume volume;
strncpy(volume.levelName, "Dungeon_Entrance", 64);
volume.bounds.center[0] = 500;
volume.bounds.extents[0] = 100;
volume.loadOnEnter = true;
volume.unloadOnExit = true;

levelManager->RegisterStreamingVolume(volume);
```

**Automatic**: Loads when player enters volume, unloads when exits.

### Sub-Levels
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

```cpp
// Load parent (auto-loads sub-levels)
levelManager->LoadLevel("FPS_Arena");

// Unload parent (auto-unloads sub-levels)
levelManager->UnloadLevel("FPS_Arena");
```

### Level Transitions
```cpp
LevelTransition transition;
strncpy(transition.fromLevel, "MainMenu", 64);
strncpy(transition.toLevel, "FPS_Arena", 64);
transition.transitionDuration = 2.0f;
transition.fadeOutDuration = 0.5f;
transition.fadeInDuration = 0.5f;
transition.keepPersistentLevel = true;

levelManager->TransitionToLevel("FPS_Arena", transition);
```

## Use Cases

### FPS Game
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
    {"name": "Zone1", "streamingMethod": "distance", "streamingDistance": 2000.0},
    {"name": "Zone2", "streamingMethod": "distance", "streamingDistance": 2000.0},
    {"name": "Zone3", "streamingMethod": "distance", "streamingDistance": 2000.0}
  ]
}
```

**Automatic streaming** as player moves!

### Dungeon Crawler
```cpp
// Register entrance volume
LevelStreamingVolume entrance;
strncpy(entrance.levelName, "Dungeon_Floor1", 64);
entrance.bounds.center[0] = 100;
entrance.loadOnEnter = true;
entrance.blockOnLoad = true;

levelManager->RegisterStreamingVolume(entrance);
```

## Integration Status

### ✅ Build System
- Added to `plugins/CMakeLists.txt`
- Linked in `CMakeLists.txt`
- Plugin manifest created

### ✅ Core Integration
- Plugin factory exported
- Loaded in `Core.cpp`
- Registered as capability "level_system"

### ✅ Build Status
- **BUILD SUCCESSFUL**
- All plugins compile
- LevelSystem loads on startup
- Ready to use

## Architecture

```
LevelSystem Plugin
  └─> LevelManager
        ├─> Level Definitions (JSON)
        │     └─> data/LevelDefinitions.json
        ├─> Level Instances (runtime)
        │     ├─> State management
        │     ├─> Entity tracking
        │     └─> Visibility control
        ├─> Streaming System
        │     ├─> Distance-based
        │     ├─> Volume-based
        │     └─> Manual
        └─> Level Transitions
              ├─> Fade in/out
              ├─> Player state
              └─> Spawn points
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
| Load Priorities | ✅ | ✅ |
| Entity Tracking | ✅ | ✅ |
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
✅ **Load priorities** - Control loading order
✅ **Visibility control** - Hide/show without unloading

## Next Steps

### 1. Create Level Files
Create actual level data files (JSON format):
```json
{
  "name": "FPS_Arena",
  "entities": [
    {
      "type": "StaticMesh",
      "position": [0, 0, 0],
      "mesh": "arena_floor.gltf"
    }
  ]
}
```

### 2. Implement Level Serialization
- Save level state
- Load level state
- Entity persistence

### 3. Add Level Instances
- Multiple instances of same level
- Instance-specific data
- Instance management

### 4. Add World Composition
- Large world management
- Tile-based streaming
- World origin rebasing

### 5. Integrate with FPS Game
```cpp
// In FPSGamePlugin::OnActivate()
auto* levelPlugin = static_cast<Levels::LevelSystemPlugin*>(
    m_core->GetCapability("level_system")
);
auto* levelManager = levelPlugin->GetLevelManager();

// Load FPS arena
levelManager->LoadLevel("FPS_Arena", LoadPriority::Immediate);
```

## Documentation

- **Complete Guide**: `docs/LEVEL_SYSTEM.md`
- **Level Definitions**: `data/LevelDefinitions.json`
- **API Reference**: `plugins/LevelSystem/src/LevelManager.h`

## Summary

✅ **Level System fully implemented**
✅ **Unreal Engine-like features**
✅ **Automatic streaming (distance + volume)**
✅ **Sub-level support**
✅ **Persistent level support**
✅ **Level transitions**
✅ **Entity tracking**
✅ **JSON-driven**
✅ **Build successful**
✅ **Ready to use**

**You can now create complex level streaming setups just like Unreal Engine!**
