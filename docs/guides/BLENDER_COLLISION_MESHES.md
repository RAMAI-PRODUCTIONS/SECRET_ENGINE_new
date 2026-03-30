# Creating Collision Meshes in Blender for SecretEngine

## Overview
Collision meshes are simplified versions of visual meshes used for physics calculations. They should be as simple as possible while still representing the object's shape accurately for gameplay.

## Why Separate Collision Meshes?

### Performance
- Visual meshes can have thousands of triangles
- Collision detection is expensive with complex geometry
- Simple collision meshes = faster physics calculations

### Accuracy
- Complex visual details don't affect gameplay
- Simplified shapes provide predictable collision behavior
- Easier to debug collision issues

## Collision Mesh Types

### 1. Box Colliders (Fastest)
Best for: Walls, platforms, crates, buildings

**Blender Setup**:
```
1. Add Cube (Shift+A → Mesh → Cube)
2. Scale to match object bounds
3. Name: "ObjectName_collision"
```

### 2. Sphere Colliders
Best for: Balls, round objects, character capsules

**Blender Setup**:
```
1. Add UV Sphere (Shift+A → Mesh → UV Sphere)
2. Scale to match object
3. Keep low poly count (8-16 segments)
4. Name: "ObjectName_collision"
```

### 3. Capsule Colliders
Best for: Characters, cylindrical objects

**Blender Setup**:
```
1. Add Cylinder
2. Add two UV Spheres for caps
3. Join (Ctrl+J)
4. Name: "Character_collision"
```

### 4. Convex Hull
Best for: Irregular objects that need accurate collision

**Blender Setup**:
```
1. Duplicate visual mesh (Shift+D)
2. Decimate modifier (reduce to ~100 triangles)
3. Mesh → Convex Hull
4. Name: "ObjectName_collision"
```

### 5. Mesh Colliders (Slowest)
Best for: Static terrain, complex static geometry

**Blender Setup**:
```
1. Duplicate visual mesh
2. Decimate heavily (target: <500 triangles)
3. Remove interior faces
4. Name: "Terrain_collision"
```

## Naming Conventions

### Standard Convention
```
Visual Mesh:     "Platform_01"
Collision Mesh:  "Platform_01_collision"

Visual Mesh:     "Enemy_Tank"
Collision Mesh:  "Enemy_Tank_collision"
```

### Alternative: Separate Collections
```
Collection: "Visual"
  - Platform_01
  - Enemy_Tank

Collection: "Collision"
  - Platform_01_col
  - Enemy_Tank_col
```

## Creating Collision Meshes: Step-by-Step

### Method 1: Simple Box Collision

```
1. Select your visual mesh
2. Shift+A → Mesh → Cube
3. Scale cube to encompass object (S key)
4. Position to match object center
5. Rename to "ObjectName_collision"
6. Set display type to "Wire" (Object Properties → Viewport Display)
7. Add to "Collision" collection
```

### Method 2: Simplified Mesh Collision

```
1. Select visual mesh
2. Duplicate (Shift+D, then Esc to keep in place)
3. Rename to "ObjectName_collision"
4. Add Decimate modifier:
   - Collapse mode
   - Ratio: 0.1 (90% reduction)
5. Apply modifier (Ctrl+A)
6. Optional: Mesh → Convex Hull for convex objects
7. Set display to "Wire"
```

### Method 3: Multi-Primitive Collision

For complex objects, use multiple simple shapes:

```
1. Analyze object shape
2. Add multiple cubes/spheres to approximate shape
3. Position each primitive
4. Select all collision primitives
5. Join (Ctrl+J)
6. Rename to "ObjectName_collision"
```

Example: Tank
- Box for body
- Box for turret
- Cylinders for wheels
- Join all → "Tank_collision"

## Collision Mesh Guidelines

### Triangle Count Targets
- **Simple objects**: 6-12 triangles (box)
- **Medium objects**: 20-50 triangles
- **Complex objects**: 100-200 triangles
- **Terrain**: 500-1000 triangles per chunk

