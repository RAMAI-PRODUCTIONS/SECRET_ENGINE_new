"""
Example batch operations for Secret Engine Level Editor
Run these scripts in Blender's Python console or Text Editor

To use:
1. Open Blender
2. Go to Scripting workspace
3. Create a new text file
4. Copy one of these examples
5. Click "Run Script"
"""

import bpy
import random

# Example 1: Set mesh and material for all selected objects
def set_mesh_material_for_selected(mesh_name="cube.meshbin", material_name="default.mat"):
    """Set the same mesh and material for all selected objects"""
    for obj in bpy.context.selected_objects:
        if obj.type == 'MESH':
            obj.secret_engine.mesh_name = mesh_name
            obj.secret_engine.material_name = material_name
            print(f"Set {obj.name}: {mesh_name}, {material_name}")

# Example 2: Auto-name objects based on their type
def auto_name_objects():
    """Automatically name objects based on their type tag"""
    counters = {}
    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH':
            obj_type = obj.secret_engine.tag_type
            if obj_type not in counters:
                counters[obj_type] = 0
            
            obj.secret_engine.tag_name = f"{obj_type}_{counters[obj_type]}"
            obj.name = obj.secret_engine.tag_name
            counters[obj_type] += 1
            print(f"Named: {obj.name}")

# Example 3: Generate random colored cubes in a grid
def create_cube_grid(rows=5, cols=5, spacing=3.0):
    """Create a grid of cubes with random colors"""
    for i in range(rows):
        for j in range(cols):
            # Create cube
            bpy.ops.mesh.primitive_cube_add(
                location=(i * spacing, j * spacing, 0)
            )
            obj = bpy.context.active_object
            
            # Set properties
            props = obj.secret_engine
            props.mesh_name = "cube.meshbin"
            props.material_name = "default.mat"
            props.tag_type = "cube"
            props.tag_name = f"cube_{i}_{j}"
            
            # Random color
            props.use_custom_color = True
            props.material_color = (
                random.random(),
                random.random(),
                random.random()
            )
            
            props.culling_radius = 1.0
            
            obj.name = props.tag_name
    
    print(f"Created {rows * cols} cubes")

# Example 4: Add LOD to all selected objects
def add_lod_to_selected(distances=[25, 75, 150]):
    """Add LOD levels to all selected objects"""
    for obj in bpy.context.selected_objects:
        if obj.type == 'MESH':
            props = obj.secret_engine
            props.use_lod = True
            
            # Clear existing LOD levels
            props.lod_levels.clear()
            
            # Add new levels
            for distance in distances:
                lod = props.lod_levels.add()
                lod.distance = distance
            
            print(f"Added LOD to {obj.name}")

# Example 5: Set culling radius based on object size
def auto_culling_radius():
    """Automatically set culling radius based on object dimensions"""
    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH':
            # Get object dimensions
            dims = obj.dimensions
            max_dim = max(dims.x, dims.y, dims.z)
            
            # Set culling radius to 1.5x the largest dimension
            obj.secret_engine.culling_radius = max_dim * 1.5
            print(f"{obj.name}: culling radius = {obj.secret_engine.culling_radius:.2f}")

# Example 6: Create a circle of objects
def create_circle_pattern(count=12, radius=10.0, mesh="Character.meshbin"):
    """Create objects in a circular pattern"""
    import math
    
    for i in range(count):
        angle = (2 * math.pi * i) / count
        x = radius * math.cos(angle)
        y = radius * math.sin(angle)
        
        bpy.ops.mesh.primitive_cube_add(location=(x, y, 0))
        obj = bpy.context.active_object
        
        # Rotate to face center
        obj.rotation_euler.z = angle + math.pi / 2
        
        props = obj.secret_engine
        props.mesh_name = mesh
        props.material_name = "default.mat"
        props.tag_type = "mesh"
        props.tag_name = f"circle_object_{i}"
        props.culling_radius = 5.0
        
        obj.name = props.tag_name
    
    print(f"Created {count} objects in circle")

