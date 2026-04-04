# Instance Cleanup System Implementation

## Problem
Instances were being added to the MegaGeometryRenderer (flywheel) but never removed when levels changed. This caused:
- Memory accumulation (4000+ instances staying in memory)
- GPU culling only hiding instances (setting instanceCount=0) but not freeing memory
- No cleanup when switching between levels

## Solution
Implemented a complete instance lifecycle management system that clears all instances when levels are loaded/unloaded.

## Changes Made

### 1. MegaGeometryRenderer (Flywheel)
**File: `plugins/VulkanRenderer/src/MegaGeometryRenderer.h`**
- Added `ClearAllInstances()` method declaration

**File: `plugins/VulkanRenderer/src/MegaGeometryRenderer.cpp`**
- Implemented `ClearAllInstances()` method that:
  - Resets the instance counter to 0
  - Clears all instance data in both double-buffered arrays
  - Resets all indirect draw command instance counts to 0
  - Logs the number of instances removed

### 2. Renderer Interface
**File: `core/include/SecretEngine/IRenderer.h`**
- Added virtual `ClearAllInstances()` method to the IRenderer interface
- Default implementation does nothing (for compatibility)

### 3. Renderer Plugin
**File: `plugins/VulkanRenderer/src/RendererPlugin.h`**
- Added `ClearAllInstances()` override declaration

**File: `plugins/VulkanRenderer/src/RendererPlugin.cpp`**
- Implemented `ClearAllInstances()` that:
  - Calls the MegaGeometryRenderer's ClearAllInstances()
  - Logs the cleanup operation

### 4. Level System Integration
**File: `plugins/LevelSystem/src/V73LevelSystemPlugin.cpp`**
- Added `#include <SecretEngine/IRenderer.h>`
- Modified `LoadLevel()` to clear all instances BEFORE loading a new level
- Modified `UnloadLevel()` to clear all instances when unloading

## How It Works

### When Loading a Level:
1. Level system calls `renderer->ClearAllInstances()`
2. All existing instances are removed from the flywheel
3. New level is loaded
4. New instances are added fresh

### When Unloading a Level:
1. Level system calls `renderer->ClearAllInstances()`
2. All instances are removed from the flywheel
3. Level data is unloaded
4. Memory is freed

## Benefits

1. **Clean State**: Every level starts with a clean slate
2. **Memory Management**: No accumulation of instances from previous levels
3. **Predictable Behavior**: Instance count accurately reflects current level
4. **Performance**: GPU only processes instances from the current level
5. **Automatic**: Cleanup happens automatically on level change

## Testing

To verify the implementation works:
1. Load a level and check instance count
2. Switch to another level
3. Verify instance count resets to 0 before new level loads
4. Check logs for "Cleared all instances" messages

## Future Enhancements

Potential improvements:
- Per-chunk instance tracking for streaming
- Partial cleanup when chunks are unloaded
- Instance pooling for reuse
- Memory defragmentation after cleanup
