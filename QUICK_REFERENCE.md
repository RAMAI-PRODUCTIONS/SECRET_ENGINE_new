# Quick Reference - SecretEngine on Android

## ✅ Current Status

**Device:** ZD222JHZ2N
**App Status:** RUNNING ✓
**Process ID:** 19128
**Window Focus:** com.secretengine.game.MainActivity

## Quick Commands

### App Control
```bash
# Launch app
adb shell am start -n com.secretengine.game/.MainActivity

# Stop app
adb shell am force-stop com.secretengine.game

# Check if running
adb shell "ps | grep secretengine"

# Check app status
check_app_status.bat
```

### Deployment
```bash
# Full build and deploy
deploy_android.bat

# Quick reinstall (if already built)
quick_deploy.bat

# Launch and monitor logs
launch_and_monitor.bat
```

### Logs
```bash
# View live logs
view_logs.bat

# View recent logs
adb logcat -d -t 100 | findstr /I "SecretEngine"

# Clear logs
adb logcat -c
```

### UI Configuration
```bash
# Edit config
notepad ui_config.json

# Copy to assets
copy ui_config.json android\app\src\main\assets\ui_config.json

# Use joystick-only mode
copy ui_config_joystick_only.json ui_config.json
```

## Testing Checklist

On your device, test:

- [ ] Touch the top UI buttons (MENU, SCENE, ARENA, RACE)
- [ ] Use the joystick in bottom-left corner for movement
- [ ] Touch and drag on right side for camera look
- [ ] Tap right side to fire
- [ ] Check if levels switch when pressing buttons

## UI Configuration Quick Edits

### Hide All Buttons
In `ui_config.json`:
```json
"screen_zones": {
  "ui_buttons_zone": {
    "enabled": false
  }
}
```

### Hide Joystick
```json
"joystick": {
  "visible": false
}
```

### Adjust Sensitivity
```json
"touch_input": {
  "joystick_sensitivity": 2.0,
  "look_sensitivity": 15.0
}
```

### Move Joystick Position
```json
"joystick": {
  "position": {
    "x_ndc": 0.7,    // -1.0 (left) to 1.0 (right)
    "y_ndc": 0.7     // -1.0 (top) to 1.0 (bottom)
  }
}
```

## Troubleshooting

### App Not Responding
```bash
adb shell am force-stop com.secretengine.game
adb shell am start -n com.secretengine.game/.MainActivity
```

### Check for Crashes
```bash
adb logcat -d | findstr /I "crash fatal"
```

### Reinstall
```bash
adb uninstall com.secretengine.game
adb install android\app\build\outputs\apk\debug\app-debug.apk
```

### Device Not Detected
```bash
adb kill-server
adb start-server
adb devices
```

## File Locations

- **UI Config:** `ui_config.json`
- **APK:** `android\app\build\outputs\apk\debug\app-debug.apk`
- **Assets:** `android\app\src\main\assets\`
- **Source:** `plugins/AndroidInput/src/`

## Documentation

- `UI_CONFIG_README.md` - Complete UI configuration guide
- `DEPLOYMENT_GUIDE.md` - Deployment instructions
- `README_FPS_GAME.md` - Game features and architecture

## Next Steps

1. Test the current UI on your device
2. Modify `ui_config.json` to customize the layout
3. Rebuild and redeploy to see changes
4. Use `view_logs.bat` to monitor app behavior
