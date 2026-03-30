# Android Deployment Guide

## ✅ Successfully Deployed!

The app has been built and installed on device: **ZD222JHZ2N**

## Quick Commands

### Full Build and Deploy
```bash
deploy_android.bat
```

### Quick Reinstall (if already built)
```bash
quick_deploy.bat
```

### Update UI Config Only
```bash
update_ui_config.bat
```

### Manual Commands

**Build APK:**
```bash
cd android
gradlew.bat assembleDebug
cd ..
```

**Install APK:**
```bash
adb install -r android\app\build\outputs\apk\debug\app-debug.apk
```

**Check Connected Devices:**
```bash
adb devices
```

**View Logs:**
```bash
adb logcat | findstr "SecretEngine"
```

## Testing the UI Configuration

### Current Setup
The app is deployed with the default `ui_config.json` which includes:
- 4 UI buttons at the top (MENU, SCENE, ARENA, RACE)
- Joystick in bottom-left corner
- Touch input for camera look and fire on right side

### To Test Joystick-Only Mode

1. Copy the joystick-only config:
   ```bash
   copy ui_config_joystick_only.json ui_config.json
   ```

2. Rebuild and deploy:
   ```bash
   deploy_android.bat
   ```

### To Modify UI at Runtime

1. Edit `ui_config.json` in the project root
2. Copy to assets:
   ```bash
   copy ui_config.json android\app\src\main\assets\ui_config.json
   ```
3. Rebuild and deploy

## UI Configuration Options

### Hide All Buttons
Set in `ui_config.json`:
```json
"screen_zones": {
  "ui_buttons_zone": {
    "enabled": false
  }
}
```

### Move Joystick
```json
"joystick": {
  "position": {
    "x_ndc": -0.7,
    "y_ndc": 0.5
  }
}
```

### Adjust Sensitivity
```json
"touch_input": {
  "joystick_sensitivity": 2.0,
  "look_sensitivity": 15.0
}
```

## Troubleshooting

### Device Not Detected
```bash
adb kill-server
adb start-server
adb devices
```

### Build Errors
Clean and rebuild:
```bash
cd android
gradlew.bat clean
gradlew.bat assembleDebug
cd ..
```

### App Crashes
View logs:
```bash
adb logcat -c  # Clear logs
adb logcat | findstr "SecretEngine"
```

### UI Not Updating
Make sure `ui_config.json` is copied to:
```
android/app/src/main/assets/ui_config.json
```

## APK Location

Debug APK: `android\app\build\outputs\apk\debug\app-debug.apk`

## Next Steps

1. Launch the app on your device
2. Test touch input and joystick
3. Try pressing the UI buttons to switch levels
4. Modify `ui_config.json` and redeploy to test different layouts

## File Locations

- **UI Config**: `ui_config.json` (project root)
- **Assets Copy**: `android/app/src/main/assets/ui_config.json`
- **Source Code**: `plugins/AndroidInput/src/UIConfig.h/cpp`
- **Documentation**: `UI_CONFIG_README.md`
