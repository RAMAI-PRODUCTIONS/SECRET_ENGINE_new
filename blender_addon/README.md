# Secret Engine Level Editor - Blender Addon

A Blender addon for creating and editing level chunks for the Secret Engine.

## Features

- **Visual Level Design**: Create level layouts directly in Blender's 3D viewport
- **Instance Management**: Automatically groups objects by mesh and material
- **Tag System**: Add custom tags to objects (type, name, and custom key-value pairs)
- **Material Parameters**: Override material colors per instance
- **Culling Radius**: Set frustum culling radius for each object
- **LOD Support**: Configure Level of Detail distances
- **Import/Export**: Full JSON import/export compatible with Secret Engine format
- **Coordinate Conversion**: Automatic conversion between Blender and engine coordinate systems

## Installation

1. Copy the `secret_engine_level_editor` folder to your Blender addons directory:
   - **Windows**: `%APPDATA%\Blender Foundation\Blender\<version>\scripts\addons\`
   - **macOS**: `~/Library/Application Support/Blender/<version>/scripts/addons/`
   - **Linux**: `~/.config/blender/<version>/scripts/addons/`

2. Open Blender and go to `Edit > Preferences > Add-ons`

3. Search for "Secret Engine Level Editor"

4. Enable the addon by checking the checkbox

## Usage

### Creating a New Level Chunk

1. Open Blender and create your level geometry using standard Blender objects

2. Open the sidebar (press `N` if not visible) and navigate to the **Secret Engine** tab

3. For each object you want to export:
   - Select the object
   - In the **Object Properties** section, configure:
     - **Mesh**: The mesh file name (e.g., `cube.meshbin`, `Character.meshbin`)
     - **Material**: The material file name (e.g., `default.mat`, `ground.mat`)
     - **Tags**: Set type and name, add custom tags as needed
     - **Material Parameters**: Enable custom color if needed
     - **Culling Radius**: Set the frustum culling radius
     - **LOD**: Enable and configure LOD levels if needed

4. In the **Chunk Settings** section, set the **Chunk ID** (e.g., `levelone_0`)

5. Click **Export Chunk** and choose where to save the JSON file

### Importing an Existing Chunk

1. Click **Import Chunk** in the Secret Engine panel

2. Select your chunk JSON file

3. The addon will create placeholder cubes for each instance with all properties preserved

4. You can now edit positions, rotations, scales, and properties visually

5. Export when done to save your changes

### Object Properties

#### Mesh & Material
- **Mesh**: Reference to the mesh file (e.g., `cube.meshbin`)
- **Material**: Reference to the material file (e.g., `default.mat`)

#### Tags
- **Type**: Object type (e.g., `mesh`, `ground`, `trigger`, `cube`)
- **Name**: Object name (defaults to Blender object name)
- **Custom Tags**: Add any key-value pairs (e.g., `target_level: leveltwo`)

#### Material Parameters
- **Use Custom Color**: Override the material's default color
- **Color**: RGB color values (0.0 - 1.0)

#### Culling
- **Culling Radius**: Frustum culling sphere radius

#### Level of Detail (LOD)
- **Use LOD**: Enable LOD system
- **LOD Levels**: Add multiple distance thresholds for LOD switching

### Coordinate System

The addon automatically converts between Blender's coordinate system and the engine's:
- **Blender**: X-right, Y-forward, Z-up
- **Engine**: X-right, Y-up, Z-back

Rotations are converted from radians (Blender) to degrees (engine).

### Tips

1. **Naming Convention**: Use descriptive names for objects - they'll be used as the default tag name

2. **Grouping**: Objects with the same mesh and material are automatically grouped in the export

3. **Instancing**: Use Blender's array modifiers or duplicate objects to create multiple instances efficiently

4. **Organization**: Use Blender collections to organize different types of objects (ground, props, triggers, etc.)

5. **Testing**: Export frequently and test in your engine to verify the layout

## Example Workflow

1. Create a ground plane:
   - Add a cube, scale it to `(100, 100, 1)`
   - Set mesh to `cube.meshbin`, material to `ground.mat`
   - Set type to `ground`, name to `ground_plane`
   - Set culling radius to `100.0`

2. Add some props:
   - Add cubes or other objects
   - Set mesh to `Character.meshbin`, material to `default.mat`
   - Set type to `mesh`, give each a unique name
   - Enable custom color and set different colors
   - Set culling radius to `5.0`
   - Enable LOD with levels at 25, 75, 150

3. Add a trigger:
   - Add a cube at the desired location
   - Set mesh to `cube.meshbin`, material to `trigger.mat`
   - Set type to `trigger`, name to `level_switch_trigger`
   - Add custom tag: `target_level` = `leveltwo`

4. Export the chunk to your engine's assets folder

## Troubleshooting

**Objects not appearing in export:**
- Make sure objects are mesh type (not cameras, lights, etc.)
- Check that mesh and material names are set

**Wrong positions/rotations:**
- The addon converts coordinates automatically
- If something looks wrong, check the transform in Blender

**Tags not working:**
- Ensure tag keys don't have spaces
- Check that required tags (type, name) are set

## File Format

The addon exports JSON in this format:

```json
{
  "chunk_id": "levelone_0",
  "mesh_groups": [
    {
      "mesh": "cube.meshbin",
      "material": "ground.mat",
      "instances": [
        {
          "transform": {
            "position": [0, -1, 0],
            "rotation": [0, 0, 0],
            "scale": [100, 1, 100]
          },
          "tags": {
            "type": "ground",
            "name": "ground_plane"
          },
          "material_params": {
            "color": [0.3, 0.5, 0.3]
          },
          "culling": {
            "radius": 100.0
          }
        }
      ]
    }
  ]
}
```

## Version History

- **1.0.0**: Initial release
  - Export/Import chunk JSON
  - Tag system
  - Material parameters
  - Culling radius
  - LOD support
  - Coordinate conversion

## License

This addon is part of the Secret Engine project.
