// SecretEngine
// Module: LevelManager
// Responsibility: Level management implementation
// Dependencies: LevelManager.h

#include "LevelManager.h"
#include <SecretEngine/Components.h>
#include <SecretEngine/IAssetProvider.h>
#include <SecretEngine/IRenderer.h>

namespace SecretEngine::Levels {

LevelManager::LevelManager(ICore* core)
    : m_core(core)
    , m_world(core->GetWorld())
    , m_logger(core->GetLogger())
    , m_levelCount(0)
    , m_persistentLevelIndex(-1)
    , m_streamingEnabled(true)
    , m_isTransitioning(false)
    , m_transitionProgress(0.0f)
    , m_volumeCount(0)
{
    m_levelLoader = new LevelLoader(core);
    m_logger->LogInfo("LevelManager", "initialized");
}

LevelManager::~LevelManager() {
    UnloadAllLevels();
    if (m_levelLoader) {
        delete m_levelLoader;
        m_levelLoader = nullptr;
    }
}

bool LevelManager::LoadLevelDefinitions(const char* path) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Attempting to load level definitions from: %s", path);
    m_logger->LogInfo("LevelManager", msg);
    
    // Use AssetProvider for cross-platform asset loading (works on Android)
    auto assetProvider = m_core->GetAssetProvider();
    if (!assetProvider) {
        m_logger->LogError("LevelManager", "AssetProvider not available");
        return false;
    }
    
    std::string jsonText = assetProvider->LoadText(path);
    if (jsonText.empty()) {
        snprintf(msg, sizeof(msg), "Failed to load level definitions: %s (file not found or empty)", path);
        m_logger->LogError("LevelManager", msg);
        return false;
    }
    
    m_logger->LogInfo("LevelManager", "File loaded successfully, parsing JSON...");
    
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(jsonText);
    } catch (const std::exception& e) {
        snprintf(msg, sizeof(msg), "JSON parse error: %s", e.what());
        m_logger->LogError("LevelManager", msg);
        return false;
    }
    
    if (!json.contains("levels")) {
        m_logger->LogError("LevelManager", "No 'levels' array in level definitions");
        return false;
    }
    
    m_logger->LogInfo("LevelManager", "Successfully parsed JSON, processing levels...");
    
    for (const auto& levelJson : json["levels"]) {
        if (m_levelCount >= MAX_LEVELS) {
            m_logger->LogWarning("LevelManager", "Max levels reached, skipping remaining");
            break;
        }
        
        LevelDefinition def = ParseLevelDefinition(levelJson);
        
        Level& level = m_levels[m_levelCount++];
        level.definition = def;
        level.state = LevelState::Unloaded;
        level.visibility = LevelVisibility::Hidden;
        
        snprintf(msg, sizeof(msg), "Registered level: %s (type: %d, path: %s)", 
                 def.name, static_cast<int>(def.type), def.path);
        m_logger->LogInfo("LevelManager", msg);
        
        // Auto-load if specified
        if (def.autoLoad) {
            LoadLevel(def.name, def.loadPriority);
        }
    }
    
    snprintf(msg, sizeof(msg), "Loaded %d level definitions from %s", m_levelCount, path);
    m_logger->LogInfo("LevelManager", msg);
    
    return true;
}

bool LevelManager::LoadLevel(const char* levelName, LoadPriority priority) {
    int index = GetLevelIndex(levelName);
    if (index < 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Level not found: %s", levelName);
        m_logger->LogError("LevelManager", msg);
        return false;
    }
    
    Level* level = &m_levels[index];
    
    if (level->state == LevelState::Loaded || level->state == LevelState::Visible) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Level already loaded: %s", levelName);
        m_logger->LogWarning("LevelManager", msg);
        return true;
    }
    
    // Clear all renderer instances before loading new level
    if (m_core) {
        auto* renderer = reinterpret_cast<IRenderer*>(m_core->GetCapability("rendering"));
        if (renderer) {
            m_logger->LogInfo("LevelManager", "Clearing all renderer instances before loading new level...");
            renderer->ClearAllInstances();
        }
    }
    
    // Unload current level before loading new one (except persistent levels)
    if (level->definition.type != LevelType::Persistent) {
        UnloadCurrentLevel();
    }
    
    char msg[128];
    snprintf(msg, sizeof(msg), "Loading level: %s (priority: %d)", 
             levelName, static_cast<int>(priority));
    m_logger->LogInfo("LevelManager", msg);
    
    level->state = LevelState::Loading;
    level->loadProgress = 0.0f;
    
    // Load level data (simplified - in production this would be async)
    LoadLevelData(level);
    
    level->state = LevelState::Loaded;
    level->loadProgress = 1.0f;
    
    if (level->definition.makeVisibleAfterLoad) {
        ShowLevel(levelName);
    }
    
    // Load sub-levels
    if (level->definition.subLevelCount > 0) {
        LoadSubLevels(levelName);
    }
    
    snprintf(msg, sizeof(msg), "Level loaded: %s", levelName);
    m_logger->LogInfo("LevelManager", msg);
    
    return true;
}

