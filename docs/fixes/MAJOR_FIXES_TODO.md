# MAJOR FIXES TODO - Comprehensive Breakdown

## ISSUE 1: Joystick Visual Feedback Not Working ✅ ALREADY FIXED
**Status**: Code is correct, joystick should be working

**Analysis**:
- InputPlugin.h correctly tracks `m_joyNormalizedX` and `m_joyNormalizedY` (-1 to 1)
- `GetJoystickPosition()` method exists and returns correct values
- `DrawJoystick()` function correctly offsets white inner circle:
  ```cpp
  float stickX = baseX + maxOffset + (joyX * maxOffset);
  float stickY = baseY + maxOffset + (joyY * maxOffset);
  ```
- Joystick resets to center on touch release (sets to 0.0f, 0.0f)

**Conclusion**: This is already working correctly. User may need to test again.

---

## ISSUE 2: Level Switching Not Working ❌ CRITICAL BUG
**Status**: JSON files not being loaded from Android assets

**Root Cause**:
- LevelDefinitions.json paths reference `"Assets/fps_arena.json"` etc.
- But Android AAssetManager expects paths WITHOUT the assets folder prefix
- Files are in `android/app/src/main/assets/Assets/fps_arena.json`
- AAssetManager should open with path `"Assets/fps_arena.json"` (relative to assets folder)

**Evidence from logs**:
```
[ERROR] Level not found: Scene
[ERROR] Level not found: FPS_Arena
Total levels: 0
```

**Fix Required**:
1. Verify JSON files exist in correct Android assets location
2. Check if LevelDefinitions.json is being loaded (path: "data/LevelDefinitions.json")
3. Ensure all level JSON files exist in assets folder
4. Add debug logging to see exact file paths being attempted

**Files to Check**:
- `android/app/src/main/assets/data/LevelDefinitions.json` ✅ EXISTS
- `android/app/src/main/assets/Assets/fps_arena.json` ❓ NEED TO VERIFY
- `android/app/src/main/assets/Assets/scene.json` ❓ NEED TO VERIFY
- `android/app/src/main/assets/Assets/main_menu.json` ❓ NEED TO VERIFY
- `android/app/src/main/assets/Assets/racing_track.json` ❓ NEED TO VERIFY

**Implementation**:
- UnloadCurrentLevel() is already implemented correctly
- LoadLevel() already calls UnloadCurrentLevel() before loading new level
- Just need to fix file paths/locations

---

## ISSUE 3: GPU Frustum Culling Not Working ❌ CRITICAL BUG
**Status**: Triangle count stays constant regardless of camera direction

**Root Cause Analysis**:

### Compute Shader (cull.comp) - ✅ LOOKS CORRECT
- Frustum test logic is sound
- Uses sphere-frustum intersection
- Writes visible instances to output buffer
- Atomically increments instance count

### PreRender() Function - ⚠️ POTENTIAL ISSUES
1. **Compute shader may not be compiled/loaded**
   - Need to verify `cull_comp.spv` exists in assets
   - Check if shader module creation succeeded

2. **View-projection matrix may be identity**
   - If camera isn't updating VP matrix, all instances pass frustum test
   - Need to verify `SetViewProjection()` is being called with correct matrix

3. **Indirect buffer reset may be wrong**
   - Currently resets at offset 4 (instanceCount field)
   - Should reset entire command structure

4. **Dispatch size calculation**
   - Uses 256 threads per workgroup (good)
   - Dispatch: `(count + 255) / 256` groups (correct)

### Render() Function - ❌ MAJOR BUG FOUND
```cpp
void MegaGeometryRenderer::Render(VkCommandBuffer cmd) {
    // ...
    // Swap buffers (NITRO Double Buffering)
    m_frameIndex = (m_frameIndex + 1) % 2;  // ❌ WRONG! Swaps AFTER PreRender
    // ...
}
```

**BUG**: Frame index is swapped in `Render()` but `PreRender()` uses `m_frameIndex`!
- PreRender() runs first, uses frame N
- Render() swaps to frame N+1, then uses frame N+1
- **Result**: PreRender and Render use DIFFERENT buffers!

**Fix**: Move frame swap to START of PreRender() or END of Render()

### Additional Issues:
4. **Stats calculation uses wrong buffer**
   - GetStats() reads `m_indirectMapped[0]->instanceCount`
   - But this is the INPUT instance count, not OUTPUT from culling
   - Should read from indirect buffer AFTER culling to see actual visible count

---

## ISSUE 4: Asset File Locations ❌ NEEDS VERIFICATION
**Status**: Need to verify all JSON files are in correct Android assets location

**Required Files**:
```
android/app/src/main/assets/
├── data/
│   ├── LevelDefinitions.json ✅ EXISTS
│   └── GameDataTable.json ✅ EXISTS
├── Assets/
│   ├── fps_arena.json ❓ NEED TO CREATE
│   ├── scene.json ❓ NEED TO CREATE
│   ├── main_menu.json ❓ NEED TO CREATE
│   └── racing_track.json ❓ NEED TO CREATE
└── levels/
    ├── test_level.json ✅ EXISTS
    ├── OpenWorld_Zone1.json ❓ NEED TO CREATE
    └── OpenWorld_Zone2.json ❓ NEED TO CREATE
```

**Action**: Copy existing level JSON files to Android assets with correct names

---

## IMPLEMENTATION PLAN

### Phase 1: Fix Asset Files (5 minutes)
1. Check if `Assets/` folder exists in android assets
2. Copy/rename existing level files to match LevelDefinitions.json paths
3. Create missing level JSON files (can be minimal/empty for testing)

### Phase 2: Fix GPU Culling (10 minutes)
1. Move frame index swap to correct location
2. Add debug logging to verify compute shader is running
3. Verify view-projection matrix is being set correctly
4. Fix stats calculation to read actual visible count

### Phase 3: Add Debug Logging (5 minutes)
1. Add logging in LevelManager::LoadLevelDefinitions() to show file paths
2. Add logging in LevelLoader to show which files are being loaded
3. Add logging in PreRender() to show culling results

### Phase 4: Test (User)
1. User tests level switching
2. User tests joystick (should already work)
3. User tests GPU culling by rotating camera

---

## PRIORITY ORDER
1. **HIGHEST**: Fix GPU culling frame index bug (breaks rendering)
2. **HIGH**: Fix asset file locations (breaks level loading)
3. **MEDIUM**: Add debug logging (helps diagnose issues)
4. **LOW**: Joystick (already working, just needs testing)

---

## ESTIMATED TIME
- Total implementation: 20-30 minutes
- Testing: 5-10 minutes
- **Total**: 25-40 minutes
