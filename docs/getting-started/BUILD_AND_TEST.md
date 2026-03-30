# Build and Test Instructions

## Quick Build

### Windows (from project root)
```cmd
cd android
gradlew assembleDebug
```

### Install to Device
```cmd
adb install -r app\build\outputs\apk\debug\app-debug.apk
```

## Monitor Logs

### Clear and Monitor All Engine Logs
```bash
adb logcat -c && adb logcat -s SecretEngine_Native Profiler DebugPlugin Core VulkanRenderer
```

### Monitor Only Performance Stats
```bash
adb logcat -c && adb logcat -s Profiler DebugPlugin
```

### Monitor Only Warnings
```bash
adb logcat -s Profiler:W DebugPlugin:W
```

## What to Look For

### 1. Plugin Activation (Should appear on startup)
```
[INFO][DebugPlugin] === DEBUG PLUGIN v1.0 LOADED ===
[INFO][DebugPlugin] ========================================
[INFO][DebugPlugin] === DEBUG PLUGIN ACTIVATED ===
[INFO][DebugPlugin] ========================================
[INFO][DebugPlugin] Starting performance monitoring...
```

### 2. Performance Reports (Every 1 second)
```
[INFO][Profiler] [PERF] Frame:60 | DT:16.6ms | FPS:60(avg:60) | CPU:12.3ms GPU:4.3ms
[INFO][Profiler] [RENDER] TRI:125k INST:1000 DRAW:12 | PIPE:8 DESC:15 | VRAM:256MB
[INFO][Profiler] [MEMORY] SYS:128MB | ARENA:64MB | POOL:75.5% | RAM_FREE:2048MB
[INFO][Profiler] [HARDWARE] BATT:100% | THERMAL:NONE
[INFO][Profiler] ----------------------------------------
```

### 3. Periodic System Status (Every 5 seconds)
```
[INFO][DebugPlugin] [RENDERER] Instances: 1000 | Triangles: 125000 | Draw calls: 12
```

### 4. Engine Status (Every 10 seconds)
```
[INFO][DebugPlugin] [ENGINE] Frame: 600 | Running: YES | Renderer Ready: YES
```

## Troubleshooting

### No "ACTIVATED" Message
- Check that Core.cpp was rebuilt
- Verify the activation loop is present in Core::Initialize()

### No Performance Reports
- Check that plugin is activated (look for ACTIVATED message)
- Verify Profiler::LogReport() is being called
- Check logcat filters aren't too restrictive

### Wrong Statistics
- Verify renderer is registered and active
- Check that renderer->GetStats() is implemented
- Ensure stats are being updated by renderer

## Performance Targets

### Good Performance (60 FPS)
- Frame time: ~16.6ms
- CPU time: < 12ms
- GPU time: < 4ms
- FPS average: 58-60

### Acceptable Performance (30 FPS)
- Frame time: ~33.3ms
- CPU time: < 25ms
- GPU time: < 8ms
- FPS average: 28-30

### Warning Triggers
- FPS < 30: Low FPS warning
- CPU > 16.6ms: High CPU time warning
- Thermal >= SEVERE: Thermal warning
- RAM < 512MB: Low memory warning

## Quick Test Script

Save as `test_debug_plugin.sh`:
```bash
#!/bin/bash

echo "Building..."
cd android
./gradlew assembleDebug

echo "Installing..."
adb install -r app/build/outputs/apk/debug/app-debug.apk

echo "Clearing logcat..."
adb logcat -c

echo "Starting app..."
adb shell am start -n com.secretengine.game/.MainActivity

echo "Monitoring logs (Ctrl+C to stop)..."
adb logcat -s Profiler DebugPlugin | grep -E "(ACTIVATED|PERF|RENDER|MEMORY|HARDWARE|WARNING)"
```

Make executable:
```bash
chmod +x test_debug_plugin.sh
./test_debug_plugin.sh
```

---

**Next Steps**:
1. Build the project: `cd android && gradlew assembleDebug`
2. Install to device: `adb install -r app/build/outputs/apk/debug/app-debug.apk`
3. Monitor logs: `adb logcat -s Profiler DebugPlugin`
4. Verify you see activation message and performance reports