### Quality Checklist
- ✅ Fully encloses visual mesh (no gaps)
- ✅ No interior faces (remove hidden geometry)
- ✅ Convex when possible (faster collision)
- ✅ Manifold geometry (no holes, no non-manifold edges)
- ✅ Proper scale (matches visual mesh bounds)
- ✅ Named correctly (includes "_collision" suffix)

### Common Mistakes
- ❌ Too many triangles (defeats the purpose)
- ❌ Collision mesh smaller than visual mesh (objects clip through)
- ❌ Non-manifold geometry (causes physics glitches)
- ❌ Overlapping collision meshes (causes jittering)
- ❌ Forgetting to apply scale/rotation

## Blender Tools for Collision Meshes

### Decimate Modifier
Reduces triangle count while preserving shape:
```
1. Add Modifier → Decimate
2. Collapse mode
3. Ratio: 0.1 to 0.3 (experiment)
4. Apply modifier
```

### Convex Hull
Creates convex collision mesh:
```
1. Select collision mesh
2. Edit Mode (Tab)
3. Select all (A)
4. Mesh → Convex Hull
5. Tab to exit Edit Mode
```

### Remesh Modifier
Creates uniform topology:
```
1. Add Modifier → Remesh
2. Mode: Blocks or Voxel
3. Octree Depth: 4-6
4. Apply modifier
```

### Shrinkwrap Modifier
Fits collision to visual mesh:
```
1. Create simple collision shape
2. Add Modifier → Shrinkwrap
3. Target: Visual mesh
4. Offset: 0.01 (slight expansion)
5. Apply modifier
```

## Exporting Collision Meshes

### Option 1: Separate Collision JSON

Export collision meshes to separate JSON file:

```python
def export_collision_meshes(filepath):
    collision_data = {
        "version": "1.0",
        "collision_meshes": []
    }
    
    for obj in bpy.context.scene.objects:
        if "_collision" in obj.name.lower():
            mesh_data = {
                "name": obj.name,
                "type": "mesh",  # or "box", "sphere", "capsule"
                "vertices": [],
                "indices": []
            }
            
            # Export mesh data
            mesh = obj.data
            for vert in mesh.vertices:
                mesh_data["vertices"].extend([vert.co.x, vert.co.z, -vert.co.y])
            
            for poly in mesh.polygons:
                mesh_data["indices"].extend(poly.vertices)
            
            collision_data["collision_meshes"].append(mesh_data)
    
    with open(filepath, 'w') as f:
        json.dump(collision_data, f, indent=2)
```

### Option 2: Embed in Level JSON

Add collision reference to entity:

```json
{
  "name": "Platform_01",
  "transform": { ... },
  "mesh": "meshes/platform.meshbin",
  "collision": {
    "type": "box",
    "bounds": {
      "center": [0, 0, 0],
      "extents": [5, 0.5, 5]
    }
  }
}
```

### Option 3: Auto-Generate from Bounds

Let engine generate simple collision:

```json
{
  "name": "Platform_01",
  "transform": { ... },
  "mesh": "meshes/platform.meshbin",
  "collision": {
    "type": "auto",
    "shape": "box"
  }
}
```

## Collision Types in SecretEngine

### Static Collision
Objects that never move:
- Terrain
- Buildings
- Walls
- Platforms

**Properties**:
- No physics simulation
- Optimized for static queries
- Can be combined into single collision mesh

### Dynamic Collision
Objects that move and respond to physics:
- Characters
- Vehicles
- Projectiles
- Movable objects

**Properties**:
- Full physics simulation
- Requires simple collision shapes
- Should use convex hulls when possible

### Trigger Volumes
Invisible collision zones:
- Spawn points
- Checkpoints
- Damage zones
- Level boundaries

**Properties**:
- No physical response
- Detects overlaps only
- Can be any shape

## Advanced Techniques

### Compound Collision Shapes

For complex objects, use multiple simple shapes:

