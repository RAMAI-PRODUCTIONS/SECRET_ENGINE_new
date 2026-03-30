# 6M Triangle Rendering Optimization

## Changes Made

### Instance Count Increased: 1000 → 4000
**File**: `plugins/VulkanRenderer/src/RendererPlugin.cpp`

**Before**:
- 1000 instances spawned in sphere (radius 1500)
- ~1.5M triangles rendered
- 120+ FPS on Adreno 619

**After**:
- 4000 instances spawned in sphere (radius 2000)
- ~6M triangles target
- Expected: 60-90 FPS on Adreno 619

### Changes:
1. Loop count: `for(int i = 0; i < 1000; i++)` → `for(int i = 0; i < 4000; i++)`
2. Sphere radius: `300.0f - 1500.0f` → `300.0f - 2000.0f` (larger distribution)
3. Log message updated to reflect 4000 instances and 6M triangle target

## System Capacity

The MegaGeometry system supports:
- **MAX_INSTANCES**: 65,536 (we're using 4000 = 6% capacity)
- **MAX_VERTICES**: 1,000,000
- **MAX_INDICES**: 3,000,000
- **MAX_MESHES**: 256

**Current usage is well within limits.**

## Expected Performance

### Current Optimizations Active:
✅ GPU-driven instanced rendering (4000 instances, 2 draw calls)
✅ Frustum culling (automatic triangle reduction based on view)
✅ Batch transform updates (1000 instances/frame)
✅ Persistent-mapped buffers (zero-copy updates)
✅ Double-buffered instance data (no stalls)

### Performance Prediction:
- **Best case**: 90 FPS (all instances visible, no thermal throttling)
- **Typical**: 60-75 FPS (partial culling, normal conditions)
- **Worst case**: 45-60 FPS (all visible, thermal throttling)

## Monitoring Performance

Use the DebugPlugin to monitor real-time stats:

```bash
adb logcat -s Profiler | grep "\[RENDER\]"
```

**Key metrics to watch**:
- `Tris`: Should show ~6M when all instances are visible
- `Instances`: Should show 4000
- `DrawCalls`: Should remain at 2 (instanced rendering)
- `FPS`: Target 60+

**Example output**:
```
[RENDER] Tris:6000000 Inst:4000 Draws:2 Pipe:1 Desc:1 VRAM:245MB
```

## Build and Test

```bash
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb logcat -s Profiler VulkanRenderer
```

## Further Optimizations (if needed)

If performance drops below 60 FPS:

1. **LOD System** (50-70% triangle savings)
   - Reduce mesh detail based on distance
   - Swap to lower poly models at 500m+

2. **Occlusion Culling** (30-50% savings)
   - Skip rendering objects behind other objects
   - Requires compute shader pass

3. **Reduce Sphere Radius** (quick fix)
   - Change back to 1500 radius
   - Keeps density but reduces visible count

4. **Increase Culling Aggressiveness**
   - Tighten frustum culling margins
   - Early-out for distant objects

## Notes

- The system uses Y-axis rotation only for 10x faster matrix construction
- Batch updates minimize cache misses
- Persistent mapping eliminates CPU→GPU copy overhead
- Double buffering prevents GPU stalls
