# Advanced Features & Extensions

This document covers advanced usage, customization, and potential extensions for the Secret Engine Level Editor addon.

## Advanced Python Scripting

### Custom Property Types

You can extend the addon to support additional property types by modifying `properties.py`:

```python
# Add to SecretEngineInstanceProperties class
physics_enabled: BoolProperty(
    name="Physics Enabled",
    description="Enable physics simulation",
    default=False
)

physics_mass: FloatProperty(
    name="Mass",
    description="Object mass for physics",
    default=1.0,
    min=0.0
)
```

Then update `operators.py` to export these properties:

```python
# In build_instance_data method
if props.physics_enabled:
    instance["physics"] = {
        "enabled": True,
        "mass": props.physics_mass
    }
```

### Custom Operators

Create custom operators for specific workflows:

```python
class SECRETENGINE_OT_CreateRoom(bpy.types.Operator):
    bl_idname = "secret_engine.create_room"
    bl_label = "Create Room"
    
    width: FloatProperty(name="Width", default=10.0)
    height: FloatProperty(name="Height", default=3.0)
    depth: FloatProperty(name="Depth", default=10.0)
    
    def execute(self, context):
        # Create floor
        bpy.ops.mesh.primitive_cube_add()
        floor = context.active_object
        floor.scale = (self.width/2, self.depth/2, 0.1)
        floor.location.z = -0.1
        
        # Create walls
        # ... (wall creation code)
        
        # Set properties
        for obj in [floor]:  # and walls
            obj.secret_engine.mesh_name = "cube.meshbin"
            obj.secret_engine.material_name = "wall.mat"
            obj.secret_engine.tag_type = "architecture"
        
        return {'FINISHED'}
```

### Property Validation

Add validation to ensure data integrity:

```python
def validate_mesh_name(self, context):
    """Validate mesh name has correct extension"""
    if self.mesh_name and not self.mesh_name.endswith('.meshbin'):
        self.mesh_name += '.meshbin'

# In property definition
mesh_name: StringProperty(
    name="Mesh",
    update=validate_mesh_name
)
```

## Advanced Export Features

### Conditional Export

Export only objects matching certain criteria:

```python
def build_chunk_data_filtered(self, context, filter_func):
    """Export only objects that pass the filter function"""
    mesh_groups = {}
    
    for obj in scene.objects:
        if obj.type != 'MESH':
            continue
        
        if not filter_func(obj):
            continue
        
        # ... rest of export logic
```

Usage:
```python
# Export only objects with specific tag
chunk_data = self.build_chunk_data_filtered(
    context,
    lambda obj: obj.secret_engine.tag_type == "mesh"
)
```

### Multi-Format Export

Support multiple export formats:

```python
class SECRETENGINE_OT_ExportChunkBinary(bpy.types.Operator):
    """Export chunk in binary format for faster loading"""
    bl_idname = "secret_engine.export_chunk_binary"
    bl_label = "Export Chunk (Binary)"
    
    def execute(self, context):
        import struct
        
        chunk_data = self.build_chunk_data(context)
        
        with open(self.filepath, 'wb') as f:
            # Write binary format
            # ... (binary serialization code)
        
        return {'FINISHED'}
```

### Export Optimization

Optimize exported data:

```python
def optimize_chunk_data(chunk_data):
    """Remove redundant data and optimize structure"""
    
    # Remove default values
    for mesh_group in chunk_data["mesh_groups"]:
        for instance in mesh_group["instances"]:
            # Remove default culling radius
            if instance.get("culling", {}).get("radius") == 1.0:
                instance.pop("culling", None)
            
            # Remove default scale
            if instance["transform"]["scale"] == [1, 1, 1]:
                instance["transform"].pop("scale")
    
    return chunk_data
```

## Advanced Import Features

### Import with Mesh Loading

Load actual mesh geometry on import:

```python
def load_mesh_geometry(mesh_name):
    """Load mesh from engine format"""
    # This would require implementing your mesh format reader
    # For now, we use placeholder cubes
    pass

def create_object_from_instance_with_mesh(self, context, instance, mesh_name):
    """Create object with actual mesh geometry"""
    
    # Try to load mesh
    mesh_data = load_mesh_geometry(mesh_name)
    
    if mesh_data:
        # Create mesh from data
        mesh = bpy.data.meshes.new(mesh_name)
        # ... populate mesh data
        obj = bpy.data.objects.new(mesh_name, mesh)
    else:
        # Fall back to cube
        bpy.ops.mesh.primitive_cube_add()
        obj = context.active_object
    
    # ... rest of import logic
```

### Import Validation

Validate imported data:

