# Blender to SecretEngine Level Workflow

## Overview
This guide explains how to use Blender as a level editor for SecretEngine by exporting scenes to the engine's JSON format.

## SecretEngine Level JSON Format

### Basic Structure
```json
{
  "version": "1.0",
  "entities": [
    {
      "name": "EntityName",
      "transform": {
        "position": [x, y, z],
        "rotation": [x, y, z],
        "scale": [x, y, z]
      },
      "mesh": "meshes/mesh_name.meshbin",
      "texture": "textures/texture_name.jpeg",
      "color": [r, g, b, a],
      "tags": ["tag1", "tag2"]
    }
  ]
}
```

### Field Descriptions
- **name**: Unique identifier for the entity (taken from Blender object name)
- **transform**: Object's position, rotation (Euler angles in degrees), and scale
- **mesh**: Path to the mesh binary file (relative to assets folder)
- **texture**: (Optional) Path to texture file
- **color**: (Optional) RGBA color values (0.0 to 1.0)
- **tags**: (Optional) Gameplay tags for the entity

## Blender Export Workflow

### Method 1: Manual Export (Simple Scenes)

For small scenes with few objects, you can manually create the JSON:

1. **Select objects** in Blender
2. **Note their properties**:
   - Location (X, Y, Z)
   - Rotation (X, Y, Z in degrees)
   - Scale (X, Y, Z)
3. **Create JSON file** with the structure above
4. **Copy to assets folder**: `android/app/src/main/assets/`

### Method 2: Python Export Script (Recommended)

Use this Blender Python script to automatically export your scene:

```python
import bpy
import json
import math

def export_scene_to_json(filepath):
    """Export Blender scene to SecretEngine JSON format"""
    
    scene_data = {
        "version": "1.0",
        "entities": []
    }
    
    # Iterate through all mesh objects in the scene
    for obj in bpy.context.scene.objects:
        if obj.type != 'MESH':
            continue
            
        # Skip objects with names starting with underscore (hidden/utility objects)
        if obj.name.startswith('_'):
            continue
        
        # Get transform data
        location = obj.location
        rotation = obj.rotation_euler
        scale = obj.scale
        
        # Convert rotation from radians to degrees
        rotation_deg = [math.degrees(r) for r in rotation]
        
        # Build entity data (Z-up to Z-up, direct mapping)
        entity = {
            "name": obj.name,
            "transform": {
                "position": [location.x, location.y, location.z],
                "rotation": [rotation_deg.x, rotation_deg.y, rotation_deg.z],
                "scale": [scale.x, scale.y, scale.z]
            }
        }
        
        # Determine mesh path
        # Convention: mesh name matches object name or use custom property
        if "mesh_path" in obj:
            entity["mesh"] = obj["mesh_path"]
        else:
            # Default: use object name
            entity["mesh"] = f"meshes/{obj.name}.meshbin"
        
        # Check for texture
        if obj.active_material and obj.active_material.use_nodes:
            for node in obj.active_material.node_tree.nodes:
                if node.type == 'TEX_IMAGE' and node.image:
                    texture_name = node.image.name
                    entity["texture"] = f"textures/{texture_name}"
                    break
        
        # Check for color override (use material base color)
        if obj.active_material:
            mat = obj.active_material
            if mat.use_nodes:
                # Get Principled BSDF base color
                for node in mat.node_tree.nodes:
                    if node.type == 'BSDF_PRINCIPLED':
                        color = node.inputs['Base Color'].default_value
                        entity["color"] = [color[0], color[1], color[2], color[3]]
                        break
        
        # Check for gameplay tags (custom property)
        if "tags" in obj:
            tags_str = obj["tags"]
            entity["tags"] = [tag.strip() for tag in tags_str.split(',')]
        
        scene_data["entities"].append(entity)
    
    # Write to file
    with open(filepath, 'w') as f:
        json.dump(scene_data, f, indent=2)
    
    print(f"Exported {len(scene_data['entities'])} entities to {filepath}")

# Run the export
export_scene_to_json("/path/to/your/level.json")
```

### Method 3: Blender Add-on (Advanced)

For production workflows, create a Blender add-on:

1. **Create add-on file**: `secretengine_exporter.py`
2. **Add UI panel** in Blender's 3D viewport
3. **Add export button** that runs the export script
4. **Add validation** to check for missing meshes/textures

## Coordinate System Conversion

### Blender vs SecretEngine Coordinates

Both Blender and SecretEngine use Z-up, right-handed coordinates.

**Coordinate System**:
- **X-axis**: Right (positive X = right, negative X = left)
- **Y-axis**: Forward (positive Y = forward, negative Y = backward)
- **Z-axis**: Up (positive Z = up, negative Z = down)

**Conversion**:
Since both use Z-up, the conversion is straightforward:
- Blender X → Engine X
- Blender Y → Engine Y
- Blender Z → Engine Z

**In Python**:
```python
# Position (direct mapping)
engine_pos = [blender_x, blender_y, blender_z]

# Rotation (direct mapping, degrees)
engine_rot = [blender_rot_x, blender_rot_y, blender_rot_z]

# Scale (direct mapping)
engine_scale = [blender_scale_x, blender_scale_y, blender_scale_z]
```

