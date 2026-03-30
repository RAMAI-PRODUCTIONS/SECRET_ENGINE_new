# Multiple Levels Guide - Each Level with Its Own Scene

## Overview
Create multiple levels, each with its own scene.json file, and switch between them seamlessly.

## Concept

Each level = One scene.json file with its own hierarchy of entities.

```
Assets/
├── main_menu.json       → MainMenu level
├── fps_arena.json       → FPS Arena level
├── racing_track.json    → Racing level
└── boss_fight.json      → Boss level
```

## Step 1: Create Multiple Scene Files

### MainMenu Scene (`Assets/main_menu.json`)
```json
{
  "entities": [
    {
      "name": "MenuCamera",
      "transform": {
        "position": [0, 5, 10],
        "rotation": [-20, 0, 0],
        "scale": [1, 1, 1]
      },
      "isPlayerStart": true
    },
    {
      "name": "MenuBackground",
      "transform": {
        "position": [0, 0, 0],
        "rotation": [0, 0, 0],
        "scale": [20, 20, 1]
      },
      "mesh": {
        "path": "meshes/Plane.meshbin",
        "color": [0.2, 0.2, 0.3, 1.0],
        "texture": "textures/menu_bg.jpeg"
      }
    },
    {
      "name": "Logo",
      "transform": {
        "position": [0, 3, 0],
        "rotation": [0, 0, 0],
        "scale": [5, 5, 5]
      },
      "mesh": {
        "path": "meshes/Logo.meshbin",
        "color": [1.0, 1.0, 1.0, 1.0]
      }
    }
  ]
}
```

### FPS Arena Scene (`Assets/fps_arena.json`)
```json
{
  "entities": [
    {
      "name": "PlayerStart",
      "transform": {
        "position": [0, 1, 10],
        "rotation": [0, 0, 0],
        "scale": [1, 1, 1]
      },
      "isPlayerStart": true
    },
    {
      "name": "ArenaFloor",
      "transform": {
        "position": [0, 0, 0],
        "rotation": [0, 0, 0],
        "scale": [100, 1, 100]
      },
      "mesh": {
        "path": "meshes/Plane.meshbin",
        "color": [0.5, 0.5, 0.5, 1.0],
        "texture": "textures/concrete.jpeg"
      }
    },
    {
      "name": "Wall_North",
      "transform": {
        "position": [0, 5, -50],
        "rotation": [0, 0, 0],
        "scale": [100, 10, 1]
      },
      "mesh": {
        "path": "meshes/Cube.meshbin",
        "color": [0.7, 0.7, 0.7, 1.0]
      }
    },
    {
      "name": "Enemy_1",
      "transform": {
        "position": [10, 0, 0],
        "rotation": [0, 180, 0],
        "scale": [15, 15, 15]
      },
      "mesh": {
        "path": "meshes/Character.meshbin",
        "color": [1.0, 0.0, 0.0, 1.0]
      }
    }
  ]
}
```

### Racing Track Scene (`Assets/racing_track.json`)
```json
{
  "entities": [
    {
      "name": "RaceStart",
      "transform": {
        "position": [0, 1, 0],
        "rotation": [0, 0, 0],
        "scale": [1, 1, 1]
      },
      "isPlayerStart": true
    },
    {
      "name": "Track",
      "transform": {
        "position": [0, 0, 0],
        "rotation": [0, 0, 0],
        "scale": [200, 1, 50]
      },
      "mesh": {
        "path": "meshes/Plane.meshbin",
        "color": [0.3, 0.3, 0.3, 1.0],
        "texture": "textures/asphalt.jpeg"
      }
    },
    {
      "name": "Car_1",
      "transform": {
        "position": [5, 0.5, 0],
        "rotation": [0, 0, 0],
        "scale": [2, 2, 2]
      },
      "mesh": {
        "path": "meshes/Car.meshbin",
        "color": [1.0, 0.0, 0.0, 1.0]
      }
    }
  ]
}
```