bool LevelManager::UnloadLevel(const char* levelName) {
    int index = GetLevelIndex(levelName);
    if (index < 0) return false;
    
    Level* level = &m_levels[index];
    
    if (level->state == LevelState::Unloaded) {
        return true;
    }
    
    // Don't unload persistent level
    if (index == m_persistentLevelIndex) {
        m_logger->LogWarning("LevelManager", "Cannot unload persistent level");
        return false;
    }
    
    char msg[128];
    snprintf(msg, sizeof(msg), "Unloading level: %s", levelName);
    m_logger->LogInfo("LevelManager", msg);
    
    // Clear all renderer instances when unloading
    if (m_core) {
        auto* renderer = reinterpret_cast<IRenderer*>(m_core->GetCapability("rendering"));
        if (renderer) {
            m_logger->LogInfo("LevelManager", "Clearing all renderer instances...");
            renderer->ClearAllInstances();
        }
    }
    
    level->state = LevelState::Unloading;
    
    // Unload sub-levels first
    UnloadSubLevels(levelName);
    
    // Hide level
    HideLevel(levelName);
    
    // Unload level data
    UnloadLevelData(level);
    
    level->state = LevelState::Unloaded;
    level->entityCount = 0;
    
    snprintf(msg, sizeof(msg), "Level unloaded: %s", levelName);
    m_logger->LogInfo("LevelManager", msg);
    
    return true;
}

void LevelManager::ShowLevel(const char* levelName) {
    Level* level = GetLevel(levelName);
    if (!level) return;
    
    if (level->state != LevelState::Loaded && level->state != LevelState::Visible) {
        m_logger->LogWarning("LevelManager", "Cannot show unloaded level");
        return;
    }
    
    level->visibility = LevelVisibility::Visible;
    level->state = LevelState::Visible;
    
    // Enable rendering for all entities in level
    // (In production, this would set visibility flags on entities)
    
    char msg[128];
    snprintf(msg, sizeof(msg), "Level visible: %s", levelName);
    m_logger->LogInfo("LevelManager", msg);
}

void LevelManager::HideLevel(const char* levelName) {
    Level* level = GetLevel(levelName);
    if (!level) return;
    
    level->visibility = LevelVisibility::Hidden;
    if (level->state == LevelState::Visible) {
        level->state = LevelState::Loaded;
    }
    
    // Disable rendering for all entities in level
    
    char msg[128];
    snprintf(msg, sizeof(msg), "Level hidden: %s", levelName);
    m_logger->LogInfo("LevelManager", msg);
}

void LevelManager::UpdateStreaming(float dt, const float playerPosition[3]) {
    if (!m_streamingEnabled) return;
    
    // Update transition
    if (m_isTransitioning) {
        m_transitionProgress += dt / m_currentTransition.transitionDuration;
        if (m_transitionProgress >= 1.0f) {
            m_isTransitioning = false;
            m_transitionProgress = 0.0f;
        }
        return;
    }
    
    // Check distance-based streaming
    CheckDistanceStreaming(playerPosition);
    
    // Check volume-based streaming
    CheckVolumeStreaming(playerPosition);
    
    // Update level states
    for (int i = 0; i < m_levelCount; i++) {
        UpdateLevelState(&m_levels[i], dt);
    }
}

void LevelManager::CheckDistanceStreaming(const float playerPosition[3]) {
    for (int i = 0; i < m_levelCount; i++) {
        Level* level = &m_levels[i];
        
        if (level->definition.streamingMethod != StreamingMethod::Distance) {
            continue;
        }
        
        float distance = level->definition.bounds.DistanceToPoint(playerPosition);
        
        // Load if within streaming distance
        if (distance <= level->definition.streamingDistance) {
            if (level->state == LevelState::Unloaded) {
                LoadLevel(level->definition.name, level->definition.loadPriority);
            }
        }
        // Unload if too far
        else if (distance > level->definition.streamingDistance * 1.5f) {
            if (level->state == LevelState::Loaded || level->state == LevelState::Visible) {
                UnloadLevel(level->definition.name);
            }
        }
    }
}