# Example 7: Add custom tag to all objects of a type
def add_tag_to_type(object_type="mesh", tag_key="layer", tag_value="gameplay"):
    """Add a custom tag to all objects of a specific type"""
    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH' and obj.secret_engine.tag_type == object_type:
            # Check if tag already exists
            existing = [t for t in obj.secret_engine.tags if t.key == tag_key]
            
            if not existing:
                tag = obj.secret_engine.tags.add()
                tag.key = tag_key
                tag.value = tag_value
                print(f"Added tag to {obj.name}")

# Example 8: Export multiple chunks from collections
def export_collections_as_chunks(output_dir="/tmp/"):
    """Export each collection as a separate chunk"""
    import os
    
    for collection in bpy.data.collections:
        # Temporarily hide all other collections
        original_visibility = {}
        for col in bpy.data.collections:
            original_visibility[col.name] = col.hide_viewport
            col.hide_viewport = (col.name != collection.name)
        
        # Set chunk ID
        bpy.context.scene.secret_engine.chunk_id = f"{collection.name}_chunk"
        
        # Export
        filepath = os.path.join(output_dir, f"{collection.name}_chunk.json")
        bpy.ops.secret_engine.export_chunk(filepath=filepath)
        
        # Restore visibility
        for col in bpy.data.collections:
            col.hide_viewport = original_visibility[col.name]
        
        print(f"Exported {collection.name} to {filepath}")

# Example 9: Validate all objects have required properties
def validate_objects():
    """Check that all objects have required properties set"""
    issues = []
    
    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH':
            props = obj.secret_engine
            
            if not props.mesh_name:
                issues.append(f"{obj.name}: Missing mesh name")
            
            if not props.material_name:
                issues.append(f"{obj.name}: Missing material name")
            
            if not props.tag_type:
                issues.append(f"{obj.name}: Missing type tag")
            
            if not props.tag_name:
                issues.append(f"{obj.name}: Missing name tag")
    
    if issues:
        print("Validation issues found:")
        for issue in issues:
            print(f"  - {issue}")
    else:
        print("All objects validated successfully!")
    
    return len(issues) == 0

# Example 10: Copy properties from active to selected
def copy_properties_to_selected():
    """Copy Secret Engine properties from active object to all selected"""
    active = bpy.context.active_object
    if not active or active.type != 'MESH':
        print("No active mesh object")
        return
    
    src_props = active.secret_engine
    
    for obj in bpy.context.selected_objects:
        if obj != active and obj.type == 'MESH':
            dst_props = obj.secret_engine
            
            # Copy basic properties
            dst_props.mesh_name = src_props.mesh_name
            dst_props.material_name = src_props.material_name
            dst_props.tag_type = src_props.tag_type
            dst_props.use_custom_color = src_props.use_custom_color
            dst_props.material_color = src_props.material_color
            dst_props.culling_radius = src_props.culling_radius
            dst_props.use_lod = src_props.use_lod
            
            # Copy LOD levels
            dst_props.lod_levels.clear()
            for lod in src_props.lod_levels:
                new_lod = dst_props.lod_levels.add()
                new_lod.distance = lod.distance
            
            # Copy custom tags
            dst_props.tags.clear()
            for tag in src_props.tags:
                new_tag = dst_props.tags.add()
                new_tag.key = tag.key
                new_tag.value = tag.value
            
            print(f"Copied properties to {obj.name}")

# Usage examples:
if __name__ == "__main__":
    # Uncomment the function you want to run:
    
    # set_mesh_material_for_selected("cube.meshbin", "default.mat")
    # auto_name_objects()
    # create_cube_grid(5, 5, 3.0)
    # add_lod_to_selected([25, 75, 150])
    # auto_culling_radius()
    # create_circle_pattern(12, 10.0)
    # add_tag_to_type("mesh", "layer", "gameplay")
    # validate_objects()
    # copy_properties_to_selected()
    
    print("Ready to run batch operations!")