```python
def validate_chunk_data(chunk_data):
    """Validate chunk data before import"""
    errors = []
    
    if "chunk_id" not in chunk_data:
        errors.append("Missing chunk_id")
    
    if "mesh_groups" not in chunk_data:
        errors.append("Missing mesh_groups")
    
    for i, mesh_group in enumerate(chunk_data.get("mesh_groups", [])):
        if "mesh" not in mesh_group:
            errors.append(f"Mesh group {i} missing mesh")
        
        if "material" not in mesh_group:
            errors.append(f"Mesh group {i} missing material")
    
    return errors
```

## UI Customization

### Custom Panels

Add specialized panels for different workflows:

```python
class SECRETENGINE_PT_PhysicsPanel(bpy.types.Panel):
    """Panel for physics properties"""
    bl_label = "Physics Properties"
    bl_idname = "SECRETENGINE_PT_physics_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Secret Engine'
    bl_parent_id = "SECRETENGINE_PT_main_panel"
    
    def draw(self, context):
        layout = self.layout
        obj = context.active_object
        
        if obj and obj.type == 'MESH':
            props = obj.secret_engine
            layout.prop(props, "physics_enabled")
            if props.physics_enabled:
                layout.prop(props, "physics_mass")
```

### Dynamic UI

Create UI that adapts based on object type:

```python
def draw(self, context):
    layout = self.layout
    obj = context.active_object
    props = obj.secret_engine
    
    # Show different properties based on type
    if props.tag_type == "trigger":
        layout.label(text="Trigger Properties")
        layout.prop(props, "trigger_radius")
        layout.prop(props, "trigger_event")
    
    elif props.tag_type == "light":
        layout.label(text="Light Properties")
        layout.prop(props, "light_intensity")
        layout.prop(props, "light_color")
```

### Custom Visualizations

Add viewport overlays:

```python
def draw_callback_px(self, context):
    """Draw custom viewport overlay"""
    import bgl
    import gpu
    from gpu_extras.batch import batch_for_shader
    
    # Draw culling radius spheres
    for obj in context.scene.objects:
        if obj.type == 'MESH':
            radius = obj.secret_engine.culling_radius
            # ... draw sphere at obj.location with radius
```

## Integration with Engine

### Live Preview

Create a live preview system:

```python
class SECRETENGINE_OT_StartLivePreview(bpy.types.Operator):
    """Start live preview in engine"""
    bl_idname = "secret_engine.start_live_preview"
    bl_label = "Start Live Preview"
    
    def execute(self, context):
        # Export to temp file
        temp_path = "/tmp/preview_chunk.json"
        # ... export logic
        
        # Send to engine via socket/pipe
        import socket
        sock = socket.socket()
        sock.connect(("localhost", 9999))
        sock.send(json.dumps(chunk_data).encode())
        
        return {'FINISHED'}
```

### Asset Browser Integration

Integrate with Blender's asset browser:

```python
def mark_as_asset(obj):
    """Mark object as asset for reuse"""
    obj.asset_mark()
    obj.asset_data.description = f"Secret Engine: {obj.secret_engine.tag_type}"
    obj.asset_data.tags.new("SecretEngine")
```

## Performance Optimization

### Lazy Loading

Load properties only when needed:

```python
class SecretEngineInstancePropertiesLazy(bpy.types.PropertyGroup):
    _cached_data = None
    
    def get_data(self):
        if self._cached_data is None:
            self._cached_data = self.load_data()
        return self._cached_data
```

### Batch Operations

Optimize batch operations:

```python
def batch_set_property(objects, property_name, value):
    """Set property for multiple objects efficiently"""
    # Disable updates during batch operation
    for obj in objects:
        obj.secret_engine[property_name] = value
    
    # Trigger single update
    bpy.context.view_layer.update()
```

## Extension Ideas

### 1. Collision Shape Editor

Add support for collision shapes:

```python
class SecretEngineCollisionProperties(bpy.types.PropertyGroup):
    collision_type: EnumProperty(
        name="Collision Type",
        items=[
            ('NONE', 'None', 'No collision'),
            ('BOX', 'Box', 'Box collision'),
            ('SPHERE', 'Sphere', 'Sphere collision'),
            ('MESH', 'Mesh', 'Mesh collision'),
        ]
    )
    
    collision_offset: FloatVectorProperty(
        name="Offset",
        default=(0, 0, 0)
    )
```

### 2. Light System

Add light placement and configuration:

```python
class SecretEngineLightProperties(bpy.types.PropertyGroup):
    light_type: EnumProperty(
        items=[
            ('POINT', 'Point', 'Point light'),
            ('SPOT', 'Spot', 'Spot light'),
            ('DIRECTIONAL', 'Directional', 'Directional light'),
        ]
    )
    
    intensity: FloatProperty(name="Intensity", default=1.0)
    color: FloatVectorProperty(name="Color", subtype='COLOR')
    range: FloatProperty(name="Range", default=10.0)
```