void LevelManager::CheckVolumeStreaming(const float playerPosition[3]) {
    for (int i = 0; i < m_volumeCount; i++) {
        LevelStreamingVolume& volume = m_streamingVolumes[i];
        
        bool isInside = volume.bounds.ContainsPoint(playerPosition);
        
        // Player entered volume
        if (isInside && !volume.playerInside) {
            volume.playerInside = true;
            if (volume.loadOnEnter) {
                LoadLevel(volume.levelName, 
                         volume.blockOnLoad ? LoadPriority::Immediate : LoadPriority::High);
            }
        }
        // Player exited volume
        else if (!isInside && volume.playerInside) {
            volume.playerInside = false;
            if (volume.unloadOnExit) {
                UnloadLevel(volume.levelName);
            }
        }
    }
}

Level* LevelManager::GetLevel(const char* levelName) {
    int index = GetLevelIndex(levelName);
    return (index >= 0) ? &m_levels[index] : nullptr;
}

int LevelManager::GetLevelIndex(const char* levelName) const {
    for (int i = 0; i < m_levelCount; i++) {
        if (strcmp(m_levels[i].definition.name, levelName) == 0) {
            return i;
        }
    }
    return -1;
}

bool LevelManager::IsLevelLoaded(const char* levelName) const {
    int index = GetLevelIndex(levelName);
    if (index < 0) return false;
    
    LevelState state = m_levels[index].state;
    return state == LevelState::Loaded || state == LevelState::Visible;
}

void LevelManager::SetPersistentLevel(const char* levelName) {
    int index = GetLevelIndex(levelName);
    if (index < 0) return;
    
    m_persistentLevelIndex = index;
    m_levels[index].definition.type = LevelType::Persistent;
    
    // Ensure persistent level is loaded
    if (!IsLevelLoaded(levelName)) {
        LoadLevel(levelName, LoadPriority::Immediate);
    }
    
    char msg[128];
    snprintf(msg, sizeof(msg), "Set persistent level: %s", levelName);
    m_logger->LogInfo("LevelManager", msg);
}

void LevelManager::UnloadCurrentLevel() {
    m_logger->LogInfo("LevelManager", "Unloading current level (keeping persistent)");
    
    // Unload all non-persistent levels
    for (int i = 0; i < m_levelCount; i++) {
        if (i != m_persistentLevelIndex && 
            (m_levels[i].state == LevelState::Loaded || m_levels[i].state == LevelState::Visible)) {
            UnloadLevel(m_levels[i].definition.name);
        }
    }
}

void LevelManager::UnloadAllLevels() {
    for (int i = 0; i < m_levelCount; i++) {
        if (i != m_persistentLevelIndex) {
            UnloadLevel(m_levels[i].definition.name);
        }
    }
}

void LevelManager::LoadLevelData(Level* level) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Loading level data from: %s", level->definition.path);
    m_logger->LogInfo("LevelManager", msg);
    
    // Load level file using LevelLoader
    if (m_levelLoader) {
        bool loaded = m_levelLoader->LoadLevelFromFile(level->definition.path, level);
        if (loaded) {
            snprintf(msg, sizeof(msg), "Level data loaded: %s (%d entities)", 
                     level->definition.name, level->entityCount);
            m_logger->LogInfo("LevelManager", msg);
        } else {
            snprintf(msg, sizeof(msg), "Failed to load level data: %s from path: %s", 
                     level->definition.name, level->definition.path);
            m_logger->LogError("LevelManager", msg);
        }
    }
}

void LevelManager::UnloadLevelData(Level* level) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Unloading level data: %s (%d entities)", 
             level->definition.name, level->entityCount);
    m_logger->LogInfo("LevelManager", msg);
    
    // Destroy all entities in level
    for (int i = 0; i < level->entityCount; i++) {
        Entity entity;
        entity.id = level->entities[i];
        m_world->DestroyEntity(entity);
    }
    level->entityCount = 0;
    
    m_logger->LogInfo("LevelManager", "Level data unloaded successfully");
}

void LevelManager::UpdateLevelState(Level* level, float dt) {
    if (level->state == LevelState::Loaded || level->state == LevelState::Visible) {
        level->timeLoaded += dt;
    }
}

