# Coordinate System - Z-up Throughout

SecretEngine uses **Z-up, right-handed coordinates** consistently:

- **X-axis**: Right/Left
- **Y-axis**: Forward/Backward  
- **Z-axis**: Up/Down (vertical)

This matches Blender's coordinate system, allowing direct mapping without conversion.

## Engine Implementation

The engine has been fully converted to Z-up:

### Physics
- Gravity: `{0, 0, -9.81f}` (Z is vertical)
- Jump velocity affects Z component
- Ground checks test Z position

### Movement
- Horizontal movement in XY plane
- Vertical movement in Z axis
- Camera forward/right vectors in XY plane

### Camera
- Up vector: `{0, 0, 1}`
- Forward/right calculated in XY plane
- Pitch affects Z component of view direction

### AI & Combat
- Patrol targets in XY plane
- Rotation around Z axis
- Spawn positions use Z for height

## Benefits

1. **Direct Blender Integration**: No coordinate conversion needed
2. **Consistent**: All systems use the same coordinate system
3. **Standard**: Matches Blender, 3ds Max, and many CAD tools
