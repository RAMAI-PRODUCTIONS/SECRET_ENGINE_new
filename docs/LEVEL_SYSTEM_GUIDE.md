# Level System Guide

## Overview
The level system supports JSON-based level definitions with chunk-based streaming, trigger systems, and seamless level transitions.

## Level Structure

### Level Definition (levelone.json, leveltwo.json)
```json
{
  "version": "7.3",
  "name": "levelone",
  "core": {
    "transform_settings": {
      "rotation_format": "euler",
      "rotation_unit": "degrees",
      "rotation_order": "YXZ"
    }
  },
  "references": {
    "players": "entities/levelone_players.json",
    "chunks": ["chunks/levelone_chunk.json"]
  },
  "streaming": {
    "chunk_size": 200.0,
    "load_radius": 2,
    "unload_radius": 3,
    "method": "distance"
  }
}
```

### Chunk Definition (levelone_chunk.json)
Chunks contain:
- Ground planes for player movement
- Random object placement using existing meshes
- Trigger volumes for level switching
- LOD settings for performance

#### Ground Plane
```json
{
  "mesh": "cube.meshbin",
  "material": "ground.mat",
  "instances": [{
    "transform": {
      "position": [0, -1, 0],
      "rotation": [0, 0, 0],
      "scale": [100, 1, 100]
    },
    "tags": {"type": "ground"},
    "material_params": {"color": [0.3, 0.5, 0.3]}
  }]
}
```

#### Random Objects
Objects are placed at various positions with different colors and rotations:
```json
{
  "mesh": "Character.meshbin",
  "instances": [
    {
      "transform": {
        "position": [15, 0, 10],
        "rotation": [0, 45, 0],
        "scale": [5, 5, 5]
      },
      "material_params": {"color": [0.8, 0.2, 0.2]},
      "lod": {
        "levels": [
          {"distance": 25},
          {"distance": 75},
          {"distance": 150}
        ]
      }
    }
  ]
}
```

### Triggers for Level Switching

#### Visual Trigger (Highlighted Cube)
```json
{
  "mesh": "cube.meshbin",
  "instances": [{
    "transform": {
      "position": [40, 5, 0],
      "scale": [3, 10, 3]
    },
    "tags": {
      "type": "trigger",
      "target_level": "leveltwo"
    },
    "material_params": {"color": [1.0, 1.0, 0.0]}
  }]
}
```

#### Trigger Logic
```json
{
  "triggers": [{
    "id": "switch_to_leveltwo",
    "type": "area",
    "shape": "box",
    "position": [40, 5, 0],
    "radius": 5.0,
    "enter_actions": [{
      "type": "load_level",
      "target_level": "leveltwo",
      "delay": 0.5
    }],
    "repeatable": false
  }]
}
```

## Current Levels

### Level One
- Ground: Green plane (100x100)
- Objects: 6 Character meshes in various colors
- Trigger: Yellow cube at position [40, 5, 0] → switches to Level Two
- Player spawn: [0, 2, 0] facing forward

### Level Two
- Ground: Purple plane (100x100)
- Objects: 8 Character meshes in different positions/colors
- Trigger: Cyan cube at position [-40, 5, 0] → switches to Level One
- Player spawn: [0, 2, 0] facing backward

## Testing

1. Load levelone:
   ```cpp
   levelManager->LoadLevel("levels/levelone.json");
   ```

2. Walk to the yellow trigger cube (position [40, 5, 0])

3. Level automatically switches to leveltwo

4. Walk to the cyan trigger cube (position [-40, 5, 0])

5. Level switches back to levelone

## Shader Compatibility

The system uses existing shaders:
- `mega_geometry_vert.spv` / `mega_geometry_frag.spv` for meshes
- Supports per-instance colors via material_params
- LOD system for performance optimization

## Code Integration

### TriggerSystem
- Checks player position against trigger volumes
- Executes actions when player enters/exits
- Supports cooldowns and repeatable triggers

### V73LevelManager
- Loads levels and chunks from JSON
- Initializes trigger system
- Updates triggers each frame
- Handles level transitions

## Future Enhancements

- Add more trigger types (proximity, interaction)
- Implement trigger animations
- Add sound effects on trigger activation
- Support for multiple players
- Network synchronization for multiplayer
