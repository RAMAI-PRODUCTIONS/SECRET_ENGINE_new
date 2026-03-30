# 📱 Install and Test - Final Step

## ✅ Build Complete!

The APK is ready at:
```
android/app/build/outputs/apk/debug/app-debug.apk
```

---

## 🚀 Step 1: Install

```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

**Expected output:**
```
Performing Streamed Install
Success
```

---

## 🧪 Step 2: Launch App

Tap the app icon on your device.

---

## 📊 Step 3: Watch Logs

Open a new terminal and run:
```bash
adb logcat | grep -E "(LevelManager|LevelLoader|GPU Culling|FPSUI)"
```

---

## 🎮 Step 4: Test Level Switching

### Tap these buttons:
1. **SCENE** button
2. **ARENA** button
3. **RACE** button

### What to look for in logs:
```
✅ LevelManager: Loaded 6 level definitions
✅ LevelManager: Loading level: Scene
✅ LevelLoader: File loaded successfully
✅ LevelLoader: Loaded 10 entities from scene
❌ NO "Level not found" errors
```

---

## 🎯 Step 5: Test GPU Culling

### How to rotate camera:
1. Touch screen with finger
2. Drag from LEFT edge to RIGHT edge (swipe hard!)
3. Repeat 10 times rapidly

### What to watch:
- **On screen**: Triangle count should drop from 12.5M to ~2M
- **On screen**: FPS should increase from 27 to 60+

### What to look for in logs:
```
✅ GPU Culling: VP[0]=1.000 → 0.500 → 0.000 → -0.500 → -1.000
✅ Triangle count decreasing
✅ FPS increasing
```

---

## ✅ Success Indicators

### Level System Working:
- Logs show "Loaded 6 level definitions"
- Each button loads a different level
- No "Level not found" errors

### GPU Culling Working:
- VP[0] changes dramatically (1.0 → -1.0)
- Triangle count drops from 12.5M to ~2M
- FPS increases from 27 to 60+
- Visible performance improvement

---

## 🐛 If Something's Wrong

### Level Loading Failed:
```bash
# Get detailed error
adb logcat | grep -E "(LevelManager|ERROR)"
```

Send me the output.

### GPU Culling Not Visible:
- Make sure you're swiping HARD (across entire screen)
- Do it 10+ times to rotate 180°
- Check if VP[0] is changing in logs
- If VP[0] stays near 1.0, camera isn't rotating enough

---

## 🎯 Quick Commands

```bash
# Install
adb install -r android/app/build/outputs/apk/debug/app-debug.apk

# Watch logs
adb logcat | grep -E "(LevelManager|LevelLoader|GPU Culling)"

# Clear logs (if too much spam)
adb logcat -c
```

---

## 🚀 You're Ready!

All fixes are compiled and ready to test. Install the APK and see everything working! 🎮
