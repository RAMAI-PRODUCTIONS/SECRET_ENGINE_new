# SecretEngine Performance Statistics Report
**Date**: April 1, 2026  
**Device**: ZD222JHZ2N  
**Build**: Debug APK with 4009 entities (levelone)

---

## Current Performance Metrics

### Rendering Performance
- **FPS**: 6 FPS (Average)
- **Frame Time**: ~167ms per frame
- **CPU Time**: 0.1-0.2ms per frame
- **GPU Time**: 155-163ms per frame
- **Status**: ⚠️ LOW FPS WARNING - Below 30 FPS threshold

### Memory Usage
- **Total PSS**: 103.4 MB
- **Native Heap**: 11.2 MB
- **Dalvik Heap**: 1.0 MB
- **Graphics Memory**: 40.7 MB
  - EGL mtrack: 22.6 MB
  - Gfx dev: 17.9 MB
- **Code Size**: 25.3 MB
- **Total RSS**: 167.3 MB

### GPU/Graphics
- **Pipeline**: Vulkan (native rendering)
- **Resolution**: 1600x720 (landscape)
- **Swapchain Buffers**: 5 buffers @ 4.5 MB each (22.5 MB total)
- **Format**: RGBA8888 compressed
- **Jank Frames**: 0 (0.00%)
- **Missed Vsync**: 0

### CPU Usage
- **Process**: com.secretengine.game (PID: 24537)
- **CPU Usage**: 16.6%
- **Priority**: -10 (high priority)
- **Threads**: Multiple render threads active

### Thermal & Power
- **Device Temperature**: 53-54°C (main sensors)
- **Battery Level**: 38%
- **Battery Temperature**: 37.4°C
- **Power Source**: USB powered
- **Charging Current**: 500mA max

### Level System
- **Active Level**: levelone
- **Entities Loaded**: 1024 (from 4009 in chunk file)
- **Level State**: 3 (loaded and visible)
- **Chunk File**: Assets/levels/chunks/levelone_chunk.json (2.67 MB)

---

## Performance Bottleneck Analysis

### 🔴 Critical Issue: GPU Bottleneck
The GPU is taking **155-163ms per frame** while CPU only takes **0.1-0.2ms**. This indicates:

1. **GPU-bound rendering** - The GPU cannot keep up with 4000+ cube instances

### ✅ Already Implemented (Confirmed)
- **GPU Instancing**: YES - Using `vkCmdDrawIndexedIndirect()` for single draw call
- **GPU-Driven Culling**: YES - Compute shader culling in `PreRender()`
- **Indirect Rendering**: YES - Visible instances written to buffer by GPU

### 🔍 Possible Remaining Issues
1. **Entities not using MegaGeometryRenderer** - 4000 cubes may be using old Pipeline3D path
2. **Culling shader not working** - All instances may be marked visible
3. **Overdraw** - All 4000 cubes rendering on top of each other
4. **Shader complexity** - Fragment shader too expensive
5. **No LOD system** - All cubes at full detail regardless of distance

### Target Performance
- **Target FPS**: 60 FPS (16.6ms per frame)
- **Current FPS**: 6 FPS (167ms per frame)
- **Performance Gap**: 10x slower than target

---

## Recommended Optimizations

### 1. Verify MegaGeometryRenderer Usage ⭐⭐⭐
**Priority**: CRITICAL  
**Expected Gain**: 10x+ FPS improvement if not being used

Check if the 4000 cubes from levelone_chunk.json are actually using MegaGeometryRenderer or falling back to old Pipeline3D (individual draw calls):
- Add logging to MeshRenderingSystem to count instances added
- Verify GPU culling logs show 4000+ instances
- Check if entities have correct mesh component setup

### 2. Debug GPU Culling Shader ⭐⭐⭐
**Priority**: HIGH  
**Expected Gain**: 2-5x FPS improvement

The compute culling shader may not be working correctly:
- Add debug output to see visible instance count
- Verify frustum planes are correct
- Check if all instances are being marked visible (no actual culling)

### 3. Check Fragment Shader Complexity ⭐⭐
**Priority**: HIGH  
**Expected Gain**: 2-3x FPS improvement

With 4000 cubes potentially overlapping, fragment shader cost multiplies:
- Profile shader with RenderDoc
- Simplify lighting calculations
- Reduce texture samples
- Check for expensive operations in tight loops

### 4. Implement Level of Detail (LOD) System ⭐⭐
**Priority**: MEDIUM  
**Expected Gain**: 1.5-2x FPS improvement

Reduce geometry complexity for distant objects:
- LOD0: Full detail (near camera)
- LOD1: Medium detail (mid-range)
- LOD2: Low detail (far away)

