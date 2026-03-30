# UI Level Switching System

## Overview
The UI Level Switching system provides touch-based buttons at the top of the screen to switch between different game levels. This integrates the AndroidInput plugin, FPSUIPlugin, and LevelSystem to create a seamless level switching experience.

## Architecture

### Components
1. **AndroidInput Plugin** - Detects touch input in UI button zones
2. **FPSUIPlugin** - Handles button press events and triggers level switching
3. **LevelSystem** - Manages level loading/unloading/showing/hiding

### Data Flow
```
Touch Input → AndroidInput → FPSUIPlugin → LevelSystem
```

## UI Button Layout

The top 15% of the screen is divided into 4 equal-width buttons:

```
┌─────────┬─────────┬─────────┬─────────┐
│ MainMenu│  Scene  │FPS_Arena│ Racing  │  ← Top 15% of screen
├─────────┴─────────┴─────────┴─────────┤
│                                        │
│                                        │
│         Game Content Area              │
│                                        │
│                                        │
└────────────────────────────────────────┘
```

### Button Zones
- **Button 1** (0% - 25%): MainMenu
- **Button 2** (25% - 50%): Scene
- **Button 3** (50% - 75%): FPS_Arena
- **Button 4** (75% - 100%): RacingTrack

## Implementation Details

### 1. Touch Detection (AndroidInput)

```cpp
void HandleTouch(int action, float x, float y, int pointerId, 
                 float screenWidth, float screenHeight) {
    // Check if touch is in UI zone (top 15%)
    if (y < screenHeight * 0.15f) {
        float buttonWidth = screenWidth / 4.0f;
        
        if (x < buttonWidth) {
            strncpy(m_uiButtonPressed, "MainMenu", 64);
        }
        else if (x < buttonWidth * 2) {
            strncpy(m_uiButtonPressed, "Scene", 64);
        }
        else if (x < buttonWidth * 3) {
            strncpy(m_uiButtonPressed, "FPS_Arena", 64);
        }
        else {
            strncpy(m_uiButtonPressed, "RacingTrack", 64);
        }
        return; // Don't process as game input
    }
    
    // ... rest of game input handling
}
```

### 2. Button Press Routing (AndroidInput)

```cpp
void OnUpdate(float dt) override {
    // Process UI button presses
    if (m_uiButtonPressed[0] != '\0') {
        SendUIButtonPress(m_uiButtonPressed);
        m_uiButtonPressed[0] = '\0';
    }
}

void SendUIButtonPress(const char* buttonName) {
    auto* fpsUIPlugin = m_core->GetCapability("fps_ui");
    if (fpsUIPlugin) {
        auto* fpsUI = static_cast<FPSUI::FPSUIPlugin*>(fpsUIPlugin);
        fpsUI->OnButtonPress(buttonName);
    }
}
```

### 3. Level Switching Logic (FPSUIPlugin)

```cpp
void OnButtonPress(const char* buttonName) {
    // Map button name to level name
    if (strcmp(buttonName, "MainMenu") == 0) {
        SwitchToLevel("MainMenu");
    }
    else if (strcmp(buttonName, "Scene") == 0) {
        SwitchToLevel("Scene");
    }
    // ... etc
}

void SwitchToLevel(const char* levelName) {
    // Handle current level
    if (strcmp(m_currentLevel, "MainMenu") == 0) {
        // Menu is persistent, just hide it
        m_levelManager->HideLevel(m_currentLevel);
    } else if (m_currentLevel[0] != '\0') {
        // Game levels are streaming, unload them
        m_levelManager->UnloadLevel(m_currentLevel);
    }
    
    // Load new level
    if (strcmp(levelName, "MainMenu") == 0) {
        // Menu is already loaded, just show it
        m_levelManager->ShowLevel(levelName);
    } else {
        // Load game level
        m_levelManager->LoadLevel(levelName, LoadPriority::Immediate);
    }
    
    // Update current level
    strncpy(m_currentLevel, levelName, 64);
}
```

## Level Types and Behavior

### Persistent Levels (MainMenu)
- **Always loaded** in memory
- Never unloaded, only hidden/shown
- Fast switching (no loading time)
- Use for: Main menus, HUDs, persistent UI

### Streaming Levels (Scene, FPS_Arena, RacingTrack)
- **Loaded on demand**
- Unloaded when switching away
- May have loading time
- Use for: Game levels, large environments

