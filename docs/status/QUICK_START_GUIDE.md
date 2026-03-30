# Quick Start Guide - Testing Your Changes

## 🚀 Install and Test

### 1. Install APK on Device
```bash
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
```

### 2. Launch App
Tap the app icon on your device

### 3. What You Should See

#### Top of Screen (UI Buttons)
```
┌─────────┬─────────┬─────────┬─────────┐
│  MENU   │  SCENE  │  ARENA  │  RACE   │  ← Colored buttons with labels
└─────────┴─────────┴─────────┴─────────┘
```
- **Blue button (MENU)**: Switches to MainMenu
- **Green button (SCENE)**: Switches to Scene level
- **Red button (ARENA)**: Switches to FPS_Arena level
- **Orange button (RACE)**: Switches to RacingTrack level

#### Bottom-Left (Joystick)
- Gray square with white center
- Use for movement control

### 4. Test Functionality
1. **Tap each button** - Should switch levels
2. **Use joystick** - Should control movement
3. **Tap right side** - Should fire weapon

### 5. Check Logs
```bash
adb logcat | grep SecretEngine
```

Look for:
```
[FPSUI] 🔘 Button pressed: ARENA
[FPSUI] 🎮 Switching from MainMenu to FPS_Arena
[LevelSystem] Loading level: FPS_Arena
[FPSUI] ✅ Now in level: FPS_Arena
```

---

## 📚 Documentation Created

### For Blender Users
1. **`docs/BLENDER_LEVEL_WORKFLOW.md`**
   - How to export Blender scenes to SecretEngine
   - Python export script included
   - Complete workflow guide

2. **`docs/BLENDER_COLLISION_MESHES.md`**
   - How to create collision meshes in Blender
   - Performance guidelines
   - Export methods

### For Developers
3. **`docs/UI_RENDERING_COMPLETE.md`**
   - UI button rendering implementation
   - How to enhance the UI
   - Performance details

4. **`TASKS_COMPLETED_SUMMARY.md`**
   - Complete summary of all changes
   - Technical details
   - Next steps

---

## 🎯 Quick Actions

### Rebuild APK
```bash
cd android
./gradlew clean
./gradlew assembleDebug
```

### Install APK
```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

### View Logs
```bash
adb logcat | grep SecretEngine
```

### Clear Logs
```bash
adb logcat -c
```

---

## 🔧 Troubleshooting

### Buttons Not Visible
- Check if app launched successfully
- Verify APK was rebuilt after changes
- Check logs for rendering errors

### Levels Not Loading
- Verify asset files in `android/app/src/main/assets/`
- Check `LevelDefinitions.json` paths
- Look for "Level not found" errors in logs

### Touch Not Working
- Verify touch input in logs
- Check button press messages
- Test on different screen areas

---

## 📖 Next Steps

### 1. Test Current Build
- Install APK
- Verify UI is visible
- Test level switching
- Check logs for errors

### 2. Fix Any Issues
- Asset loading errors
- Level not found errors
- Touch input problems

### 3. Enhance UI
- Make joystick thumb dynamic
- Add button press feedback
- Improve text rendering

### 4. Blender Integration
- Test Python export script
- Create test level in Blender
- Export and test in engine

---

## 📁 Important Files

### Assets
- `Assets/test_level.json` - Test level with random entities
- `android/app/src/main/assets/data/LevelDefinitions.json` - Level config
- `android/app/src/main/assets/data/GameDataTable.json` - Game data

### Code
- `plugins/VulkanRenderer/src/RendererPlugin.cpp` - UI rendering
- `plugins/FPSUIPlugin/src/FPSUIPlugin.cpp` - Level switching logic
- `plugins/AndroidInput/src/InputPlugin.cpp` - Touch input

### Documentation
- `docs/BLENDER_LEVEL_WORKFLOW.md` - Blender workflow
- `docs/BLENDER_COLLISION_MESHES.md` - Collision meshes
- `docs/UI_RENDERING_COMPLETE.md` - UI implementation
- `TASKS_COMPLETED_SUMMARY.md` - Complete summary

---

## ✅ What's Working

- ✅ UI buttons visible with colors and labels
- ✅ Joystick visible in bottom-left
- ✅ Touch input detects button presses
- ✅ Level switching logic implemented
- ✅ Asset files packaged in APK
- ✅ APK built successfully

---

## 🎮 Controls

### Touch Controls
- **Top 15% of screen**: UI buttons (level switching)
- **Left side**: Joystick (movement)
- **Right side**: Look and shoot

### UI Buttons
- **Tap MENU**: Go to main menu
- **Tap SCENE**: Load Scene level
- **Tap ARENA**: Load FPS Arena level
- **Tap RACE**: Load Racing Track level

---

## 💡 Tips

1. **Check logs first** - Most issues show up in logcat
2. **Rebuild after changes** - Always clean and rebuild APK
3. **Test on device** - Emulator may have different behavior
4. **Start simple** - Test one feature at a time

---

## 🆘 Need Help?

### Common Issues

**"Level not found"**
- Check asset files are in APK
- Verify paths in LevelDefinitions.json
- Rebuild APK after adding assets

**"Buttons not responding"**
- Check touch coordinates in logs
- Verify FPSUIPlugin is loaded
- Test different screen areas

**"Joystick not moving"**
- Joystick thumb is currently static
- Input values are working (check logs)
- See UI_RENDERING_COMPLETE.md for dynamic thumb

---

## 📞 Support

For more details, see:
- `TASKS_COMPLETED_SUMMARY.md` - Complete overview
- `docs/UI_RENDERING_COMPLETE.md` - UI details
- `docs/BLENDER_LEVEL_WORKFLOW.md` - Blender workflow
- `docs/BLENDER_COLLISION_MESHES.md` - Collision meshes

---

**Ready to test? Run:**
```bash
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
```

**Then check logs:**
```bash
adb logcat | grep SecretEngine
```

Good luck! 🎮
