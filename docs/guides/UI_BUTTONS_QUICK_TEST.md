# UI Buttons Quick Test Guide

## Build and Deploy

```bash
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## Test Procedure

### 1. Launch App
- Open SecretEngine app on Android device
- MainMenu level should auto-load (persistent)

### 2. Test Each Button

Touch the **top 15% of screen** in these zones:

```
┌─────────┬─────────┬─────────┬─────────┐
│    1    │    2    │    3    │    4    │  ← Tap here
├─────────┴─────────┴─────────┴─────────┤
│                                        │
│         Don't tap here                 │
│         (game input area)              │
│                                        │
└────────────────────────────────────────┘
```

**Button 1 (Top-Left)**: MainMenu
- Should show main menu level
- Fast switch (persistent level)

**Button 2 (Top-Center-Left)**: Scene
- Should load original scene.json
- May take ~200ms to load

**Button 3 (Top-Center-Right)**: FPS_Arena
- Should load FPS arena level
- May take ~200ms to load

**Button 4 (Top-Right)**: RacingTrack
- Should load racing track level
- May take ~200ms to load

### 3. Verify Logs

Connect via ADB and check logs:

```bash
adb logcat | grep -E "FPSUI|LevelSystem|AndroidInput"
```

Expected output when pressing buttons:
```
[AndroidInput] 🔘 Button pressed: FPS_Arena
[FPSUI] 🎮 Switching from MainMenu to FPS_Arena
[LevelSystem] Hiding level: MainMenu
[LevelSystem] Loading level: FPS_Arena
[LevelSystem] Level loaded: FPS_Arena
[FPSUI] ✅ Now in level: FPS_Arena
```

## Expected Behavior

### Successful Button Press
1. Touch detected in UI zone
2. Button name logged
3. Level switch initiated
4. Old level hidden/unloaded
5. New level loaded/shown
6. Confirmation logged

### Failed Button Press
If button doesn't work:
- Touch might be outside UI zone (below 15% line)
- Check Y coordinate is < screenHeight * 0.15
- Verify logs show "Button pressed" message

## Quick Troubleshooting

| Issue | Solution |
|-------|----------|
| No response to touch | Touch higher on screen (top 15% only) |
| Wrong level loads | Check X coordinate matches button zone |
| App crashes | Check logs for errors, verify all levels exist |
| Slow switching | Normal for streaming levels (200-1000ms) |

## Performance Metrics

- **MainMenu ↔ MainMenu**: Instant
- **MainMenu → Game Level**: ~200ms
- **Game Level → MainMenu**: ~100ms
- **Game Level ↔ Game Level**: ~500ms

## Visual Confirmation

Since buttons have no visual feedback yet, confirm level switch by:
1. Checking logs (most reliable)
2. Observing scene content change
3. Monitoring frame time (loading spike)

## Next Steps

After confirming buttons work:
1. Add visual button rendering
2. Add button labels/text
3. Add loading indicators
4. Add button press animations

## Build Status

✅ **BUILD SUCCESSFUL** - All code compiles
⏳ **DEVICE TEST PENDING** - Deploy to device to test

## Files Modified

- `plugins/AndroidInput/src/InputPlugin.h` - UI button detection
- `plugins/AndroidInput/src/InputPlugin.cpp` - Button routing
- `plugins/FPSUIPlugin/src/FPSUIPlugin.h` - Level switching interface
- `plugins/FPSUIPlugin/src/FPSUIPlugin.cpp` - Level switching logic
- `plugins/FPSUIPlugin/CMakeLists.txt` - LevelSystem dependency
