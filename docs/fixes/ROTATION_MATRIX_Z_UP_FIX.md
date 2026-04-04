# Rotation Matrix Z-Up Fix

## Problem

The `Matrix4x4::FromTRS` function in `Math.h` was using Y-up rotation matrices, causing incorrect orientation of meshes loaded from Blender (which uses Z-up).

## Root Cause

The rotation matrix construction assumed:
- Y-axis = Up
- Z-axis = Forward

But SecretEngine uses Z-up coordinate system:
- Z-axis = Up
- Y-axis = Forward

## Symptoms

- Walls with 90° X-rotation in Blender appear incorrectly oriented in-game
- Positions match but rotations are wrong
- Meshes that should be vertical appear horizontal or vice versa

## Solution

Update rotation matrices in `Math.h` to use Z-up convention:

### Y-Rotation (around vertical Z-axis)
```cpp
// OLD (Y-up):
// Row 0: [cy, 0, sy]
// Row 1: [0, 1, 0]
// Row 2: [-sy, 0, cy]

// NEW (Z-up):
// Row 0: [cy, -sy, 0]
// Row 1: [sy, cy, 0]
// Row 2: [0, 0, 1]
```

### X-Rotation (around X-axis, pitch)
```cpp
// OLD (Y-up):
// Row 0: [1, 0, 0]
// Row 1: [0, cy, -sy]
// Row 2: [0, sy, cy]

// NEW (Z-up):
// Row 0: [1, 0, 0]
// Row 1: [0, cy, -sy]
// Row 2: [0, sy, cy]
// (Same - X rotation unchanged)
```

### Z-Rotation (around vertical axis, roll)
```cpp
// OLD (Y-up):
// Row 0: [cy, -sy, 0]
// Row 1: [sy, cy, 0]
// Row 2: [0, 0, 1]

// NEW (Z-up):
// Row 0: [cy, -sy, 0]
// Row 1: [sy, cy, 0]
// Row 2: [0, 0, 1]
// (Same - this was already Z rotation)
```

## Files Modified

- `core/include/SecretEngine/Math.h` - Updated `FromTRS`, `RotationX`, `RotationY`, `RotationZ`
- `plugins/VulkanRenderer/src/MegaGeometryRenderer.cpp` - Updated fast-path Y-rotation matrix

## Verification

After fix:
- Walls with `[90, 0, 0]` rotation in Blender render vertically in-game
- Walls with `[0, 90, 0]` rotation face correct direction
- Positions and rotations match Blender viewport exactly
