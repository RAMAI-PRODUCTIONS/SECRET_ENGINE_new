# 🚀 QUICK FIX COMMANDS

## Rebuild and Deploy (Copy-Paste These)

```bash
# Navigate to android folder
cd android

# Build the APK
./gradlew assembleDebug

# Install on device
adb install -r app/build/outputs/apk/debug/app-debug.apk

# Watch logs
adb logcat | grep -E "(GPU Culling|LevelManager|LevelLoader)"
```

---

## Test GPU Culling

1. Launch app
2. Tap "ARENA" button
3. Rotate camera left/right
4. Watch triangle count change

**Expected**: Triangle count should decrease when looking away from enemies

---

## Test Level Switching

1. Launch app
2. Tap "MENU" → "SCENE" → "ARENA" → "RACE"
3. Each should load different level

**Expected**: No "Level not found" errors in logs

---

## Check Logs

```bash
# GPU Culling
adb logcat | grep "GPU Culling"

# Level Loading
adb logcat | grep "LevelManager"

# All relevant logs
adb logcat | grep -E "(MegaGeometry|LevelManager|LevelLoader)"
```

---

## If Build Fails

```bash
cd android
./gradlew clean
./gradlew assembleDebug
```

---

## If Install Fails

```bash
# Uninstall old version
adb uninstall com.secretengine.game

# Install new version
adb install app/build/outputs/apk/debug/app-debug.apk
```

---

## Success Indicators

✅ Logs show: "GPU Culling: X instances, Y workgroups"
✅ Logs show: "Loaded 6 level definitions"
✅ Triangle count changes when rotating camera
✅ Level buttons load different levels
✅ No "Level not found" errors

---

## Files That Were Fixed

- `plugins/VulkanRenderer/src/MegaGeometryRenderer.cpp` (GPU culling)
- `plugins/LevelSystem/src/LevelManager.cpp` (level loading)
- `plugins/LevelSystem/src/LevelLoader.cpp` (file loading)
- `android/app/src/main/assets/Assets/*.json` (level files)

**All fixes are in the code - just rebuild!**
