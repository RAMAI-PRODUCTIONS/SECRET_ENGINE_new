# Logcat Viewing Guide for DebugPlugin

## Quick Start

### View All Engine Logs
```bash
adb logcat -s SecretEngine_Native Profiler DebugPlugin
```

### View Only Performance Stats
```bash
adb logcat -s Profiler:I
```

### View Only Warnings
```bash
adb logcat -s Profiler:W DebugPlugin:W
```

### Clear and Monitor
```bash
adb logcat -c && adb logcat -s Profiler DebugPlugin
```

## Log Categories

### Profiler Logs
**Tag**: `Profiler`
**Content**: Performance statistics, warnings, hardware health

```
[INFO][Profiler] [PERF] Frame:3600 | DT:16.6ms | FPS:60(avg:59) | CPU:12.3ms GPU:4.3ms
[INFO][Profiler] [RENDER] TRI:125k INST:45 DRAW:12 | PIPE:8 DESC:15 | VRAM:256MB
[INFO][Profiler] [LOGIC] ENT:150 | PHYS:45 | NET_IN:12 NET_OUT:8
[INFO][Profiler] [MEMORY] SYS:128MB | ARENA:64MB | POOL:75.5% | RAM_FREE:2048MB
[INFO][Profiler] [HARDWARE] BATT:85% | THERMAL:NONE
```

### DebugPlugin Logs
**Tag**: `DebugPlugin`
**Content**: Plugin lifecycle, system status, periodic checks

```
[INFO][DebugPlugin] === DEBUG PLUGIN v1.0 LOADED ===
[INFO][DebugPlugin] === DEBUG PLUGIN ACTIVATED ===
[INFO][DebugPlugin] [RENDERER] Instances: 45 | Triangles: 125000 | Draw calls: 12
[INFO][DebugPlugin] [ENGINE] Frame: 3600 | Running: YES | Renderer Ready: YES
```

## Understanding the Output

### Performance Line
```
[PERF] Frame:3600 | DT:16.6ms | FPS:60(avg:59) | CPU:12.3ms GPU:4.3ms
```
- **Frame**: Total frames since activation
- **DT**: Delta time (time between frames)
- **FPS**: Instant FPS (average over 1 second)
- **CPU**: CPU frame time
- **GPU**: GPU frame time

**Target**: 16.6ms total for 60 FPS

### Render Line
```
[RENDER] TRI:125k INST:45 DRAW:12 | PIPE:8 DESC:15 | VRAM:256MB
```
- **TRI**: Triangles rendered (in thousands)
- **INST**: Instance count
- **DRAW**: Draw calls
- **PIPE**: Pipeline binds
- **DESC**: Descriptor set binds
- **VRAM**: Video memory usage

**Optimization Tips**:
- Keep draw calls < 100 for mobile
- Batch instances to reduce draw calls
- Monitor VRAM to avoid memory pressure

### Logic Line
```
[LOGIC] ENT:150 | PHYS:45 | NET_IN:12 NET_OUT:8
```
- **ENT**: Active entities
- **PHYS**: Physics checks this frame
- **NET_IN**: Network packets received
- **NET_OUT**: Network packets sent

### Memory Line
```
[MEMORY] SYS:128MB | ARENA:64MB | POOL:75.5% | RAM_FREE:2048MB
```
- **SYS**: System memory allocated
- **ARENA**: Arena allocator peak usage
- **POOL**: Memory pool occupancy percentage
- **RAM_FREE**: Available system RAM

**Warning Threshold**: < 512MB free RAM

### Hardware Line
```
[HARDWARE] BATT:85% | THERMAL:NONE
```
- **BATT**: Battery level (0-100%)
- **THERMAL**: Thermal status
  - `NONE`: Normal temperature
  - `LIGHT`: Slightly elevated (> 70°C)
  - `SEVERE`: High temperature (> 80°C)
  - `CRITICAL`: Dangerous temperature (> 90°C)

## Warning Messages

### Low FPS Warning
```
[WARN][Profiler] LOW FPS: Average FPS below 30!
```
**Action**: Check CPU/GPU times, reduce draw calls, optimize shaders

### High CPU Time Warning
```
[WARN][Profiler] HIGH CPU TIME: 18.5ms (target: 16.6ms for 60fps)
```
**Action**: Profile CPU-heavy systems, optimize game logic, reduce entity count