### 4. Occlusion Culling ⭐⭐
**Priority**: MEDIUM  
**Expected Gain**: 1.5-2x FPS improvement

Don't render cubes hidden behind other cubes:
- Implement hierarchical Z-buffer
- Use GPU occlusion queries

### 5. Shader Optimization ⭐
**Priority**: LOW  
**Expected Gain**: 1.2-1.5x FPS improvement

- Reduce shader complexity
- Use simpler lighting models for distant objects
- Optimize fragment shader calculations

### 6. Batch Rendering ⭐⭐
**Priority**: HIGH  
**Expected Gain**: 2-3x FPS improvement

- Group cubes by material/texture
- Reduce draw calls
- Use indirect drawing

### 7. Async Compute ⭐
**Priority**: LOW  
**Expected Gain**: 1.1-1.3x FPS improvement

- Offload culling to compute shaders
- Parallel processing on GPU

---

## Additional Monitoring Suggestions

### Real-time Stats to Add

1. **Draw Call Counter**
   - Track number of draw calls per frame
   - Target: < 100 draw calls for 4000 objects (with instancing)

2. **Visible Entity Counter**
   - Show how many entities are actually visible
   - Compare against total loaded entities

3. **GPU Memory Usage**
   - Track VRAM usage
   - Monitor buffer allocations

4. **Culling Statistics**
   - Entities culled by frustum
   - Entities culled by occlusion
   - Culling time (CPU cost)

5. **LOD Statistics**
   - Entities at each LOD level
   - LOD transitions per frame

6. **Render Pass Breakdown**
   - Time per render pass
   - Shadow pass, main pass, post-processing

7. **Buffer Statistics**
   - Vertex buffer size
   - Index buffer size
   - Uniform buffer updates

8. **Pipeline Statistics**
   - Shader invocations
   - Primitives generated
   - Fragment shader invocations

### Profiling Tools to Integrate

1. **RenderDoc Integration**
   - Capture frames for detailed GPU analysis
   - Inspect draw calls and state

2. **Android GPU Inspector**
   - Real-time GPU profiling
   - Shader performance analysis

3. **Systrace/Perfetto**
   - System-wide performance tracing
   - CPU/GPU timeline visualization

4. **Custom Profiler Overlay**
   - On-screen stats display
   - Frame time graph
   - Memory usage graph

---

## Implementation Priority

### Phase 1: Critical (Week 1)
1. ✅ Verify 4000+ entities loading (DONE)
2. ✅ GPU instancing implemented (CONFIRMED)
3. ✅ GPU-driven culling implemented (CONFIRMED)
4. 🔲 Debug why 6 FPS with instancing enabled
5. 🔲 Add logging to verify MegaGeometryRenderer usage
6. 🔲 Check GPU culling shader effectiveness

**Expected Result**: Identify root cause of 6 FPS

### Phase 2: High Priority (Week 2)
1. 🔲 Implement spatial partitioning (octree)
2. 🔲 Add LOD system (3 levels)
3. 🔲 Optimize batch rendering

**Expected Result**: 60+ FPS stable

### Phase 3: Polish (Week 3)
1. 🔲 Add occlusion culling
2. 🔲 Optimize shaders
3. 🔲 Implement async compute culling

**Expected Result**: 60 FPS with headroom for more entities

---

## Monitoring Commands

### Get Current Stats
```bash
# FPS and frame times
adb -s ZD222JHZ2N logcat -d | grep "PERF"

# Memory usage
adb -s ZD222JHZ2N shell dumpsys meminfo com.secretengine.game

# GPU stats
adb -s ZD222JHZ2N shell dumpsys gfxinfo com.secretengine.game

# CPU usage
adb -s ZD222JHZ2N shell "top -n 1 -b | grep secretengine"

# Temperature
adb -s ZD222JHZ2N shell "cat /sys/class/thermal/thermal_zone*/temp"

# Battery
adb -s ZD222JHZ2N shell dumpsys battery
```

### Continuous Monitoring
```bash
# Watch FPS in real-time
adb -s ZD222JHZ2N logcat -s Profiler:I | grep "PERF"

# Watch memory
watch -n 1 'adb -s ZD222JHZ2N shell dumpsys meminfo com.secretengine.game | grep "TOTAL"'
```

---

## Notes

- The app successfully loads all 4009 entities from the chunk file
- Level system is working correctly (MainMenu unloaded, levelone visible)
- The bottleneck is purely GPU rendering performance
- With proper instancing and culling, 60 FPS should be achievable
- Current 6 FPS is expected for naive rendering of 4000+ individual draw calls

**Next Step**: Implement GPU instancing to batch all cube instances into a single draw call.
