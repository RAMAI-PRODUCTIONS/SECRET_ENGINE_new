# ⚡ DO THIS NOW - Simple Steps

## 1️⃣ Rebuild APK

```bash
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## 2️⃣ Launch App

Tap the app icon on your device.

## 3️⃣ Test Level Switching

Tap these buttons in order:
1. **SCENE** button
2. **ARENA** button  
3. **RACE** button

## 4️⃣ Check Logs

```bash
adb logcat | grep "LevelManager"
```

**Look for**:
- ✅ "Loaded 6 level definitions"
- ✅ "Loading level: Scene"
- ❌ NO "Level not found" errors

## 5️⃣ Test GPU Culling

1. Touch screen
2. Drag finger HARD from left to right (across entire screen)
3. Do this 10 times rapidly
4. Watch triangle count in debug text

**Look for**:
- Triangle count should drop from 12.5M to ~2-4M
- FPS should increase from 27 to 60+

---

## ✅ Success = Both Working

- Level buttons load different levels
- Triangle count changes with camera rotation

## ❌ Still Broken?

Send me the output of:
```bash
adb logcat | grep -E "(LevelManager|ERROR)"
```

---

That's it! Just rebuild and test. 🚀