## Usage Examples

### Adding a New Level Button

1. **Add level to LevelDefinitions.json**:
```json
{
  "name": "NewLevel",
  "path": "Assets/new_level.json",
  "type": "streaming",
  "streamingMethod": "manual",
  "autoLoad": false,
  "blockOnLoad": true,
  "makeVisibleAfterLoad": true
}
```

2. **Update UI button layout** (if needed):
   - Adjust button count in AndroidInput::HandleTouch()
   - Update button width calculation

3. **Add button handler in FPSUIPlugin**:
```cpp
void OnButtonPress(const char* buttonName) {
    // ... existing buttons
    else if (strcmp(buttonName, "NewLevel") == 0) {
        SwitchToLevel("NewLevel");
    }
}
```

### Customizing Button Zones

To change button layout, modify `AndroidInput::HandleTouch()`:

```cpp
// Example: 3 buttons instead of 4
float buttonWidth = screenWidth / 3.0f;

if (x < buttonWidth) {
    strncpy(m_uiButtonPressed, "Button1", 64);
}
else if (x < buttonWidth * 2) {
    strncpy(m_uiButtonPressed, "Button2", 64);
}
else {
    strncpy(m_uiButtonPressed, "Button3", 64);
}
```

### Changing UI Zone Height

To adjust the UI button area height:

```cpp
// Change from 15% to 20%
if (y < screenHeight * 0.20f) {
    // UI button zone
}
```

## Performance Considerations

### Level Switching Performance
- **Persistent → Persistent**: Instant (hide/show only)
- **Persistent → Streaming**: ~100-500ms (load time)
- **Streaming → Streaming**: ~200-1000ms (unload + load)
- **Streaming → Persistent**: ~100ms (unload + show)

### Memory Management
- Persistent levels stay in memory (use sparingly)
- Streaming levels are unloaded when not active
- Only one game level active at a time (current implementation)

### Optimization Tips
1. Keep persistent levels small (UI only)
2. Use streaming for all game content
3. Preload next level in background (future feature)
4. Use level streaming distance for open worlds

## Testing

### Manual Testing
1. Build and deploy to Android device
2. Tap top-left corner → Should switch to MainMenu
3. Tap top-center-left → Should switch to Scene
4. Tap top-center-right → Should switch to FPS_Arena
5. Tap top-right corner → Should switch to RacingTrack

### Log Output
Look for these log messages:
```
[FPSUI] 🔘 Button pressed: MainMenu
[FPSUI] 🎮 Switching from Scene to MainMenu
[LevelSystem] Hiding level: Scene
[LevelSystem] Showing level: MainMenu
[FPSUI] ✅ Now in level: MainMenu
```

## Troubleshooting

### Button Not Responding
- Check touch coordinates are in top 15% of screen
- Verify FPSUIPlugin is loaded and activated
- Check log for "Button pressed" messages

### Level Not Switching
- Verify level exists in LevelDefinitions.json
- Check LevelSystem is initialized
- Look for error messages in logs

### Wrong Level Loads
- Verify button name matches level name exactly
- Check button zone calculations
- Test with different screen sizes

## Future Enhancements

### Visual Feedback
- Add button highlight on press
- Show button labels/text
- Add button icons
- Animate button presses

### Advanced Features
- Button enable/disable states
- Loading progress indicators
- Smooth level transitions
- Button customization via JSON

### Multi-Touch Support
- Handle multiple simultaneous button presses
- Gesture support (swipe to switch levels)
- Long-press for level options

## Related Documentation
- [Level System](LEVEL_SYSTEM.md)
- [Multiple Levels Guide](MULTIPLE_LEVELS_GUIDE.md)
- [Level Switching Example](LEVEL_SWITCHING_EXAMPLE.md)
- [Scene to Level Migration](SCENE_TO_LEVEL_MIGRATION.md)

## File Locations
- **AndroidInput**: `plugins/AndroidInput/src/InputPlugin.h/cpp`
- **FPSUIPlugin**: `plugins/FPSUIPlugin/src/FPSUIPlugin.h/cpp`
- **LevelSystem**: `plugins/LevelSystem/src/LevelManager.h/cpp`
- **Level Definitions**: `data/LevelDefinitions.json`