## Naming Conventions

### Object Names
- Use descriptive names: `Platform_01`, `Enemy_Spawn_A`, `Wall_North`
- Avoid spaces (use underscores)
- Prefix utility objects with `_` to exclude from export

### Mesh References
- Store mesh path in custom property: `mesh_path = "meshes/cube.meshbin"`
- Or use naming convention: object `Platform_01` → `meshes/Platform_01.meshbin`

### Texture References
- Use material's image texture node
- Texture name should match file in `textures/` folder

## Custom Properties in Blender

Add custom properties to objects for engine-specific data:

### Adding Custom Properties
1. Select object
2. Go to Object Properties panel
3. Scroll to Custom Properties
4. Click "Add" or "Edit"

### Useful Custom Properties
- `mesh_path` (string): Override mesh file path
- `tags` (string): Comma-separated gameplay tags (e.g., "enemy,spawner,ground")
- `collision_mesh` (string): Path to collision mesh
- `layer` (int): Rendering layer
- `physics_type` (string): "static", "dynamic", "kinematic"

## Workflow Steps

### 1. Scene Setup in Blender
```
1. Create your level geometry
2. Add materials and textures
3. Set object transforms (position, rotation, scale)
4. Add custom properties for engine-specific data
5. Name objects descriptively
```

### 2. Export Scene
```
1. Run the Python export script
2. Save JSON to project folder
3. Validate JSON structure
```

### 3. Copy to Android Assets
```bash
# Copy level JSON
cp level_name.json android/app/src/main/assets/

# Update LevelDefinitions.json
# Add entry for new level
```

### 4. Rebuild APK
```bash
cd android
./gradlew clean
./gradlew assembleDebug
```

### 5. Test on Device
```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb logcat | grep SecretEngine
```

## Example: Creating a Simple Arena

### In Blender:
1. Create ground plane (scale 50x50)
2. Add walls (4 cubes, scaled and positioned)
3. Add platforms (cubes at various heights)
4. Add spawn points (empties with custom property `tags = "player_spawn"`)
5. Set materials/colors

### Export:
```python
export_scene_to_json("C:/SecretEngine/Assets/my_arena.json")
```

### Result JSON:
```json
{
  "version": "1.0",
  "entities": [
    {
      "name": "Ground",
      "transform": {
        "position": [0, 0, 0],
        "rotation": [0, 0, 0],
        "scale": [50, 1, 50]
      },
      "mesh": "meshes/cube.meshbin",
      "color": [0.3, 0.5, 0.3, 1.0]
    },
    {
      "name": "Platform_01",
      "transform": {
        "position": [10, 2, 5],
        "rotation": [0, 0, 0],
        "scale": [5, 0.5, 5]
      },
      "mesh": "meshes/cube.meshbin",
      "color": [0.8, 0.3, 0.2, 1.0]
    }
  ]
}
```

## Tips and Best Practices

### Performance
- Keep entity count reasonable (< 1000 per level)
- Use instancing for repeated objects
- Combine static geometry where possible

### Organization
- Use Blender collections to organize level sections
- Name collections by zone/area
- Use layers for different object types

### Iteration
- Keep Blender file and JSON in sync
- Use version control (Git) for both
- Test frequently on device

### Debugging
- Check logcat for loading errors
- Verify mesh paths exist
- Validate JSON syntax

## Automation

### Batch Export Multiple Levels
```python
import bpy
import os

levels = [
    ("Scene_MainMenu", "main_menu.json"),
    ("Scene_Arena", "fps_arena.json"),
    ("Scene_Racing", "racing_track.json")
]

for scene_name, output_file in levels:
    bpy.context.window.scene = bpy.data.scenes[scene_name]
    export_scene_to_json(f"C:/SecretEngine/Assets/{output_file}")
```

### Auto-Copy to Assets
```python
import shutil

def export_and_copy(blender_scene, json_name):
    # Export
    temp_path = f"/tmp/{json_name}"
    export_scene_to_json(temp_path)
    
    # Copy to assets
    dest_path = f"C:/SecretEngine/android/app/src/main/assets/{json_name}"
    shutil.copy(temp_path, dest_path)
    print(f"Copied to {dest_path}")
```

## Troubleshooting

### Objects Not Appearing
- Check mesh path is correct
- Verify mesh file exists in assets
- Check object scale (not zero)
- Verify position is within camera view

### Wrong Orientation
- Double-check coordinate conversion
- Apply rotation in Blender before export
- Check rotation mode (Euler XYZ)

### Missing Textures
- Verify texture path in JSON
- Check texture file exists in assets folder
- Ensure texture is assigned in Blender material

### Performance Issues
- Reduce entity count
- Simplify collision meshes
- Use LOD (Level of Detail) system

## Related Documentation
- [Collision Meshes in Blender](BLENDER_COLLISION_MESHES.md)
- [Level System](LEVEL_SYSTEM.md)
- [Gameplay Tag System](GAMEPLAY_TAG_SYSTEM.md)

## Next Steps
1. Install Blender (if not already installed)
2. Create a test scene
3. Run the export script
4. Test in SecretEngine
5. Iterate and refine workflow
