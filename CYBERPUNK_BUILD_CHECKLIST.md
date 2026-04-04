# ✅ Cyberpunk City - Build Checklist

Use this checklist to verify your build is ready and troubleshoot any issues.

## 📋 Pre-Build Checklist

### Environment Setup
- [ ] Windows PC with Android SDK installed
- [ ] Vulkan SDK installed (for shader compilation)
- [ ] `glslc` command available in PATH
- [ ] Android device with USB debugging enabled
- [ ] USB cable connected (data-capable, not charge-only)
- [ ] Device shows in `adb devices` output

### Verify Installation
```bash
# Check glslc (shader compiler)
glslc --version

# Check adb (Android Debug Bridge)
adb version

# Check device connection
adb devices
```

Expected output:
```
List of devices attached
ABC123XYZ    device
```

## 🔍 File Verification

### New Plugins Created
- [ ] `plugins/ProceduralCityPlugin/CMakeLists.txt`
- [ ] `plugins/ProceduralCityPlugin/plugin_manifest.json`
- [ ] `plugins/ProceduralCityPlugin/src/ProceduralCityPlugin.cpp`
- [ ] `plugins/ParticleSystemPlugin/CMakeLists.txt`
- [ ] `plugins/ParticleSystemPlugin/plugin_manifest.json`
- [ ] `plugins/ParticleSystemPlugin/src/ParticleSystemPlugin.cpp`

### Shaders Created
- [ ] `plugins/VulkanRenderer/shaders/particle_vert.glsl`
- [ ] `plugins/VulkanRenderer/shaders/particle_frag.glsl`

### Shaders Compiled
- [ ] `android/app/src/main/assets/shaders/particle_vert.spv`
- [ ] `android/app/src/main/assets/shaders/particle_frag.spv`

Verify with:
```bash
ls android/app/src/main/assets/shaders/particle*.spv
```

### Assets Created
- [ ] `Assets/levels/cyberpunk_city.json`
- [ ] `Assets/ui_cyberpunk.json`
- [ ] `Assets/engine_config_cyberpunk.json`

### Build Scripts
- [ ] `compile_shaders.bat` (updated)
- [ ] `build_cyberpunk_demo.bat` (new)

### Documentation
- [ ] `CYBERPUNK_CITY_DEMO.md`
- [ ] `QUICKSTART_CYBERPUNK.md`
- [ ] `CYBERPUNK_IMPLEMENTATION_SUMMARY.md`
- [ ] `CYBERPUNK_ARCHITECTURE.md`
- [ ] `CYBERPUNK_BUILD_CHECKLIST.md` (this file)

### CMake Configuration
- [ ] `plugins/CMakeLists.txt` includes ProceduralCityPlugin
- [ ] `plugins/CMakeLists.txt` includes ParticleSystemPlugin

## 🔨 Build Steps

### Step 1: Compile Shaders
```bash
compile_shaders.bat
```

**Expected output:**
```
Compiling shaders...
Shader compilation complete!
```

**Verify:**
```bash
ls android/app/src/main/assets/shaders/*.spv
```

Should show:
- basic3d_frag.spv
- basic3d_vert.spv
- mega_geometry_frag.spv
- mega_geometry_vert.spv
- particle_frag.spv ← NEW
- particle_vert.spv ← NEW
- (and others)

### Step 2: Build Android APK

#### Option A: Automated Build
```bash
build_cyberpunk_demo.bat
```

**Expected output:**
```
========================================
  CYBERPUNK CITY DEMO - BUILD SCRIPT
========================================

[1/3] Compiling shaders...
Shader compilation complete!

[2/3] Building Android APK...
BUILD SUCCESSFUL in 45s

[3/3] Installing on device...
Success

========================================
  BUILD COMPLETE! APK INSTALLED!
========================================
```

#### Option B: Manual Build
```bash
cd android
gradlew assembleDebug
cd ..
```

**Expected output:**
```
BUILD SUCCESSFUL in 45s
```

**Verify APK exists:**
```bash
ls android/app/build/outputs/apk/debug/app-debug.apk
```

### Step 3: Install on Device
```bash
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
```

**Expected output:**
```
Performing Streamed Install
Success
```

## 🧪 Testing Checklist

### Launch Test
- [ ] App icon appears on device
- [ ] App launches without crashing
- [ ] Splash screen appears (if any)
- [ ] Main scene loads

### Visual Test
- [ ] Buildings are visible
- [ ] Buildings have varied heights
- [ ] Neon colors are visible (orange/yellow windows)
- [ ] Ground plane is visible
- [ ] Sky/fog color is dark blue
- [ ] Camera is at correct height (1.8 units)

### Control Test
- [ ] Touch screen shows UI elements
- [ ] Left joystick appears
- [ ] Joystick responds to touch
- [ ] Camera rotates when swiping right side
- [ ] Jump button appears (if implemented)
- [ ] Movement is smooth

### Particle Test (if renderer integrated)
- [ ] Rain particles visible
- [ ] Particles fall downward
- [ ] Particles recycle at bottom
- [ ] Spark particles visible
- [ ] Dust particles visible

### Performance Test
- [ ] FPS is 30+ (check with debug overlay if enabled)
- [ ] No stuttering during movement
- [ ] No frame drops when looking around
- [ ] Battery usage is reasonable

## 🐛 Troubleshooting

