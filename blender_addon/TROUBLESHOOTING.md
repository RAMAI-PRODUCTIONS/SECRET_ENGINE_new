# Troubleshooting Guide

## Installation Issues

### Addon doesn't appear in preferences

**Problem**: After running the install script, the addon doesn't show up in Blender's preferences.

**Solutions**:
1. Check the installation path is correct:
   - Windows: `%APPDATA%\Blender Foundation\Blender\<version>\scripts\addons\`
   - macOS: `~/Library/Application Support/Blender/<version>/scripts/addons/`
   - Linux: `~/.config/blender/<version>/scripts/addons/`

2. Verify the folder structure:
   ```
   addons/
   └── secret_engine_level_editor/
       ├── __init__.py
       ├── properties.py
       ├── operators.py
       └── panels.py
   ```

3. Check Blender version compatibility (requires 3.0+)

4. Restart Blender completely

5. Check the Blender console for Python errors:
   - Windows: `Window > Toggle System Console`
   - macOS/Linux: Run Blender from terminal to see output

### Import errors on addon enable

**Problem**: Python errors appear when enabling the addon.

**Solutions**:
1. Make sure all four Python files are present
2. Check file permissions (should be readable)
3. Look for syntax errors in the console output
4. Try reinstalling with the install script
5. Manually copy the folder if the script fails

## Export Issues

### Export creates empty or invalid JSON

**Problem**: Exported JSON is empty or doesn't load in the engine.

**Solutions**:
1. Make sure you have mesh objects in the scene (not just cameras/lights)
2. Check that objects have mesh_name and material_name set
3. Verify the chunk_id is set in Chunk Settings
4. Check file permissions on the export location
5. Try exporting to a different location

### Objects missing from export

**Problem**: Some objects don't appear in the exported JSON.

**Solutions**:
1. Only MESH type objects are exported (not curves, empties, etc.)
2. Check object isn't hidden in viewport
3. Verify object is in the current scene
4. Make sure mesh_name and material_name are not empty
5. Check the exported JSON manually to see if it's there

### Wrong positions/rotations in engine

**Problem**: Objects appear in wrong locations or orientations.

**Solutions**:
1. The addon converts coordinates automatically (Blender Y-up to Engine Z-up)
2. Check if your engine expects a different coordinate system
3. Verify rotation order matches (addon uses YXZ)
4. Check if transforms are applied in Blender (`Ctrl+A` > All Transforms)
5. Look at the exported JSON to verify the values

## Import Issues

### Import fails with error

**Problem**: Clicking Import Chunk shows an error.

**Solutions**:
1. Verify the JSON file is valid (use a JSON validator)
2. Check the file path is accessible
3. Make sure the JSON follows the expected format
4. Look for Python errors in the console
5. Try importing a freshly exported chunk first

### Imported objects in wrong positions

**Problem**: Objects appear in different locations than expected.

**Solutions**:
1. The addon converts from engine coordinates to Blender
2. Check if the original export was from a different tool
3. Verify the coordinate system matches
4. Try exporting and re-importing to test the conversion

### Properties not imported correctly

**Problem**: Tags, colors, or other properties are missing after import.

**Solutions**:
1. Check the JSON has the expected fields
2. Verify property names match exactly (case-sensitive)
3. Look for warnings in the console
4. Some properties may have default values if missing from JSON

## UI Issues

### Secret Engine panel not visible

**Problem**: Can't find the Secret Engine panel in Blender.

**Solutions**:
1. Press `N` to show the sidebar if hidden
2. Look for the "Secret Engine" tab at the top of the sidebar
3. Make sure the addon is enabled in preferences
4. Try switching to a different workspace and back
5. Restart Blender

### Object Properties panel is empty

**Problem**: The Object Properties section shows nothing.

**Solutions**:
1. Make sure you have a mesh object selected
2. The panel only appears for MESH type objects
3. Try selecting a different object
4. Check if the object is in edit mode (switch to object mode with `Tab`)

### Can't add tags or LOD levels

**Problem**: Add buttons don't work.

**Solutions**:
1. Make sure an object is selected
2. Check the object is a mesh type
3. Look for errors in the console
4. Try restarting Blender
5. Reinstall the addon

## Property Issues

### Properties reset after closing Blender

**Problem**: Secret Engine properties don't save with the .blend file.

**Solutions**:
1. Make sure to save the .blend file (`Ctrl+S`)
2. Properties are stored in the .blend file, not externally
3. Check file permissions on the .blend file
4. Try "Save As" to a new file
5. Verify the addon is enabled when reopening

### Can't change mesh or material name

**Problem**: Text fields don't accept input.

**Solutions**:
1. Click in the field to focus it
2. Try clicking the field label first
3. Check if Blender is in the right mode (object mode)
4. Restart Blender if fields are unresponsive

### Colors don't export

**Problem**: Custom colors don't appear in the exported JSON.

**Solutions**:
1. Make sure "Use Custom Color" is enabled
2. Check the color values are set (not default)
3. Verify material_params section exists in exported JSON
4. Colors are in 0.0-1.0 range, not 0-255

## Performance Issues

### Export is very slow

**Problem**: Exporting large scenes takes a long time.

**Solutions**:
1. This is normal for scenes with thousands of objects
2. Consider splitting into multiple chunks
3. Use collections to organize and export separately
4. Remove unnecessary objects before export
5. Check if you have very complex geometry

### Blender becomes unresponsive

**Problem**: Blender freezes when using the addon.

**Solutions**:
1. Reduce the number of objects in the scene
2. Close other applications to free memory
3. Save your work frequently
4. Consider upgrading your hardware
5. Split large levels into multiple chunks

## Workflow Issues

### Hard to manage many objects

**Problem**: Difficult to keep track of properties for many objects.

**Solutions**:
1. Use Blender collections to organize by type
2. Use descriptive names for objects
3. Use the batch operation scripts (see example_batch_operations.py)
4. Export frequently to test in engine
5. Consider using Blender's outliner to manage objects

### Repetitive property setting

**Problem**: Setting the same properties for many objects is tedious.

**Solutions**:
1. Use the copy_properties_to_selected() function from example_batch_operations.py
2. Set properties on one object, then duplicate it
3. Use Python scripts for batch operations
4. Create template objects with common settings
5. Use Blender's linked duplicates for identical objects

## Engine Integration Issues

### Exported chunks don't load in engine

**Problem**: Engine can't load the exported JSON.

**Solutions**:
1. Verify the JSON is valid
2. Check the file path is correct in your level definition
3. Make sure mesh and material files exist in the engine
4. Check engine logs for specific errors
5. Compare with a working chunk JSON

### Objects appear but look wrong

**Problem**: Objects load but don't look correct in the engine.

**Solutions**:
1. Verify mesh files match the mesh_name in JSON
2. Check material files exist and are correct
3. Verify colors are in the right format (0.0-1.0)
4. Check culling radius isn't too small
5. Verify LOD distances are appropriate

### Tags not working in engine

**Problem**: Custom tags don't have the expected effect.

**Solutions**:
1. Check tag names match what the engine expects (case-sensitive)
2. Verify tag values are in the correct format
3. Check engine code is reading the tags correctly
4. Look at engine logs for tag-related errors
5. Compare with working examples

## Getting Help

If you're still having issues:

1. Check the README.md for detailed documentation
2. Look at example_batch_operations.py for code examples
3. Examine a working chunk JSON to compare format
4. Check Blender's console for Python errors
5. Verify your Blender version is 3.0 or higher
6. Try with a fresh Blender scene
7. Reinstall the addon
8. Check the engine documentation for level format requirements

## Reporting Bugs

When reporting issues, please include:

- Blender version
- Operating system
- Addon version
- Steps to reproduce the problem
- Error messages from console
- Example .blend file if possible
- Exported JSON if relevant

## Common Error Messages

### "No active mesh object"
- Select a mesh object before using the operation

### "Invalid JSON format"
- The imported file is not valid JSON or has wrong structure

### "Permission denied"
- Check file permissions on export/import location

### "Module not found"
- Addon installation is incomplete or corrupted

### "Attribute error"
- Addon may need to be reloaded or reinstalled
