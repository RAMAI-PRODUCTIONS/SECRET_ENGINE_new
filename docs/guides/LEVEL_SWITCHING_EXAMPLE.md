# Level Switching - Code Examples

## Quick Start

### 1. Get Level Manager
```cpp
auto* levelPlugin = static_cast<Levels::LevelSystemPlugin*>(
    core->GetCapability("level_system")
);
auto* levelManager = levelPlugin->GetLevelManager();
```

### 2. Switch Levels
```cpp
// From MainMenu to FPS_Arena
levelManager->HideLevel("MainMenu");
levelManager->LoadLevel("FPS_Arena", LoadPriority::Immediate);

// From FPS_Arena to RacingTrack
levelManager->UnloadLevel("FPS_Arena");
levelManager->LoadLevel("RacingTrack", LoadPriority::Immediate);

// Back to MainMenu
levelManager->UnloadLevel("RacingTrack");
levelManager->ShowLevel("MainMenu");
```

## Complete Example: Game with Multiple Levels

```cpp
// GameManager.h
class GameManager {
public:
    void Initialize(ICore* core);
    void Update(float dt);
    
    // Level switching
    void StartFPSGame();
    void StartRacingGame();
    void ReturnToMenu();
    void SwitchToLevel(const char* levelName);
    
private:
    ICore* m_core;
    Levels::LevelManager* m_levelManager;
    char m_currentLevel[64];
};

// GameManager.cpp
void GameManager::Initialize(ICore* core) {
    m_core = core;
    
    auto* levelPlugin = static_cast<Levels::LevelSystemPlugin*>(
        core->GetCapability("level_system")
    );
    m_levelManager = levelPlugin->GetLevelManager();
    
    // Start at main menu
    strncpy(m_currentLevel, "MainMenu", 64);
}

void GameManager::StartFPSGame() {
    SwitchToLevel("FPS_Arena");
}

void GameManager::StartRacingGame() {
    SwitchToLevel("RacingTrack");
}

void GameManager::ReturnToMenu() {
    SwitchToLevel("MainMenu");
}

void GameManager::SwitchToLevel(const char* newLevel) {
    auto logger = m_core->GetLogger();
    
    char msg[128];
    snprintf(msg, sizeof(msg), "Switching from %s to %s", m_currentLevel, newLevel);
    logger->LogInfo("GameManager", msg);
    
    // Handle current level
    if (strcmp(m_currentLevel, "MainMenu") == 0) {
        // Menu is persistent, just hide it
        m_levelManager->HideLevel(m_currentLevel);
    } else {
        // Game levels are streaming, unload them
        m_levelManager->UnloadLevel(m_currentLevel);
    }
    
    // Load new level
    if (strcmp(newLevel, "MainMenu") == 0) {
        // Menu is already loaded, just show it
        m_levelManager->ShowLevel(newLevel);
    } else {
        // Load game level
        m_levelManager->LoadLevel(newLevel, LoadPriority::Immediate);
    }
    
    // Update current level
    strncpy(m_currentLevel, newLevel, 64);
    
    snprintf(msg, sizeof(msg), "Now in level: %s", m_currentLevel);
    logger->LogInfo("GameManager", msg);
}
```

## Integration with Input

```cpp
// In your input handler
void OnKeyPress(int key) {
    switch (key) {
        case KEY_1:
            gameManager->ReturnToMenu();
            break;
        case KEY_2:
            gameManager->StartFPSGame();
            break;
        case KEY_3:
            gameManager->StartRacingGame();
            break;
        case KEY_ESC:
            gameManager->ReturnToMenu();
            break;
    }
}
```

## Integration with FPS Game Plugin

```cpp
// In FPSGamePlugin.cpp
void FPSGamePlugin::OnActivate() {
    m_logger->LogInfo("FPSGameLogic", "activated");
    
    // Get level manager
    auto* levelPlugin = static_cast<Levels::LevelSystemPlugin*>(
        m_core->GetCapability("level_system")
    );
    
    if (levelPlugin) {
        auto* levelManager = levelPlugin->GetLevelManager();
        
        // Check if we're in FPS_Arena level
        if (levelManager->IsLevelLoaded("FPS_Arena")) {
            m_logger->LogInfo("FPSGameLogic", "FPS Arena level detected");
            
            // Create player and bots for FPS game
            CreatePlayerEntity();
            CreateBotEntities(5);
        }
        // Or check if we're in the original Scene level
        else if (levelManager->IsLevelLoaded("Scene")) {
            m_logger->LogInfo("FPSGameLogic", "Scene level detected");
            
            // Create player and bots for original scene
            CreatePlayerEntity();
            CreateBotEntities(5);
        }
    }
}
```

## Level Switching with Loading Screen

```cpp
class LevelSwitcher {
public:
    void SwitchWithLoadingScreen(const char* newLevel) {
        // Show loading screen
        ShowLoadingScreen();
        
        // Start async level load
        m_isLoading = true;
        m_targetLevel = newLevel;
        
        // Unload current level
        if (m_currentLevel[0] != '\0') {
            m_levelManager->UnloadLevel(m_currentLevel);
        }
        
        // Load new level (async)
        m_levelManager->LoadLevel(newLevel, LoadPriority::High);
    }
    
    void Update(float dt) {
        if (m_isLoading) {
            // Check if level is loaded
            if (m_levelManager->IsLevelLoaded(m_targetLevel)) {
                // Hide loading screen
                HideLoadingScreen();
                
                // Update current level
                strncpy(m_currentLevel, m_targetLevel, 64);
                m_isLoading = false;
            }
        }
    }
    
private:
    Levels::LevelManager* m_levelManager;
    char m_currentLevel[64] = "";
    const char* m_targetLevel = nullptr;
    bool m_isLoading = false;
};
```

## Level Switching with Fade Transition

```cpp
void SwitchLevelWithFade(const char* fromLevel, const char* toLevel) {
    Levels::LevelTransition transition;
    strncpy(transition.fromLevel, fromLevel, 64);
    strncpy(transition.toLevel, toLevel, 64);
    
    // Fade settings
    transition.transitionDuration = 2.0f;
    transition.fadeOutDuration = 0.5f;
    transition.fadeInDuration = 0.5f;
    
    // Keep persistent levels
    transition.keepPersistentLevel = true;
    
    // Don't keep player state (fresh start)
    transition.keepPlayerState = false;
    
    // Spawn position in new level
    transition.spawnPosition[0] = 0;
    transition.spawnPosition[1] = 1;
    transition.spawnPosition[2] = 10;
    
    m_levelManager->TransitionToLevel(toLevel, transition);
}
```

## Testing Level Switching

```cpp
void TestLevelSwitching() {
    auto logger = m_core->GetLogger();
    
    // Test 1: Menu to FPS
    logger->LogInfo("Test", "Switching to FPS Arena...");
    gameManager->StartFPSGame();
    
    // Wait 5 seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Test 2: FPS to Racing
    logger->LogInfo("Test", "Switching to Racing Track...");
    gameManager->StartRacingGame();
    
    // Wait 5 seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Test 3: Racing to Menu
    logger->LogInfo("Test", "Returning to Main Menu...");
    gameManager->ReturnToMenu();
}
```

## Summary

✅ **Simple API** - `LoadLevel()`, `UnloadLevel()`, `ShowLevel()`, `HideLevel()`
✅ **Persistent Levels** - Menu stays loaded, just hide/show
✅ **Streaming Levels** - Game levels load/unload dynamically
✅ **Easy Integration** - Works with existing game code
✅ **Flexible** - Add new levels without code changes

**Switch between levels with just a few lines of code!**