## Step 2: Define Levels in LevelDefinitions.json

```json
{
  "version": "1.0.0",
  "levels": [
    {
      "name": "MainMenu",
      "path": "Assets/main_menu.json",
      "type": "persistent",
      "streamingMethod": "always",
      "autoLoad": true,
      "makeVisibleAfterLoad": true,
      "bounds": {
        "center": [0, 0, 0],
        "extents": [50, 50, 50]
      }
    },
    {
      "name": "FPS_Arena",
      "path": "Assets/fps_arena.json",
      "type": "streaming",
      "streamingMethod": "manual",
      "autoLoad": false,
      "blockOnLoad": true,
      "makeVisibleAfterLoad": true,
      "bounds": {
        "center": [0, 0, 0],
        "extents": [100, 50, 100]
      }
    },
    {
      "name": "RacingTrack",
      "path": "Assets/racing_track.json",
      "type": "streaming",
      "streamingMethod": "manual",
      "autoLoad": false,
      "blockOnLoad": true,
      "makeVisibleAfterLoad": true,
      "bounds": {
        "center": [0, 0, 0],
        "extents": [200, 50, 100]
      }
    },
    {
      "name": "BossFight",
      "path": "Assets/boss_fight.json",
      "type": "streaming",
      "streamingMethod": "manual",
      "autoLoad": false,
      "blockOnLoad": true,
      "makeVisibleAfterLoad": true,
      "bounds": {
        "center": [0, 0, 0],
        "extents": [150, 100, 150]
      }
    }
  ]
}
```

## Step 3: Level Switching Code

### Basic Level Switch
```cpp
auto* levelPlugin = static_cast<Levels::LevelSystemPlugin*>(
    core->GetCapability("level_system")
);
auto* levelManager = levelPlugin->GetLevelManager();

// Switch from MainMenu to FPS_Arena
void StartFPSGame() {
    // Hide menu (keep in memory)
    levelManager->HideLevel("MainMenu");
    
    // Load and show arena
    levelManager->LoadLevel("FPS_Arena", LoadPriority::Immediate);
    
    m_logger->LogInfo("Game", "Switched to FPS Arena");
}

// Return to menu
void ReturnToMenu() {
    // Unload arena (destroy all entities)
    levelManager->UnloadLevel("FPS_Arena");
    
    // Show menu
    levelManager->ShowLevel("MainMenu");
    
    m_logger->LogInfo("Game", "Returned to Main Menu");
}
```

### Level Switch with Transition
```cpp
void SwitchLevel(const char* fromLevel, const char* toLevel) {
    LevelTransition transition;
    strncpy(transition.fromLevel, fromLevel, 64);
    strncpy(transition.toLevel, toLevel, 64);
    
    // Transition settings
    transition.transitionDuration = 2.0f;
    transition.fadeOutDuration = 0.5f;
    transition.fadeInDuration = 0.5f;
    transition.keepPersistentLevel = true;
    transition.keepPlayerState = false;
    
    // Spawn point in new level
    transition.spawnPosition[0] = 0;
    transition.spawnPosition[1] = 1;
    transition.spawnPosition[2] = 10;
    
    levelManager->TransitionToLevel(toLevel, transition);
}

// Usage
SwitchLevel("MainMenu", "FPS_Arena");
```

