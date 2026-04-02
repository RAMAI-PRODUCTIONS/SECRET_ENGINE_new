# Quick Start Guide

## Installation (Windows)

1. Double-click `install.bat`
2. Open Blender
3. Go to `Edit > Preferences > Add-ons`
4. Search for "Secret Engine"
5. Enable the addon

## Installation (macOS/Linux)

1. Run `chmod +x install.sh && ./install.sh`
2. Open Blender
3. Go to `Edit > Preferences > Add-ons`
4. Search for "Secret Engine"
5. Enable the addon

## First Level Chunk in 5 Minutes

### 1. Create Ground Plane
- Press `Shift+A` > Mesh > Cube
- Press `S` then type `100` > `Enter` (scale to 100)
- Press `S` then `Z` then `0.01` > `Enter` (flatten)
- Press `N` to open sidebar > Go to "Secret Engine" tab
- Set:
  - Mesh: `cube.meshbin`
  - Material: `ground.mat`
  - Type: `ground`
  - Name: `ground_plane`
  - Culling Radius: `100`

### 2. Add Some Objects
- Press `Shift+A` > Mesh > Cube
- Press `G` to move it around
- In Secret Engine panel:
  - Mesh: `Character.meshbin`
  - Material: `default.mat`
  - Type: `mesh`
  - Name: `object_1`
  - Enable "Use Custom Color" and pick a color
  - Culling Radius: `5`

### 3. Duplicate Objects
- Select the object
- Press `Shift+D` to duplicate
- Move it to a new position
- Change the name in Secret Engine panel to `object_2`
- Change the color

### 4. Export
- In Chunk Settings, set Chunk ID: `my_first_chunk`
- Click "Export Chunk"
- Save to your engine's assets folder

### 5. Test in Engine
- Copy the exported JSON to `Assets/levels/chunks/`
- Reference it in your level JSON
- Run your engine!

## Common Tasks

### Import Existing Chunk
1. Click "Import Chunk"
2. Select your chunk JSON
3. Edit in Blender
4. Export when done

### Add Trigger Zone
1. Add a cube where you want the trigger
2. Set:
   - Mesh: `cube.meshbin`
   - Material: `trigger.mat`
   - Type: `trigger`
   - Name: `my_trigger`
3. Click "Add Tag" under Custom Tags
4. Set key: `target_level`, value: `leveltwo`

### Add LOD to Object
1. Select object
2. Enable "Use LOD"
3. Click "Add LOD Level" three times
4. Set distances: 25, 75, 150

### Batch Edit Multiple Objects
1. Select all objects you want to edit
2. Use Blender's built-in batch operations
3. For Secret Engine properties, you'll need to edit each individually (or use a Python script)

## Tips

- Use Blender collections to organize objects by type
- Name objects descriptively - it becomes the default tag name
- Use array modifiers for repeating patterns
- Export often and test in your engine
- Keep culling radius slightly larger than the object's visual size

## Keyboard Shortcuts (Blender)

- `Shift+A`: Add object
- `G`: Move (grab)
- `R`: Rotate
- `S`: Scale
- `Shift+D`: Duplicate
- `X`: Delete
- `N`: Toggle sidebar
- `Tab`: Edit mode
- `Ctrl+S`: Save

## Next Steps

- Read the full README.md for detailed documentation
- Check out example_batch_operations.py for automation scripts
- Experiment with different mesh and material combinations
- Create your own level layouts!