LevelDefinition LevelManager::ParseLevelDefinition(const nlohmann::json& json) {
    LevelDefinition def;
    
    std::string name = json.value("name", "");
    strncpy(def.name, name.c_str(), LevelDefinition::MAX_NAME_LENGTH - 1);
    
    std::string path = json.value("path", "");
    strncpy(def.path, path.c_str(), LevelDefinition::MAX_PATH_LENGTH - 1);
    
    std::string typeStr = json.value("type", "streaming");
    if (typeStr == "persistent") def.type = LevelType::Persistent;
    else if (typeStr == "sublevel") def.type = LevelType::SubLevel;
    else def.type = LevelType::Streaming;
    
    std::string methodStr = json.value("streamingMethod", "manual");
    if (methodStr == "distance") def.streamingMethod = StreamingMethod::Distance;
    else if (methodStr == "volume") def.streamingMethod = StreamingMethod::Volume;
    else if (methodStr == "always") def.streamingMethod = StreamingMethod::Always;
    else def.streamingMethod = StreamingMethod::Manual;
    
    def.streamingDistance = json.value("streamingDistance", 1000.0f);
    def.autoLoad = json.value("autoLoad", false);
    def.blockOnLoad = json.value("blockOnLoad", false);
    def.makeVisibleAfterLoad = json.value("makeVisibleAfterLoad", true);
    
    // Parse bounds
    if (json.contains("bounds")) {
        auto bounds = json["bounds"];
        if (bounds.contains("center")) {
            def.bounds.center[0] = bounds["center"][0];
            def.bounds.center[1] = bounds["center"][1];
            def.bounds.center[2] = bounds["center"][2];
        }
        if (bounds.contains("extents")) {
            def.bounds.extents[0] = bounds["extents"][0];
            def.bounds.extents[1] = bounds["extents"][1];
            def.bounds.extents[2] = bounds["extents"][2];
        }
    }
    
    // Parse sub-levels
    if (json.contains("subLevels")) {
        for (const auto& subLevel : json["subLevels"]) {
            std::string subName = subLevel.get<std::string>();
            def.AddSubLevel(subName.c_str());
        }
    }
    
    return def;
}

void LevelManager::PrintLevelInfo() const {
    m_logger->LogInfo("LevelManager", "=== Level System Info ===");
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Total levels: %d", m_levelCount);
    m_logger->LogInfo("LevelManager", msg);
    
    snprintf(msg, sizeof(msg), "Streaming enabled: %s", m_streamingEnabled ? "yes" : "no");
    m_logger->LogInfo("LevelManager", msg);
    
    for (int i = 0; i < m_levelCount; i++) {
        const Level& level = m_levels[i];
        snprintf(msg, sizeof(msg), "  [%d] %s - State: %d, Entities: %d", 
                 i, level.definition.name, static_cast<int>(level.state), level.entityCount);
        m_logger->LogInfo("LevelManager", msg);
    }
}

bool LevelManager::LoadSubLevels(const char* parentLevelName) {
    Level* parent = GetLevel(parentLevelName);
    if (!parent) return false;
    
    for (int i = 0; i < parent->definition.subLevelCount; i++) {
        const char* subLevelName = parent->definition.subLevels[i];
        LoadLevel(subLevelName, parent->definition.loadPriority);
        
        // Link parent-child relationship
        int subIndex = GetLevelIndex(subLevelName);
        if (subIndex >= 0) {
            m_levels[subIndex].parentLevelIndex = GetLevelIndex(parentLevelName);
            parent->childLevelIndices[parent->childLevelCount++] = subIndex;
        }
    }
    
    return true;
}

bool LevelManager::UnloadSubLevels(const char* parentLevelName) {
    Level* parent = GetLevel(parentLevelName);
    if (!parent) return false;
    
    for (int i = 0; i < parent->childLevelCount; i++) {
        int childIndex = parent->childLevelIndices[i];
        if (childIndex >= 0 && childIndex < m_levelCount) {
            UnloadLevel(m_levels[childIndex].definition.name);
        }
    }
    
    parent->childLevelCount = 0;
    return true;
}

void LevelManager::RegisterStreamingVolume(const LevelStreamingVolume& volume) {
    if (m_volumeCount < 32) {
        m_streamingVolumes[m_volumeCount++] = volume;
        
        char msg[128];
        snprintf(msg, sizeof(msg), "Registered streaming volume for level: %s", volume.levelName);
        m_logger->LogInfo("LevelManager", msg);
    }
}

} // namespace SecretEngine::Levels
