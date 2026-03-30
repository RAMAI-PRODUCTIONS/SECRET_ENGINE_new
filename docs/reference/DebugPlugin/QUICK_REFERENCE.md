# DebugPlugin Quick Reference Card

## 🚀 Quick Start

### View Logs
```bash
adb logcat -s Profiler DebugPlugin
```

### View Only Warnings
```bash
adb logcat -s Profiler:W DebugPlugin:W
```

## 📊 Log Format

### Every 1 Second
```
[PERF] Frame:3600 | DT:16.6ms | FPS:60(avg:59) | CPU:12.3ms GPU:4.3ms
[RENDER] TRI:125k INST:45 DRAW:12 | PIPE:8 DESC:15 | VRAM:256MB
[MEMORY] SYS:128MB | ARENA:64MB | POOL:75.5% | RAM_FREE:2048MB
[HARDWARE] BATT:85% | THERMAL:NONE
```

## ⚠️ Warning Triggers

| Warning | Trigger | Target |
|---------|---------|--------|
| LOW FPS | < 30 FPS | 60 FPS |
| HIGH CPU | > 16.6ms | < 16.6ms |
| THERMAL | SEVERE/CRITICAL | NONE |
| LOW MEMORY | < 512MB | > 1GB |

## 🎯 Performance Targets (60 FPS)

| Metric | Target | Max |
|--------|--------|-----|
| Frame Time | 16.6ms | 16.6ms |
| CPU Time | < 12ms | 16.6ms |
| GPU Time | < 4ms | 16.6ms |
| Draw Calls | < 50 | 100 |
| Triangles | < 250k | 500k |
| Free RAM | > 1GB | > 512MB |

## 💻 Code Usage

### Access Stats
```cpp
auto& stats = Profiler::Instance().GetStats();
float fps = stats.fps_instant.load(std::memory_order_relaxed);
```

### Track Memory
```cpp
Profiler::Instance().TrackAllocation(size);
Profiler::Instance().TrackDeallocation(size);
```

### Time Scope
```cpp
std::atomic<float> timing{0.0f};
Profiler::ScopeTimer timer("MyOp", &timing);
// ... code ...
```

### Change Interval
```cpp
Profiler::Instance().SetReportInterval(2.0f);
```

## 🔍 Common Filters

```bash
# Performance only
adb logcat -s Profiler:I | grep "\[PERF\]"

# Memory only
adb logcat -s Profiler:I | grep "\[MEMORY\]"

# Render stats only
adb logcat -s Profiler:I | grep "\[RENDER\]"

# All warnings
adb logcat Profiler:W DebugPlugin:W *:S
```

## 🛠️ Troubleshooting

| Issue | Solution |
|-------|----------|
| No output | Check `adb devices`, verify app running |
| No stats | Look for "DEBUG PLUGIN LOADED" message |
| Wrong values | Verify renderer registered as "rendering" |
| Too much output | Use `-s Profiler:W` for warnings only |

## 📈 Metrics Explained

### Frame Timing
- **DT**: Time between frames (ms)
- **FPS**: Frames per second (instant)
- **avg**: Average FPS over 1 second
- **CPU**: CPU processing time (ms)
- **GPU**: GPU rendering time (ms)

### Rendering
- **TRI**: Triangles rendered (thousands)
- **INST**: Instance count
- **DRAW**: Draw calls
- **PIPE**: Pipeline state changes
- **DESC**: Descriptor set binds
- **VRAM**: Video memory usage (MB)

### Memory
- **SYS**: System memory allocated (MB)
- **ARENA**: Arena allocator peak (MB)
- **POOL**: Memory pool occupancy (%)
- **RAM_FREE**: Available system RAM (MB)

### Hardware
- **BATT**: Battery level (%)
- **THERMAL**: Temperature status
  - NONE: Normal
  - LIGHT: Warm (> 70°C)
  - SEVERE: Hot (> 80°C)
  - CRITICAL: Danger (> 90°C)

## 🎓 Best Practices

1. ✅ Monitor during development
2. ✅ Baseline on target devices
3. ✅ Watch for warning patterns
4. ✅ Correlate with gameplay
5. ✅ Profile after major changes
6. ✅ Keep logs for comparison
7. ✅ Test on low-end devices

## 🔗 Related Files

- `README.md` - Full documentation
- `LOGCAT_GUIDE.md` - Detailed viewing guide
- `IMPROVEMENTS.md` - Change summary

## 📞 Quick Help

```bash
# Start monitoring
adb logcat -c && adb logcat -s Profiler DebugPlugin

# Save to file
adb logcat -s Profiler DebugPlugin > perf.log

# Real-time FPS
adb logcat -s Profiler:I | grep "FPS:"

# Check for issues
adb logcat -s Profiler:W DebugPlugin:W
```

---
**Tip**: Keep this card handy during development! 📌