### Complete Game Flow Example
```cpp
class GameManager {
public:
    void Initialize(ICore* core) {
        m_core = core;
        m_levelManager = GetLevelManager();
        m_currentLevel = "MainMenu";
    }
    
    void StartGame() {
        // Menu → Game
        if (strcmp(m_currentLevel, "MainMenu") == 0) {
            SwitchToLevel("FPS_Arena");
        }
    }
    
    void LoadRacingMode() {
        // Any level → Racing
        SwitchToLevel("RacingTrack");
    }
    
    void LoadBossFight() {
        // Any level → Boss
        SwitchToLevel("BossFight");
    }
    
    void ReturnToMenu() {
        // Any level → Menu
        SwitchToLevel("MainMenu");
    }
    
private:
    void SwitchToLevel(const char* newLevel) {
        // Unload current level (if not persistent)
        if (strcmp(m_currentLevel, "MainMenu") != 0) {
            m_levelManager->UnloadLevel(m_currentLevel);
        } else {
            m_levelManager->HideLevel(m_currentLevel);
        }
        
        // Load new level
        if (strcmp(newLevel, "MainMenu") == 0) {
            m_levelManager->ShowLevel(newLevel);
        } else {
            m_levelManager->LoadLevel(newLevel, LoadPriority::Immediate);
        }
        
        // Update current level
        strncpy(m_currentLevel, newLevel, 64);
        
        char msg[128];
        snprintf(msg, sizeof(msg), "Switched to level: %s", newLevel);
        m_core->GetLogger()->LogInfo("GameManager", msg);
    }
    
    ICore* m_core;
    LevelManager* m_levelManager;
    char m_currentLevel[64];
};
```

## Step 4: Input Handling for Level Switching

### Example: Keyboard Input
```cpp
void OnKeyPress(int key) {
    switch (key) {
        case KEY_1:
            gameManager->ReturnToMenu();
            break;
        case KEY_2:
            gameManager->StartGame();  // FPS Arena
            break;
        case KEY_3:
            gameManager->LoadRacingMode();
            break;
        case KEY_4:
            gameManager->LoadBossFight();
            break;
    }
}
```

### Example: UI Button
```cpp
void OnMenuButtonClick(const char* buttonName) {
    if (strcmp(buttonName, "PlayFPS") == 0) {
        gameManager->StartGame();
    }
    else if (strcmp(buttonName, "PlayRacing") == 0) {
        gameManager->LoadRacingMode();
    }
    else if (strcmp(buttonName, "PlayBoss") == 0) {
        gameManager->LoadBossFight();
    }
    else if (strcmp(buttonName, "Quit") == 0) {
        gameManager->ReturnToMenu();
    }
}
```

## Step 5: Level State Management

### Track Current Level
```cpp
class LevelStateManager {
public:
    void SetCurrentLevel(const char* levelName) {
        strncpy(m_currentLevel, levelName, 64);
        m_levelHistory[m_historyCount++] = std::string(levelName);
    }
    
    const char* GetCurrentLevel() const {
        return m_currentLevel;
    }
    
    const char* GetPreviousLevel() const {
        if (m_historyCount >= 2) {
            return m_levelHistory[m_historyCount - 2].c_str();
        }
        return nullptr;
    }
    
    void GoBack() {
        if (m_historyCount >= 2) {
            const char* prevLevel = GetPreviousLevel();
            if (prevLevel) {
                // Switch to previous level
                m_historyCount--;
            }
        }
    }
    
private:
    char m_currentLevel[64] = "MainMenu";
    std::string m_levelHistory[32];
    int m_historyCount = 0;
};
```

## Complete Example: FPS Game with Multiple Levels

### Level Structure
```
MainMenu (persistent)
  ├─> Tutorial (streaming)
  ├─> Level_1 (streaming)
  ├─> Level_2 (streaming)
  ├─> Level_3 (streaming)
  └─> BossFight (streaming)
```

### Level Definitions
```json
{
  "levels": [
    {
      "name": "MainMenu",
      "path": "Assets/main_menu.json",
      "type": "persistent",
      "autoLoad": true
    },
    {
      "name": "Tutorial",
      "path": "Assets/tutorial.json",
      "type": "streaming"
    },
    {
      "name": "Level_1",
      "path": "Assets/level_1.json",
      "type": "streaming"
    },
    {
      "name": "Level_2",
      "path": "Assets/level_2.json",
      "type": "streaming"
    },
    {
      "name": "Level_3",
      "path": "Assets/level_3.json",
      "type": "streaming"
    },
    {
      "name": "BossFight",
      "path": "Assets/boss_fight.json",
      "type": "streaming"
    }
  ]
}
```

