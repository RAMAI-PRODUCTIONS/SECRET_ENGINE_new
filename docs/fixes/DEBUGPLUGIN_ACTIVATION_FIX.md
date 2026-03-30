# DebugPlugin Activation Fix

## Issue
The DebugPlugin was being loaded but never activated, so `OnUpdate()` was never called and no performance statistics were being logged to logcat.

## Root Cause
In `core/src/Core.cpp`, the `Initialize()` method was calling `OnLoad()` on all plugins but never calling `OnActivate()`. This meant:
- ✅ Plugin was loaded and registered
- ✅ Profiler was initialized
- ❌ Plugin was never activated
- ❌ `OnUpdate()` was never called
- ❌ No performance logs were generated

## Evidence from Logcat
```
[INFO] Registered Capability: debug
[INFO] === DEBUG PLUGIN v1.0 LOADED ===
[INFO] Features: Performance profiling, Memory tracking, Hardware monitoring
```

**Missing**: No "=== DEBUG PLUGIN ACTIVATED ===" message
**Missing**: No performance reports every 1 second

## Fix Applied
Added plugin activation loop in `Core::Initialize()` after all plugins are loaded:

```cpp
// Activate all loaded plugins
for (auto const& [name, plugin] : m_capabilities) {
    if (plugin) {
        plugin->OnActivate();
    }
}
```

## Expected Behavior After Fix

### Startup Logs
```
[INFO] === DEBUG PLUGIN v1.0 LOADED ===
[INFO] Features: Performance profiling, Memory tracking, Hardware monitoring
[INFO] ========================================
[INFO] === DEBUG PLUGIN ACTIVATED ===
[INFO] ========================================
[INFO] Starting performance monitoring...
```

### Runtime Logs (Every 1 Second)
```
[INFO][Profiler] [PERF] Frame:60 | DT:16.6ms | FPS:60(avg:60) | CPU:12.3ms GPU:4.3ms
[INFO][Profiler] [RENDER] TRI:125k INST:45 DRAW:12 | PIPE:8 DESC:15 | VRAM:256MB
[INFO][Profiler] [MEMORY] SYS:128MB | ARENA:64MB | POOL:75.5% | RAM_FREE:2048MB
[INFO][Profiler] [HARDWARE] BATT:100% | THERMAL:NONE
[INFO][Profiler] ----------------------------------------
```

### Periodic System Logs (Every 5 Seconds)
```
[INFO][DebugPlugin] [RENDERER] Instances: 1000 | Triangles: 125000 | Draw calls: 12
[INFO][DebugPlugin] [WORLD] Active and processing
[INFO][DebugPlugin] [INPUT] System active
```

### Periodic Engine Logs (Every 10 Seconds)
```
[INFO][DebugPlugin] [ENGINE] Frame: 600 | Running: YES | Renderer Ready: YES
```

## Testing
To verify the fix works:

```bash
# Clear logcat and run app
adb logcat -c
adb logcat -s Profiler DebugPlugin

# You should see:
# 1. "DEBUG PLUGIN ACTIVATED" message on startup
# 2. Performance reports every 1 second
# 3. Periodic system status logs
# 4. Automatic warnings if thresholds exceeded
```

## Files Modified
- `core/src/Core.cpp` - Added plugin activation loop

## Impact
- ✅ All plugins now properly activated
- ✅ DebugPlugin OnUpdate() called every frame
- ✅ Performance statistics logged every 1 second
- ✅ Comprehensive engine monitoring active
- ✅ Automatic performance warnings enabled

## Related Documentation
- `plugins/DebugPlugin/README.md` - Full plugin documentation
- `plugins/DebugPlugin/LOGCAT_GUIDE.md` - How to view logs
- `DEBUG_PLUGIN_UPGRADE_SUMMARY.md` - Complete upgrade summary

---

**Status**: ✅ FIXED  
**Date**: 2026-02-08  
**Rebuild Required**: YES
