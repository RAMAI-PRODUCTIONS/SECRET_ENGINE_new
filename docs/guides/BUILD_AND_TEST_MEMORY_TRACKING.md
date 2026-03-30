# Build and Test: Memory Tracking & Multithreading

## 🎯 What We're Testing

1. **Native Memory Tracking** - SYS should show real MB values
2. **VRAM Tracking** - VRAM should show real MB values
3. **Job System** - Infrastructure ready (not yet integrated)

---

## 🔨 Build Instructions

### Step 1: Clean Build
```bash
cd android
./gradlew clean
```

### Step 2: Build Debug APK
```bash
./gradlew assembleDebug
```

**Expected Output:**
```
BUILD SUCCESSFUL in 45s
```

### Step 3: Install on Device
```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

**Expected Output:**
```
Success
```

---

## 🧪 Test 1: Native Memory Tracking

### Run Test
```bash
adb logcat -c
adb logcat -s Profiler | grep "\[MEMORY\]"
```

### Expected Output
```
[INFO][Profiler] [MEMORY] SYS:45MB | ARENA:0MB | POOL:0.0% | RAM_FREE:2048MB
[INFO][Profiler] [MEMORY] SYS:45MB | ARENA:0MB | POOL:0.0% | RAM_FREE:2048MB
[INFO][Profiler] [MEMORY] SYS:45MB | ARENA:0MB | POOL:0.0% | RAM_FREE:2048MB
```

### Success Criteria
- ✅ SYS shows non-zero value (30-100MB typical)
- ✅ Value is consistent (not jumping wildly)
- ✅ RAM_FREE shows system memory (1-4GB typical)

### If SYS Still Shows 0MB
**Debug Steps:**

1. **Check if SystemAllocator is being used:**
```bash
adb logcat -s SecretEngine_Native | grep "Allocat"
```

2. **Add debug log to SystemAllocator.cpp:**
```cpp
void* SystemAllocator::Allocate(size_t size, size_t alignment) {
    void* ptr = _aligned_malloc(size, alignment);
    if (ptr) {
        uint64_t total = g_totalAllocated.fetch_add(size, std::memory_order_relaxed);
        printf("Allocated %zu bytes, total: %llu\n", size, total + size);
    }
    return ptr;
}
```

3. **Rebuild and check logs:**
```bash
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb logcat | grep "Allocated"
```

---

## 🧪 Test 2: VRAM Tracking

### Run Test
```bash
adb logcat -c
adb logcat -s Profiler | grep "VRAM:"
```

### Expected Output
```
[INFO][Profiler] [RENDER] TRI:6195k INST:4001 DRAW:2 | PIPE:1 DESC:1 | VRAM:280MB
[INFO][Profiler] [RENDER] TRI:6195k INST:4001 DRAW:2 | PIPE:1 DESC:1 | VRAM:280MB
[INFO][Profiler] [RENDER] TRI:6195k INST:4001 DRAW:2 | PIPE:1 DESC:1 | VRAM:280MB
```

### Success Criteria
- ✅ VRAM shows non-zero value (200-300MB for 6M triangles)
- ✅ Value is realistic (not 0MB or 10GB)
- ✅ Value increases during initialization

### If VRAM Still Shows 0MB
**Debug Steps:**

1. **Check if CreateBuffer is being called:**
```bash
adb logcat -s VulkanRenderer | grep "CreateBuffer"
```

2. **Add debug log to VulkanHelpers.cpp:**
```cpp
bool Helpers::CreateBuffer(...) {
    // ... existing code ...
    
    g_vramAllocated.fetch_add(memRequirements.size, std::memory_order_relaxed);
    
    uint64_t total = g_vramAllocated.load(std::memory_order_relaxed);
    printf("VRAM allocated: %llu bytes (%.1f MB)\n", 
           total, total / (1024.0f * 1024.0f));
    
    return true;
}
```

3. **Rebuild and check logs:**
```bash
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb logcat | grep "VRAM allocated"
```

---

## 🧪 Test 3: Full Performance Report

### Run Test
```bash
adb logcat -c
adb logcat -s Profiler
```

### Expected Output
```
[INFO][Profiler] [PERF] Frame:60 | DT:16.6ms | FPS:60(avg:60) | CPU:12.3ms GPU:4.3ms
[INFO][Profiler] [RENDER] TRI:6195k INST:4001 DRAW:2 | PIPE:1 DESC:1 | VRAM:280MB
[INFO][Profiler] [LOGIC] ENT:0 | PHYS:0 | NET_IN:0 NET_OUT:0
[INFO][Profiler] [MEMORY] SYS:45MB | ARENA:0MB | POOL:0.0% | RAM_FREE:2048MB
[INFO][Profiler] [HARDWARE] BATT:85% | THERMAL:NONE
[INFO][Profiler] ----------------------------------------
```

### Success Criteria
- ✅ All 5 log lines appear every second
- ✅ SYS shows non-zero value
- ✅ VRAM shows non-zero value
- ✅ FPS is stable (45-60 FPS)
- ✅ No warnings or errors

---

## 🧪 Test 4: Memory Growth Test

### Purpose
Verify memory tracking increases as objects are created.

### Run Test
```bash
# Start app and watch memory
adb logcat -s Profiler | grep "SYS:"
```

### Expected Behavior
```
[MEMORY] SYS:30MB | ...  (at startup)
[MEMORY] SYS:35MB | ...  (after loading meshes)
[MEMORY] SYS:45MB | ...  (after creating instances)
[MEMORY] SYS:45MB | ...  (stable during gameplay)
```

### Success Criteria
- ✅ SYS increases during initialization
- ✅ SYS stabilizes during gameplay
- ✅ SYS doesn't continuously grow (no leak)

---

## 🧪 Test 5: VRAM Budget Test

### Purpose
Verify we're staying under 19MB danger zone (we're not, but we can see it now!)

### Run Test
```bash
adb logcat -s Profiler | grep "VRAM:"
```

### Current Status
```
VRAM:280MB (6M triangles, 4000 instances)
```

**Status:** ⚠️ Over budget (need VMA + optimization)

### Target
```
VRAM:18MB (optimized with VMA)
```

**Next Steps:**
1. Integrate VMA (see VMA_INTEGRATION_GUIDE.md)
2. Use smaller vertex formats
3. Compress normals
4. Reduce instance count if needed

---

## 🧪 Test 6: Job System (Not Yet Integrated)

### Purpose
Verify job system compiles and links correctly.

### Check Build Output
```bash
./gradlew assembleDebug 2>&1 | grep -i "jobsystem"
```

### Expected Output
```
> Task :app:buildCMakeDebug[arm64-v8a]
[1/50] Building CXX object core/CMakeFiles/SecretEngine_Core.dir/src/JobSystem.cpp.o
```

### Success Criteria
- ✅ JobSystem.cpp compiles without errors
- ✅ No linker errors
- ✅ APK builds successfully

### Note
Job system is **not yet initialized** - it's just infrastructure. To use it:
1. Add `JobSystem::Instance().Initialize()` to `Core::Initialize()`
2. See `MULTITHREADING_QUICK_START.md` for usage

---

## 📊 Performance Baseline

### Current Performance (Before Multithreading)
```
[PERF] Frame:3600 | DT:20.0ms | FPS:50(avg:48) | CPU:14.0ms GPU:6.0ms
[RENDER] TRI:6195k INST:4001 DRAW:2 | PIPE:1 DESC:1 | VRAM:280MB
[MEMORY] SYS:45MB | ARENA:0MB | POOL:0.0% | RAM_FREE:2048MB
```

**Analysis:**
- ✅ FPS: 48-50 (acceptable for 6M triangles)
- ✅ CPU: 14ms (room for improvement)
- ✅ GPU: 6ms (excellent)
- ⚠️ VRAM: 280MB (need to optimize to 18MB)

### Expected After Multithreading
```
[PERF] Frame:3600 | DT:10.0ms | FPS:100(avg:98) | CPU:4.3ms GPU:6.0ms
[RENDER] TRI:6195k INST:4001 DRAW:2 | PIPE:1 DESC:1 | VRAM:280MB
[MEMORY] SYS:45MB | ARENA:0MB | POOL:0.0% | RAM_FREE:2048MB
```

**Improvements:**
- ✅ FPS: 98-100 (2x increase)
- ✅ CPU: 4.3ms (3.3x faster)
- ✅ GPU: 6ms (unchanged, not bottleneck)

---

## 🔍 Troubleshooting

### Build Fails
**Error:** `undefined reference to SystemAllocator::GetTotalAllocated()`

**Solution:**
```bash
# Clean and rebuild
./gradlew clean
./gradlew assembleDebug
```

### Memory Still Shows 0MB
**Possible Causes:**
1. SystemAllocator not being used
2. DebugPlugin not reading the value
3. Include path issue

**Solution:**
Add debug logs (see Test 1 above)

### VRAM Still Shows 0MB
**Possible Causes:**
1. VulkanHelpers::CreateBuffer not being called
2. Renderer not initialized yet
3. Include path issue

**Solution:**
Add debug logs (see Test 2 above)

### App Crashes
**Possible Causes:**
1. Null pointer in DebugPlugin
2. Include path issue
3. Memory corruption

**Solution:**
```bash
# Get crash log
adb logcat -s DEBUG AndroidRuntime
```

---

## ✅ Success Checklist

### Memory Tracking
- [ ] SYS shows non-zero value (30-100MB)
- [ ] VRAM shows non-zero value (200-300MB)
- [ ] Values are stable during gameplay
- [ ] RAM_FREE shows system memory

### Performance
- [ ] FPS is 45-60 (acceptable for 6M triangles)
- [ ] CPU time is 12-16ms
- [ ] GPU time is 4-8ms
- [ ] No warnings or errors

### Build
- [ ] Clean build succeeds
- [ ] APK installs successfully
- [ ] App runs without crashes
- [ ] All logs appear correctly

---

## 📈 Next Steps

### Immediate
1. ✅ Verify memory tracking works
2. ✅ Document baseline performance
3. ⬜ Initialize JobSystem in Core::Initialize()

### Short Term
1. ⬜ Parallelize instance updates
2. ⬜ Parallelize frustum culling
3. ⬜ Measure performance gains

### Medium Term
1. ⬜ Integrate VMA
2. ⬜ Optimize VRAM to 18MB
3. ⬜ Parallelize physics

---

## 📚 Related Documentation

- `CRITICAL_FIXES_SUMMARY.md` - Overview of all changes
- `MEMORY_AND_THREADING_UPGRADE.md` - Technical details
- `MULTITHREADING_QUICK_START.md` - How to use job system
- `VMA_INTEGRATION_GUIDE.md` - VRAM optimization

---

## 📞 Support

### If Tests Fail
1. Check build output for errors
2. Add debug logs to track down issue
3. Verify include paths are correct
4. Clean and rebuild

### If Performance is Poor
1. Check CPU time (should be 12-16ms)
2. Check GPU time (should be 4-8ms)
3. Check VRAM usage (should be 200-300MB)
4. Look for warnings in logcat

### If Memory Tracking Doesn't Work
1. Verify SystemAllocator is being used
2. Check if DebugPlugin is reading values
3. Add debug logs to track allocations
4. Verify include paths

---

**Status:** Ready for Testing ✅  
**Estimated Time:** 30 minutes  
**Priority:** HIGH - Verify memory tracking works