### Thermal Warning
```
[WARN][Profiler] THERMAL WARNING: Device temperature elevated!
```
**Action**: Reduce rendering quality, lower frame rate cap, optimize GPU usage

### Low Memory Warning
```
[WARN][Profiler] LOW MEMORY: Only 256MB available
```
**Action**: Free unused assets, reduce texture sizes, implement streaming

## Advanced Filtering

### Show Only Warnings and Errors
```bash
adb logcat Profiler:W DebugPlugin:W *:S
```

### Show Performance Stats Only
```bash
adb logcat -s Profiler:I | grep "\[PERF\]"
```

### Show Memory Stats Only
```bash
adb logcat -s Profiler:I | grep "\[MEMORY\]"
```

### Show Render Stats Only
```bash
adb logcat -s Profiler:I | grep "\[RENDER\]"
```

### Monitor Specific Metric
```bash
# Monitor FPS
adb logcat -s Profiler:I | grep "FPS:"

# Monitor draw calls
adb logcat -s Profiler:I | grep "DRAW:"

# Monitor memory
adb logcat -s Profiler:I | grep "RAM_FREE:"
```

## Performance Analysis

### Capture Session to File
```bash
adb logcat -s Profiler DebugPlugin > performance_log.txt
```

### Real-Time Monitoring Script
```bash
#!/bin/bash
# monitor_performance.sh
adb logcat -c
adb logcat -s Profiler:I | while read line; do
    if [[ $line == *"[PERF]"* ]]; then
        echo "$line"
    fi
done
```

### Extract FPS Data
```bash
adb logcat -s Profiler:I | grep "\[PERF\]" | awk '{print $8}' > fps_data.txt
```

## Troubleshooting

### No Output
1. Check device connection: `adb devices`
2. Verify app is running
3. Check log tags: `adb logcat | grep -i secret`

### Too Much Output
1. Use specific tags: `-s Profiler`
2. Filter by priority: `Profiler:W` (warnings only)
3. Use grep to filter content

### Missing Statistics
1. Check plugin loaded: Look for "DEBUG PLUGIN LOADED"
2. Verify renderer active: Look for "[RENDERER]" logs
3. Check activation: Look for "DEBUG PLUGIN ACTIVATED"

## Integration with Android Studio

### Logcat Window
1. Open Android Studio
2. View → Tool Windows → Logcat
3. Filter: `tag:Profiler|DebugPlugin`
4. Level: Info or higher

### Custom Filters
1. Click "+" to add filter
2. Name: "Engine Performance"
3. Log Tag: `Profiler|DebugPlugin`
4. Log Level: Info
5. Save

## Performance Targets

### Mobile (60 FPS)
- Frame time: < 16.6ms
- CPU time: < 12ms
- GPU time: < 4ms
- Draw calls: < 100
- Triangles: < 500k
- Memory: > 512MB free

### Mobile (30 FPS)
- Frame time: < 33.3ms
- CPU time: < 25ms
- GPU time: < 8ms
- Draw calls: < 200
- Triangles: < 1M
- Memory: > 256MB free

## Example Analysis Session

```bash
# 1. Clear logs and start monitoring
adb logcat -c
adb logcat -s Profiler DebugPlugin > session.log &

# 2. Run your test scenario
# ... play the game, test features ...

# 3. Stop logging (Ctrl+C)

# 4. Analyze results
grep "LOW FPS" session.log
grep "HIGH CPU" session.log
grep "\[PERF\]" session.log | tail -20

# 5. Extract average FPS
grep "\[PERF\]" session.log | awk -F'avg:' '{print $2}' | awk '{print $1}' | \
    awk '{sum+=$1; count++} END {print "Average FPS:", sum/count}'
```

## Tips

1. **Monitor during development**: Keep logcat open while testing
2. **Baseline performance**: Record stats on target devices
3. **Watch for spikes**: Sudden changes indicate issues
4. **Correlate with gameplay**: Note what's happening when warnings appear
5. **Regular profiling**: Check performance after each major change

## Related Files
- `README.md` - Full plugin documentation
- `DebugPlugin.h` - Plugin interface
- `Profiler.h` - Profiler implementation
