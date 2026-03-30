# REBUILD AND TEST GUIDE

## ⚠️ CRITICAL: You Are Running an OLD APK

All fixes have been implemented in the code, but you need to rebuild and redeploy the APK to see them work.

---

## 🔧 STEP 1: REBUILD THE APK

Open your terminal and run:

```bash
cd android
./gradlew assembleDebug
```

**Expected output:**
```
BUILD SUCCESSFUL in Xs
```

**If build fails:**
- Check for compilation errors in the output
- Ensure all dependencies are installed
- Try `./gradlew clean` then `./gradlew assembleDebug`

---

## 📱 STEP 2: DEPLOY TO DEVICE

### Option A: Using ADB (Recommended)
```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

### Option B: Manual Install
1. Copy `android/app/build/outputs/apk/debug/app-debug.apk` to your device
2. Open the APK file on your device
3. Allow installation from unknown sources if prompted
4. Install the app

---

## 🧪 STEP 3: TEST GPU CULLING

### What to Test:
1. Launch the app
2. Load FPS Arena (tap "ARENA" button)
3. Look at the debug text showing triangle count
4. **Rotate camera to face enemies** - note the triangle count
5. **Rotate camera to face away from enemies** - triangle count should DECREASE significantly

### Expected Results:
- **Looking at enemies**: ~10-12M triangles
- **Looking away**: ~2-4M triangles (60-80% reduction)
- **FPS improvement**: Should see 2-3x FPS when looking away

### Check Logs:
```bash
adb logcat | grep "GPU Culling"
```

**Expected log output (first 3 frames only):**
```
[INFO] MegaGeometryRenderer: GPU Culling: 4000 instances, 16 workgroups, VP[0]=-0.123456
[INFO] MegaGeometryRenderer: GPU Culling: 4000 instances, 16 workgroups, VP[0]=-0.123457
[INFO] MegaGeometryRenderer: GPU Culling: 4000 instances, 16 workgroups, VP[0]=-0.123458
```

### If Culling Still Not Working:
1. Check if VP[0] value is changing when you rotate camera
2. If VP[0] is always 1.0, the view-projection matrix isn't being updated
3. Verify compute shader compiled: `adb logcat | grep "cull"`

---

## 🎮 STEP 4: TEST LEVEL SWITCHING

### What to Test:
1. Launch the app
2. Tap "MENU" button (top left)
3. Tap "SCENE" button
4. Tap "ARENA" button
5. Tap "RACE" button

### Expected Results:
- Each button press should load a different level
- Screen should show different content for each level
- No error messages about "Level not found"

### Check Logs:
```bash
adb logcat | grep "LevelManager"
```

**Expected log output:**
```
[INFO] LevelManager: Attempting to load level definitions from: data/LevelDefinitions.json
[INFO] LevelManager: Successfully parsed JSON, processing levels...
[INFO] LevelManager: Registered level: MainMenu (type: 0, path: Assets/main_menu.json)
[INFO] LevelManager: Registered level: Scene (type: 1, path: Assets/scene.json)
[INFO] LevelManager: Registered level: FPS_Arena (type: 1, path: Assets/fps_arena.json)
[INFO] LevelManager: Registered level: RacingTrack (type: 1, path: Assets/racing_track.json)
[INFO] LevelManager: Loaded 6 level definitions from data/LevelDefinitions.json
```

**When switching levels:**
```
[INFO] LevelManager: Unloading current level (keeping persistent)
[INFO] LevelManager: Loading level: Scene (priority: 1)
[INFO] LevelManager: Loading level data from: Assets/scene.json
[INFO] LevelLoader: Attempting to load level file: Assets/scene.json
[INFO] LevelLoader: File opened successfully, parsing JSON...
[INFO] LevelLoader: JSON parsed successfully
[INFO] LevelLoader: Detected scene format (entities array)
[INFO] LevelLoader: Loaded 10 entities from scene
[INFO] LevelManager: Level loaded: Scene
```

### If Level Switching Still Not Working:
1. Check if "Total levels: 0" appears in logs
2. If yes, LevelDefinitions.json isn't being loaded
3. Verify asset files exist: `adb shell ls /data/app/*/base.apk`
4. Check for file path errors in logs

---

## 🕹️ STEP 5: TEST JOYSTICK (Should Already Work)

### What to Test:
1. Touch left side of screen
2. Drag finger around
3. White inner circle should move with your finger
4. Release finger - white circle returns to center

### Expected Results:
- Joystick responds immediately to touch
- White circle follows finger movement
- Circle returns to center on release
- Character moves based on joystick input

### If Joystick Not Working:
- This was already implemented correctly
- Check if touch events are being received: `adb logcat | grep "Input"`

---

## 📊 WHAT WAS FIXED

### 1. GPU Culling Fix (MegaGeometryRenderer.cpp)
**Problem**: Frame index was swapped in `Render()` instead of `PreRender()`, causing PreRender and Render to use different buffers.

**Fix**: Moved `m_frameIndex = (m_frameIndex + 1) % 2;` to the START of `PreRender()`.

**Result**: Both PreRender and Render now use the SAME buffer, so culling works correctly.

### 2. Level Loading Fix (Asset Files)
**Problem**: Level JSON files were in wrong location - LevelDefinitions.json referenced `Assets/fps_arena.json` but files didn't exist there.

**Fix**: Created `android/app/src/main/assets/Assets/` folder and copied all level files.

**Result**: Level switching should now work correctly.

### 3. Debug Logging Added
**Added logging to:**
- LevelManager::LoadLevelDefinitions() - shows file paths
- LevelManager::LoadLevelData() - shows level data loading
- LevelLoader::LoadLevelFromFile() - shows file open status
- MegaGeometryRenderer::PreRender() - shows GPU culling info

**Result**: Easy to diagnose issues by checking logs.

---

## 🐛 TROUBLESHOOTING

### Build Fails
```bash
cd android
./gradlew clean
./gradlew assembleDebug
```

### APK Won't Install
```bash
adb uninstall com.secretengine.game
adb install app/build/outputs/apk/debug/app-debug.apk
```

### No Logs Appearing
```bash
# Clear logcat buffer
adb logcat -c

# Start fresh log capture
adb logcat | grep -E "(MegaGeometry|LevelManager|LevelLoader)"
```

### App Crashes on Launch
```bash
# Get crash logs
adb logcat | grep -E "(FATAL|AndroidRuntime)"
```

---

## ✅ SUCCESS CRITERIA

### GPU Culling Working:
- [ ] Triangle count decreases when looking away from instances
- [ ] FPS improves when looking away
- [ ] Logs show "GPU Culling: X instances, Y workgroups"

### Level Switching Working:
- [ ] All buttons load different levels
- [ ] Logs show "Loaded 6 level definitions"
- [ ] No "Level not found" errors

### Joystick Working:
- [ ] White circle moves with finger
- [ ] Circle returns to center on release
- [ ] Character moves based on input

---

## 📝 REPORT RESULTS

After testing, please report:

1. **GPU Culling**: Does triangle count change when rotating camera?
2. **Level Switching**: Do buttons load different levels?
3. **Joystick**: Does white circle move with finger?
4. **Logs**: Copy relevant log output showing success or errors

---

## 🎯 NEXT STEPS

Once all tests pass:
1. Performance should be significantly improved
2. Level switching should work smoothly
3. All controls should be responsive

If any issues remain:
1. Provide full logcat output
2. Describe exact behavior vs expected behavior
3. Include screenshots if helpful

---

## 📦 FILES THAT WERE FIXED

### Code Changes:
- `plugins/VulkanRenderer/src/MegaGeometryRenderer.cpp` (frame sync fix + logging)
- `plugins/LevelSystem/src/LevelManager.cpp` (debug logging)
- `plugins/LevelSystem/src/LevelLoader.cpp` (debug logging + error handling)

### Asset Changes:
- `android/app/src/main/assets/Assets/fps_arena.json` (copied)
- `android/app/src/main/assets/Assets/scene.json` (copied)
- `android/app/src/main/assets/Assets/main_menu.json` (copied)
- `android/app/src/main/assets/Assets/racing_track.json` (copied)

All fixes are in the code - you just need to rebuild!