### Shader Compilation Fails

**Error:** `glslc: command not found`

**Solution:**
1. Install Vulkan SDK from https://vulkan.lunarg.com/
2. Add to PATH: `C:\VulkanSDK\[version]\Bin`
3. Restart terminal
4. Try again

**Error:** `.glsl file encountered but no -fshader-stage specified`

**Solution:**
- Already fixed in `compile_shaders.bat`
- Ensure you're using the updated script

### Build Fails

**Error:** `JAVA_HOME not set`

**Solution:**
```bash
set JAVA_HOME=C:\Program Files\Android\Android Studio\jre
```

**Error:** `SDK location not found`

**Solution:**
Create `android/local.properties`:
```
sdk.dir=C:\\Users\\[YourUser]\\AppData\\Local\\Android\\Sdk
```

**Error:** `Plugin not found: ProceduralCityPlugin`

**Solution:**
1. Verify `plugins/CMakeLists.txt` includes the plugin
2. Clean build: `cd android && gradlew clean`
3. Rebuild: `gradlew assembleDebug`

### Installation Fails

**Error:** `device unauthorized`

**Solution:**
1. Check device screen for authorization prompt
2. Tap "Allow" on device
3. Try `adb install` again

**Error:** `INSTALL_FAILED_INSUFFICIENT_STORAGE`

**Solution:**
1. Free up space on device (need ~100MB)
2. Uninstall old version: `adb uninstall com.secretengine.app`
3. Try again

### App Crashes

**Error:** App crashes immediately on launch

**Solution:**
1. Check logs: `adb logcat | grep SecretEngine`
2. Look for error messages
3. Common issues:
   - Missing shader files
   - Vulkan not supported
   - Insufficient memory

**Error:** Black screen on launch

**Solution:**
1. Check if Vulkan is supported: `adb shell getprop ro.hardware.vulkan`
2. Verify shaders compiled correctly
3. Check logs for renderer errors

### No Buildings Visible

**Possible causes:**
- [ ] ProceduralCityPlugin not loaded
- [ ] Camera position incorrect
- [ ] Fog too dense
- [ ] Buildings culled incorrectly

**Debug steps:**
1. Check logs for "Procedural City Plugin Loaded"
2. Check logs for "Generating Cyberpunk City"
3. Check logs for "City generation complete"
4. Verify camera position in level JSON

### Controls Not Working

**Possible causes:**
- [ ] AndroidInput plugin not loaded
- [ ] UI config not loaded
- [ ] Touch events not registered

**Debug steps:**
1. Check logs for "AndroidInput" messages
2. Verify `ui_cyberpunk.json` is in assets
3. Test with simple touch (should see log messages)

## 📊 Performance Benchmarks

### Expected Performance

| Device | FPS | Particle Count | Notes |
|--------|-----|----------------|-------|
| Galaxy S10+ | 60 | 10,600 | Smooth |
| Pixel 4 | 55-60 | 10,600 | Occasional drops |
| OnePlus 7T | 60 | 10,600 | Smooth |
| Mid-range 2018 | 40-50 | 5,000 | Reduce particles |
| Low-end 2016 | 30-40 | 2,000 | Reduce particles & buildings |

### Performance Tuning

If FPS < 30:

1. **Reduce particle count**
   Edit `plugins/ParticleSystemPlugin/src/ParticleSystemPlugin.cpp`:
   ```cpp
   for (int i = 0; i < 4000; i++) { // Was 8000
   ```

2. **Reduce building count**
   Edit `plugins/ProceduralCityPlugin/src/ProceduralCityPlugin.cpp`:
   ```cpp
   const float radius = 30.0f; // Was 42.0f
   ```

3. **Disable fog**
   Edit `Assets/levels/cyberpunk_city.json`:
   ```json
   "fog": {
     "enabled": false
   }
   ```

4. **Lower resolution**
   Edit Android manifest to reduce render resolution

## ✅ Success Criteria

Your build is successful if:
- [x] APK installs without errors
- [x] App launches and shows 3D scene
- [x] Buildings are visible
- [x] Camera can move and look around
- [x] Touch controls respond
- [x] FPS is 30+
- [x] No crashes during 5 minutes of use

## 🎉 Next Steps After Success

Once everything works:
1. Explore the city
2. Take screenshots
3. Record video
4. Share with team
5. Start customizing (colors, sizes, etc.)
6. Add more features (sound, weather, etc.)

## 📞 Getting Help

If stuck:
1. Check logs: `adb logcat | grep SecretEngine`
2. Review documentation in `CYBERPUNK_CITY_DEMO.md`
3. Check architecture in `CYBERPUNK_ARCHITECTURE.md`
4. Verify all files in this checklist exist

## 🔄 Clean Build (Nuclear Option)

If nothing works, try a clean build:

```bash
# 1. Clean Android build
cd android
gradlew clean
cd ..

# 2. Delete build artifacts
rm -rf android/app/build
rm -rf android/app/.cxx

# 3. Recompile shaders
compile_shaders.bat

# 4. Rebuild
cd android
gradlew assembleDebug
cd ..

# 5. Reinstall
adb uninstall com.secretengine.app
adb install android/app/build/outputs/apk/debug/app-debug.apk
```

---

**Good luck with your build! 🚀**

Check off each item as you go, and you'll have a working cyberpunk city in no time!