### Game Flow
```cpp
class FPSGameFlow {
public:
    void StartNewGame() {
        m_currentLevelIndex = 0;
        LoadLevel("Tutorial");
    }
    
    void CompleteLevel() {
        const char* levels[] = {
            "Tutorial", "Level_1", "Level_2", "Level_3", "BossFight"
        };
        
        m_currentLevelIndex++;
        
        if (m_currentLevelIndex < 5) {
            // Load next level
            LoadLevel(levels[m_currentLevelIndex]);
        } else {
            // Game complete
            ReturnToMenu();
            ShowVictoryScreen();
        }
    }
    
    void RestartLevel() {
        const char* levels[] = {
            "Tutorial", "Level_1", "Level_2", "Level_3", "BossFight"
        };
        
        // Reload current level
        LoadLevel(levels[m_currentLevelIndex]);
    }
    
private:
    void LoadLevel(const char* levelName) {
        // Unload previous level
        if (m_currentLevel[0] != '\0') {
            m_levelManager->UnloadLevel(m_currentLevel);
        }
        
        // Load new level
        m_levelManager->LoadLevel(levelName, LoadPriority::Immediate);
        
        // Update state
        strncpy(m_currentLevel, levelName, 64);
    }
    
    LevelManager* m_levelManager;
    char m_currentLevel[64] = "";
    int m_currentLevelIndex = 0;
};
```

## Advanced: Open World with Multiple Zones

### Zone-Based Levels
```json
{
  "levels": [
    {
      "name": "Zone_Forest",
      "path": "Assets/zone_forest.json",
      "streamingMethod": "distance",
      "streamingDistance": 2000.0,
      "bounds": {
        "center": [0, 0, 0],
        "extents": [1000, 200, 1000]
      }
    },
    {
      "name": "Zone_Desert",
      "path": "Assets/zone_desert.json",
      "streamingMethod": "distance",
      "streamingDistance": 2000.0,
      "bounds": {
        "center": [2000, 0, 0],
        "extents": [1000, 200, 1000]
      }
    },
    {
      "name": "Zone_Mountains",
      "path": "Assets/zone_mountains.json",
      "streamingMethod": "distance",
      "streamingDistance": 2000.0,
      "bounds": {
        "center": [0, 0, 2000],
        "extents": [1000, 200, 1000]
      }
    }
  ]
}
```

**Automatic streaming** as player moves between zones!

## Best Practices

### 1. Persistent Menu
Keep menu as persistent level:
```json
{
  "name": "MainMenu",
  "type": "persistent",
  "autoLoad": true
}
```

### 2. Streaming Game Levels
Load game levels on demand:
```json
{
  "name": "Level_1",
  "type": "streaming",
  "autoLoad": false
}
```

### 3. Clean Transitions
Always unload before loading:
```cpp
levelManager->UnloadLevel("OldLevel");
levelManager->LoadLevel("NewLevel");
```

### 4. Memory Management
Unload unused levels:
```cpp
// After completing level
levelManager->UnloadLevel("CompletedLevel");
```

### 5. Loading Screens
Show loading screen during transitions:
```cpp
ShowLoadingScreen();
levelManager->LoadLevel("NextLevel", LoadPriority::High);
// Loading happens async
// Hide loading screen when complete
```

## Summary

✅ **Multiple Levels** - Each with its own scene.json
✅ **Easy Switching** - Load/unload levels dynamically
✅ **Clean Hierarchy** - Each level is self-contained
✅ **Memory Efficient** - Only loaded levels in memory
✅ **Flexible** - Add new levels without code changes
✅ **Streaming** - Automatic distance-based loading

**Create as many levels as you want, each with its own scene.json!**
