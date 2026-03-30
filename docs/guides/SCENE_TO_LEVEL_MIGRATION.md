# Scene.json to Level System Migration

## Overview
The Level System now supports loading `scene.json` files as levels, providing level management features while maintaining compatibility with existing scene files.

## What Changed

### Before (Direct scene.json loading)
```cpp
// Renderer loads scene.json directly in OnActivate()
std::string sceneJson = LoadAssetAsString("scene.json");
// Parse and create entities...
```

### After (Level System)
```cpp
// Level System loads scene.json as a level
auto* levelManager = GetLevelManager();
levelManager->LoadLevel("Scene");  // Loads Assets/scene.json
```

## Benefits

✅ **Level Management**: Load/unload scenes dynamically
✅ **Entity Tracking**: Know which entities belong to which level
✅ **Streaming**: Distance and volume-based loading
✅ **Sub-Levels**: Organize large scenes
✅ **Transitions**: Seamless scene changes
✅ **Backward Compatible**: Existing scene.json files work as-is

## Configuration

### Level Definitions (`data/LevelDefinitions.json`)

```json
{
  "levels": [
    {
      "name": "Scene",
      "path": "Assets/scene.json",
      "type": "persistent",
      "streamingMethod": "always",
      "autoLoad": true,
      "makeVisibleAfterLoad": true
    }
  ]
}
```

**Key Points**:
- `name`: "Scene" - Level identifier
- `path`: "Assets/scene.json" - Points to existing scene file
- `type`: "persistent" - Always loaded
- `autoLoad`: true - Loads on startup

## Usage

### Load Scene as Level
```cpp
auto* levelPlugin = static_cast<Levels::LevelSystemPlugin*>(
    core->GetCapability("level_system")
);
auto* levelManager = levelPlugin->GetLevelManager();

// Load scene.json as a level
levelManager->LoadLevel("Scene", LoadPriority::Immediate);
```

### Check if Scene is Loaded
```cpp
if (levelManager->IsLevelLoaded("Scene")) {
    // Scene is loaded
}
```

### Unload Scene
```cpp
levelManager->UnloadLevel("Scene");
```

### Get Scene Entities
```cpp
Level* sceneLevel = levelManager->GetLevel("Scene");
if (sceneLevel) {
    for (int i = 0; i < sceneLevel->entityCount; i++) {
        uint32_t entityId = sceneLevel->entities[i];
        // Do something with entity
    }
}
```

## Scene.json Format Support

The Level System supports the full scene.json format:

```json
{
  "entities": [
    {
      "name": "PlayerStart",
      "transform": {
        "position": [0, 80, 100],
        "rotation": [-25, 0, 0],
        "scale": [1, 1, 1]
      },
      "isPlayerStart": true
    },
    {
      "name": "Character",
      "transform": {
        "position": [0, 0, 0],
        "rotation": [0, 90, 0],
        "scale": [15, 15, 15]
      },
      "mesh": {
        "path": "meshes/Character.meshbin",
        "color": [1.0, 0.5, 0.3, 1.0],
        "texture": "textures/diffuse.jpeg",
        "normalMap": "textures/NormalMap.png"
      }
    }
  ]
}
```

**Supported Fields**:
- `name`: Entity name
- `transform`: Position, rotation, scale
- `mesh`: Mesh path, color, texture, normal map
- `isPlayerStart`: Player spawn point
- `isPlayer`: Player entity flag

## Level Components

Each entity loaded from scene.json gets a `LevelComponent`:

```cpp
struct LevelComponent {
    char levelName[64];  // "Scene"
    int levelIndex;      // Index in level manager
    bool isPersistent;   // true for persistent levels
};
```

**Usage**:
```cpp
auto* levelComp = static_cast<LevelComponent*>(
    world->GetComponent(entity, LevelComponent::TypeID)
);

if (levelComp) {
    // Know which level this entity belongs to
    const char* levelName = levelComp->levelName;
}
```

## Migration Steps

### 1. Keep Existing scene.json
No changes needed! Your existing `Assets/scene.json` works as-is.

### 2. Add Level Definition
Add to `data/LevelDefinitions.json`:
```json
{
  "name": "Scene",
  "path": "Assets/scene.json",
  "type": "persistent",
  "autoLoad": true
}
```

### 3. Load via Level System
```cpp
// Instead of renderer loading directly
levelManager->LoadLevel("Scene");
```

### 4. (Optional) Disable Renderer Auto-Load
If you want Level System to handle loading, disable renderer's auto-load:
```cpp
// In RendererPlugin::OnActivate()
// Comment out or remove scene.json loading code
```

## Advanced Usage

### Multiple Scenes
```json
{
  "levels": [
    {
      "name": "MainScene",
      "path": "Assets/scene.json",
      "type": "persistent",
      "autoLoad": true
    },
    {
      "name": "BattleArena",
      "path": "Assets/battle_arena.json",
      "type": "streaming",
      "autoLoad": false
    }
  ]
}
```

```cpp
// Switch scenes
levelManager->UnloadLevel("MainScene");
levelManager->LoadLevel("BattleArena");
```

### Scene Streaming
```json
{
  "name": "OpenWorld",
  "path": "Assets/openworld.json",
  "streamingMethod": "distance",
  "streamingDistance": 2000.0
}
```

**Automatic loading** when player gets close!

### Scene Sub-Levels
```json
{
  "name": "MainScene",
  "path": "Assets/scene.json",
  "subLevels": ["Lighting", "Audio", "Props"]
}
```

Organize large scenes into smaller pieces.

## Example: FPS Game

### Level Definitions
```json
{
  "levels": [
    {
      "name": "MainMenu",
      "path": "Assets/main_menu.json",
      "type": "persistent",
      "autoLoad": true
    },
    {
      "name": "FPS_Arena",
      "path": "Assets/fps_arena.json",
      "type": "streaming",
      "autoLoad": false
    }
  ]
}
```

### Game Flow
```cpp
// Start game
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

## Debugging

### Print Level Info
```cpp
levelManager->PrintLevelInfo();
```

**Output**:
```
=== Level System Info ===
Total levels: 1
Streaming enabled: yes
  [0] Scene - State: 3, Entities: 100
```

### Check Entity's Level
```cpp
auto* levelComp = static_cast<LevelComponent*>(
    world->GetComponent(entity, LevelComponent::TypeID)
);

if (levelComp) {
    printf("Entity belongs to level: %s\n", levelComp->levelName);
}
```

## Performance

### Memory Management
- **Loaded**: Entities in memory, not rendering
- **Visible**: Entities rendering
- **Unloaded**: Entities destroyed, memory freed

### Best Practices
1. Use `persistent` for always-needed content (UI, player)
2. Use `streaming` for large worlds
3. Use `sub-levels` to organize complex scenes
4. Unload levels when not needed to save memory

## Troubleshooting

### Scene Not Loading
```cpp
if (!levelManager->IsLevelLoaded("Scene")) {
    // Check level definitions
    // Check file path
    // Check logs
}
```

### Entities Not Rendering
```cpp
// Make sure level is visible
levelManager->ShowLevel("Scene");
```

### Duplicate Entities
If renderer still loads scene.json AND level system loads it:
- Disable one of them
- Or use different scene files

## Summary

✅ **Backward Compatible**: Existing scene.json files work
✅ **Level Management**: Load/unload dynamically
✅ **Entity Tracking**: Know which level entities belong to
✅ **Streaming**: Automatic distance/volume-based loading
✅ **Easy Migration**: Just add level definition
✅ **No Code Changes**: scene.json format unchanged

**Your existing scene.json now has full level management capabilities!**