```python
# In Blender export script
def export_compound_collision(obj):
    collision_parts = []
    
    # Find all child collision objects
    for child in obj.children:
        if "_collision" in child.name:
            part = {
                "type": detect_shape_type(child),  # "box", "sphere", etc.
                "transform": get_relative_transform(child, obj)
            }
            collision_parts.append(part)
    
    return {
        "type": "compound",
        "parts": collision_parts
    }
```

### LOD Collision

Different collision detail at different distances:

```
Close range:  Detailed collision (200 triangles)
Medium range: Simplified collision (50 triangles)
Far range:    Box collision (12 triangles)
```

### Collision Layers

Organize collision by gameplay purpose:

```
Layer 0: Terrain
Layer 1: Characters
Layer 2: Projectiles
Layer 3: Triggers
Layer 4: Vehicles
```

## Testing Collision Meshes

### Visual Verification in Blender
```
1. Set collision mesh display to "Wire"
2. Set visual mesh to "Solid"
3. Verify collision encompasses visual mesh
4. Check for gaps or overlaps
```

### In-Engine Debug Rendering
Enable collision visualization in SecretEngine:
```cpp
// In game code
PhysicsSystem::SetDebugDraw(true);
PhysicsSystem::SetCollisionWireframe(true);
```

### Performance Testing
```
1. Monitor frame time with collision enabled
2. Check physics update time in profiler
3. Reduce collision complexity if needed
```

## Optimization Tips

### Reduce Triangle Count
- Use boxes instead of meshes when possible
- Combine multiple objects into single collision mesh
- Remove interior faces
- Use convex hulls for irregular shapes

### Simplify Hierarchy
- Flatten nested collision objects
- Combine static collision meshes
- Use spatial partitioning for large scenes

### Use Appropriate Shapes
- Box: 12 triangles → Fastest
- Sphere: 0 triangles (analytical) → Fastest
- Capsule: 0 triangles (analytical) → Fastest
- Convex Hull: 20-100 triangles → Fast
- Mesh: 100-1000 triangles → Slow

## Common Scenarios

### Character Collision
```
Type: Capsule
Height: 1.8m
Radius: 0.3m
Triangles: 0 (analytical)
```

### Vehicle Collision
```
Type: Compound (box + wheels)
Body: Box (12 triangles)
Wheels: 4 cylinders (48 triangles)
Total: 60 triangles
```

### Building Collision
```
Type: Simplified mesh
Original: 50,000 triangles
Collision: 200 triangles
Method: Decimate + Convex Hull
```

### Terrain Collision
```
Type: Mesh
Size: 1000x1000 units
Triangles: 2000 (1m resolution)
Method: Heightmap or simplified mesh
```

## Troubleshooting

### Objects Fall Through Floor
- Collision mesh too small
- Missing collision mesh
- Collision layer mismatch
- Scale not applied in Blender

### Jittery Physics
- Overlapping collision meshes
- Non-convex collision on dynamic objects
- Too many collision triangles
- Collision mesh not centered

### Poor Performance
- Too many collision triangles
- Using mesh collision for dynamic objects
- Not using spatial partitioning
- Collision meshes not optimized

## Workflow Checklist

Before exporting:
- [ ] All collision meshes named with "_collision" suffix
- [ ] Collision meshes in separate collection
- [ ] Scale applied (Ctrl+A → Scale)
- [ ] Rotation applied (Ctrl+A → Rotation)
- [ ] No non-manifold geometry
- [ ] Triangle count within targets
- [ ] Collision encompasses visual mesh
- [ ] Display set to "Wire" for visibility

## Related Documentation
- [Blender Level Workflow](BLENDER_LEVEL_WORKFLOW.md)
- [Physics System](../plugins/PhysicsPlugin/PHYSICS_FEATURES.md)
- [Level System](LEVEL_SYSTEM.md)

## Resources
- Blender Decimate Modifier: https://docs.blender.org/manual/en/latest/modeling/modifiers/generate/decimate.html
- Convex Hull: https://docs.blender.org/manual/en/latest/modeling/meshes/editing/mesh/convex_hull.html
- Physics Best Practices: See engine documentation

## Next Steps
1. Create test collision meshes in Blender
2. Export and test in SecretEngine
3. Profile performance
4. Iterate and optimize