### 3. Particle System

Support particle emitters:

```python
class SecretEngineParticleProperties(bpy.types.PropertyGroup):
    particle_system: StringProperty(name="Particle System")
    emission_rate: FloatProperty(name="Emission Rate", default=10.0)
    lifetime: FloatProperty(name="Lifetime", default=5.0)
```

### 4. Audio Sources

Add audio source placement:

```python
class SecretEngineAudioProperties(bpy.types.PropertyGroup):
    audio_clip: StringProperty(name="Audio Clip")
    volume: FloatProperty(name="Volume", default=1.0, min=0.0, max=1.0)
    loop: BoolProperty(name="Loop", default=False)
    spatial: BoolProperty(name="3D Spatial", default=True)
    range: FloatProperty(name="Range", default=10.0)
```

### 5. Navigation Mesh

Generate navigation meshes:

```python
class SECRETENGINE_OT_GenerateNavMesh(bpy.types.Operator):
    """Generate navigation mesh from level geometry"""
    bl_idname = "secret_engine.generate_navmesh"
    bl_label = "Generate NavMesh"
    
    def execute(self, context):
        # Analyze level geometry
        # Generate walkable surfaces
        # Export navmesh data
        return {'FINISHED'}
```

### 6. Prefab System

Create reusable prefabs:

```python
class SECRETENGINE_OT_SavePrefab(bpy.types.Operator):
    """Save selected objects as prefab"""
    bl_idname = "secret_engine.save_prefab"
    bl_label = "Save Prefab"
    
    prefab_name: StringProperty(name="Prefab Name")
    
    def execute(self, context):
        # Save selected objects with properties
        # Store in prefab library
        return {'FINISHED'}
```

### 7. Terrain Editor

Add terrain editing tools:

```python
class SECRETENGINE_OT_CreateTerrain(bpy.types.Operator):
    """Create terrain from heightmap"""
    bl_idname = "secret_engine.create_terrain"
    bl_label = "Create Terrain"
    
    heightmap_path: StringProperty(subtype='FILE_PATH')
    size: FloatProperty(default=100.0)
    height_scale: FloatProperty(default=10.0)
    
    def execute(self, context):
        # Load heightmap
        # Generate terrain mesh
        # Set properties
        return {'FINISHED'}
```

### 8. Material Preview

Preview engine materials in Blender:

```python
def create_material_preview(material_name):
    """Create Blender material that approximates engine material"""
    mat = bpy.data.materials.new(material_name)
    mat.use_nodes = True
    
    # Load material definition
    # Create node setup
    # Apply to object
    
    return mat
```

### 9. Validation Tools

Advanced validation and error checking:

```python
class SECRETENGINE_OT_ValidateLevel(bpy.types.Operator):
    """Validate entire level for common issues"""
    bl_idname = "secret_engine.validate_level"
    bl_label = "Validate Level"
    
    def execute(self, context):
        issues = []
        
        # Check for overlapping objects
        # Verify all references exist
        # Check for performance issues
        # Validate chunk boundaries
        
        # Display report
        return {'FINISHED'}
```

### 10. Version Control Integration

Integrate with version control:

```python
class SECRETENGINE_OT_CommitLevel(bpy.types.Operator):
    """Commit level changes to version control"""
    bl_idname = "secret_engine.commit_level"
    bl_label = "Commit Level"
    
    commit_message: StringProperty(name="Commit Message")
    
    def execute(self, context):
        # Export level
        # Git add/commit
        # Push to remote
        return {'FINISHED'}
```

## Best Practices for Extensions

1. **Modularity**: Keep extensions in separate files
2. **Documentation**: Document all custom properties and operators
3. **Testing**: Test with various scenarios
4. **Performance**: Profile and optimize
5. **Compatibility**: Maintain backward compatibility
6. **User Feedback**: Provide clear feedback and error messages
7. **Defaults**: Use sensible default values
8. **Validation**: Validate all user input
9. **Undo Support**: Ensure operations are undoable
10. **Consistency**: Follow Blender UI conventions

## Contributing Extensions

If you create useful extensions:

1. Document the feature thoroughly
2. Include example usage
3. Test with various Blender versions
4. Follow the existing code style
5. Submit as a pull request or share with the community

## Resources

- Blender Python API: https://docs.blender.org/api/current/
- Blender Addon Tutorial: https://docs.blender.org/manual/en/latest/advanced/scripting/addon_tutorial.html
- Secret Engine Documentation: (link to your engine docs)
