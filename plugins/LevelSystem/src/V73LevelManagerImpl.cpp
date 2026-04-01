// SecretEngine - v7.3 Ultra Full Level Manager Main Implementation

#include "V73LevelManager.h"
#include <SecretEngine/Components.h>
#include <algorithm>
#include <chrono>
#include <fstream>

namespace SecretEngine::Levels::V73 {

// ============================================================================
// Main V73LevelManager Implementation
// ============================================================================
V73LevelManager::V73LevelManager(ICore* core)
    : m_core(core)
    , m_world(core->GetWorld())
    , m_logger(core->GetLogger())
    , m_assetProvider(core->GetAssetProvider())
{
    m_logger->LogInfo("V73LevelManager", "Initializing v7.3 Ultra Full Level System");
    
    // Initialize subsystems
    m_streamingManager = std::make_unique<StreamingManager>(core);
    m_instanceManager = std::make_unique<InstanceManager>(core);
    m_audioManager = std::make_unique<AudioManager>(core);
    m_weatherSystem = std::make_unique<WeatherSystem>(core);
    m_playerManager = std::make_unique<PlayerManager>(core);
    m_triggerSystem = std::make_unique<TriggerSystem>(this);
    
    // Register trigger action handlers
    m_triggerSystem->RegisterActionHandler("load_level", 
        [this](const Trigger::Action& action, uint32_t entityId) {
            if (!action.target_level.empty()) {
                std::string levelPath = "levels/" + action.target_level + ".json";
                m_logger->LogInfo("TriggerSystem", ("Loading level: " + levelPath).c_str());
                LoadLevel(levelPath);
            }
        });
    
    m_logger->LogInfo("V73LevelManager", "v7.3 Level System initialized successfully");
}

V73LevelManager::~V73LevelManager() {
    if (m_shouldStopStreaming) {
        m_shouldStopStreaming = true;
        if (m_streamingThread.joinable()) {
            m_streamingThread.join();
        }
    }
    
    UnloadLevel();
    m_logger->LogInfo("V73LevelManager", "v7.3 Level System shutdown complete");
}

bool V73LevelManager::LoadLevel(const std::string& levelPath) {
    std::lock_guard<std::mutex> lock(m_levelMutex);
    
    if (m_levelState == LevelState::Loading) {
        m_logger->LogWarning("V73LevelManager", "Level is already loading");
        return false;
    }
    
    if (m_currentLevel) {
        UnloadLevel();
    }
    
    m_levelState = LevelState::Loading;
    
    char msg[512];
    snprintf(msg, sizeof(msg), "Loading v7.3 level: %s", levelPath.c_str());
    m_logger->LogInfo("V73LevelManager", msg);
    
    // Load level JSON
    if (!m_assetProvider) {
        m_logger->LogError("V73LevelManager", "AssetProvider not available");
        m_levelState = LevelState::Error;
        return false;
    }
    
    std::string jsonText = m_assetProvider->LoadText(levelPath.c_str());
    if (jsonText.empty()) {
        snprintf(msg, sizeof(msg), "Failed to load level file: %s", levelPath.c_str());
        m_logger->LogError("V73LevelManager", msg);
        m_levelState = LevelState::Error;
        return false;
    }
    
    bool success = LoadLevelFromJSON(jsonText);
    if (success) {
        InitializeLevel();
        m_levelState = LevelState::Loaded;
        
        // Fire level loaded event
        LevelEvent event;
        event.type = LevelEvent::LevelLoaded;
        event.level_id = m_currentLevel->level_id;
        FireEvent(event);
        
        snprintf(msg, sizeof(msg), "Successfully loaded level: %s", m_currentLevel->name.c_str());
        m_logger->LogInfo("V73LevelManager", msg);
    } else {
        m_levelState = LevelState::Error;
        m_logger->LogError("V73LevelManager", "Failed to parse level JSON");
    }
    
    return success;
}

bool V73LevelManager::LoadLevelFromJSON(const std::string& jsonText) {
    try {
        nlohmann::json json = nlohmann::json::parse(jsonText);
        
        m_currentLevel = std::make_unique<Level>();
        *m_currentLevel = ParseLevelJSON(json);
        
        return true;
    } catch (const std::exception& e) {
        char msg[512];
        snprintf(msg, sizeof(msg), "JSON parse error: %s", e.what());
        m_logger->LogError("V73LevelManager", msg);
        return false;
    }
}

Level V73LevelManager::ParseLevelJSON(const nlohmann::json& json) {
    Level level;
    
    // Basic level info
    level.level_id = json.value("level_id", "unknown");
    level.version = json.value("version", "2.0");
    level.name = json.value("name", "Unnamed Level");
    level.description = json.value("description", "");
    level.difficulty_rating = json.value("difficulty_rating", 3);
    
    if (json.contains("recommended_players") && json["recommended_players"].is_array()) {
        auto players = json["recommended_players"];
        for (size_t i = 0; i < std::min(players.size(), size_t(3)); ++i) {
            level.recommended_players[i] = players[i].get<int32_t>();
        }
    }
    
    // Environment
    if (json.contains("environment")) {
        const auto& env = json["environment"];
        level.environment.terrain_type = env.value("terrain_type", "ruins");
        level.environment.heightmap = env.value("heightmap", "");
        
        if (env.contains("terrain_scale") && env["terrain_scale"].is_array()) {
            auto scale = env["terrain_scale"];
            level.environment.terrain_scale = glm::vec3(
                scale[0].get<float>(),
                scale[1].get<float>(),
                scale[2].get<float>()
            );
        }
        
        if (env.contains("ground_materials") && env["ground_materials"].is_array()) {
            for (const auto& material : env["ground_materials"]) {
                level.environment.ground_materials.push_back(material.get<std::string>());
            }
        }
        
        // Skybox
        if (env.contains("skybox")) {
            const auto& skybox = env["skybox"];
            level.environment.skybox.cubemap = skybox.value("cubemap", "");
            level.environment.skybox.clouds = skybox.value("clouds", "");
            level.environment.skybox.stars_visible = skybox.value("stars_visible", false);
        }
        
        // Ambient lighting
        if (env.contains("ambient_lighting")) {
            const auto& ambient = env["ambient_lighting"];
            if (ambient.contains("color") && ambient["color"].is_array()) {
                auto color = ambient["color"];
                level.environment.ambient_lighting.color = glm::vec3(
                    color[0].get<float>(),
                    color[1].get<float>(),
                    color[2].get<float>()
                );
            }
            level.environment.ambient_lighting.intensity = ambient.value("intensity", 0.5f);
            level.environment.ambient_lighting.occlusion_strength = ambient.value("occlusion_strength", 0.8f);
        }
        
        // Fog
        if (env.contains("fog")) {
            const auto& fog = env["fog"];
            level.environment.fog.enabled = fog.value("enabled", true);
            if (fog.contains("color") && fog["color"].is_array()) {
                auto color = fog["color"];
                level.environment.fog.color = glm::vec3(
                    color[0].get<float>(),
                    color[1].get<float>(),
                    color[2].get<float>()
                );
            }
            level.environment.fog.start_distance = fog.value("start_distance", 50.0f);
            level.environment.fog.end_distance = fog.value("end_distance", 300.0f);
            level.environment.fog.density = fog.value("density", 0.02f);
        }
    }
    
    // Time of day
    if (json.contains("time_of_day")) {
        const auto& tod = json["time_of_day"];
        level.time_of_day.cycle_enabled = tod.value("cycle_enabled", true);
        level.time_of_day.duration_seconds = tod.value("duration_seconds", 1800);
        level.time_of_day.current_time = tod.value("current_time", 43200);
        
        if (tod.contains("sun")) {
            const auto& sun = tod["sun"];
            if (sun.contains("direction") && sun["direction"].is_array()) {
                auto dir = sun["direction"];
                level.time_of_day.sun.direction = glm::vec3(
                    dir[0].get<float>(),
                    dir[1].get<float>(),
                    dir[2].get<float>()
                );
            }
            if (sun.contains("color") && sun["color"].is_array()) {
                auto color = sun["color"];
                level.time_of_day.sun.color = glm::vec3(
                    color[0].get<float>(),
                    color[1].get<float>(),
                    color[2].get<float>()
                );
            }
            level.time_of_day.sun.intensity = sun.value("intensity", 1.0f);
            level.time_of_day.sun.shadow_cascades = sun.value("shadow_cascades", 4);
            level.time_of_day.sun.shadow_distance = sun.value("shadow_distance", 200.0f);
        }
    }
    
    // Weather system
    if (json.contains("weather_system")) {
        const auto& weather = json["weather_system"];
        level.weather_system.current_weather = weather.value("current_weather", "clear");
        level.weather_system.transition_time = weather.value("transition_time", 30.0f);
        
        if (weather.contains("patterns") && weather["patterns"].is_array()) {
            for (const auto& pattern : weather["patterns"]) {
                WeatherPattern wp;
                wp.name = pattern.value("name", "unknown");
                wp.duration_min = pattern.value("duration_min", 180);
                wp.duration_max = pattern.value("duration_max", 300);
                wp.visibility = pattern.value("visibility", 1.0f);
                wp.wind_speed = pattern.value("wind_speed", 2.0f);
                wp.precipitation = pattern.value("precipitation", 0.0f);
                wp.lightning_frequency = pattern.value("lightning_frequency", 0.0f);
                wp.cloud_coverage = pattern.value("cloud_coverage", 0.1f);
                wp.particle_effect = pattern.value("particle_effect", "");
                wp.sound_effect = pattern.value("sound_effect", "");
                wp.damage_per_second = pattern.value("damage_per_second", 0.0f);
                
                level.weather_system.patterns.push_back(wp);
            }
        }
    }
    
    // Boundaries
    if (json.contains("boundaries")) {
        const auto& bounds = json["boundaries"];
        
        if (bounds.contains("playable_min") && bounds["playable_min"].is_array()) {
            auto min_bounds = bounds["playable_min"];
            level.boundaries.playable_min = glm::vec3(
                min_bounds[0].get<float>(),
                min_bounds[1].get<float>(),
                min_bounds[2].get<float>()
            );
        }
        
        if (bounds.contains("playable_max") && bounds["playable_max"].is_array()) {
            auto max_bounds = bounds["playable_max"];
            level.boundaries.playable_max = glm::vec3(
                max_bounds[0].get<float>(),
                max_bounds[1].get<float>(),
                max_bounds[2].get<float>()
            );
        }
        
        level.boundaries.out_of_bounds_damage = bounds.value("out_of_bounds_damage", 10.0f);
        level.boundaries.return_timeout = bounds.value("return_timeout", 5.0f);
    }
    
    // Parse chunks
    if (json.contains("chunks") && json["chunks"].is_array()) {
        for (const auto& chunkJson : json["chunks"]) {
            Chunk chunk = ParseChunkJSON(chunkJson);
            level.chunks.push_back(chunk);
        }
    }
    
    // Parse players
    if (json.contains("players") && json["players"].is_array()) {
        for (const auto& playerJson : json["players"]) {
            Player player = ParsePlayerJSON(playerJson);
            level.players.push_back(player);
        }
    }
    
    // Store additional game data
    if (json.contains("gameplay_data")) {
        level.gameplay_data = json["gameplay_data"];
    }
    
    if (json.contains("progression_data")) {
        level.progression_data = json["progression_data"];
    }
    
    if (json.contains("monetization_data")) {
        level.monetization_data = json["monetization_data"];
    }
    
    return level;
}

Chunk V73LevelManager::ParseChunkJSON(const nlohmann::json& json) {
    Chunk chunk;
    
    chunk.chunk_id = json.value("chunk_id", "unknown");
    
    // Bounds
    if (json.contains("bounds")) {
        const auto& bounds = json["bounds"];
        
        if (bounds.contains("min") && bounds["min"].is_array()) {
            auto min_bounds = bounds["min"];
            chunk.bounds_min = glm::vec3(
                min_bounds[0].get<float>(),
                min_bounds[1].get<float>(),
                min_bounds[2].get<float>()
            );
        }
        
        if (bounds.contains("max") && bounds["max"].is_array()) {
            auto max_bounds = bounds["max"];
            chunk.bounds_max = glm::vec3(
                max_bounds[0].get<float>(),
                max_bounds[1].get<float>(),
                max_bounds[2].get<float>()
            );
        }
        
        if (bounds.contains("center") && bounds["center"].is_array()) {
            auto center = bounds["center"];
            chunk.bounds_center = glm::vec3(
                center[0].get<float>(),
                center[1].get<float>(),
                center[2].get<float>()
            );
        }
    }
    
    // Mesh groups
    if (json.contains("mesh_groups") && json["mesh_groups"].is_array()) {
        for (const auto& meshGroup : json["mesh_groups"]) {
            std::string mesh = meshGroup.value("mesh", "");
            std::string material = meshGroup.value("material", "");
            
            if (meshGroup.contains("instances") && meshGroup["instances"].is_array()) {
                for (const auto& instance : meshGroup["instances"]) {
                    MeshInstance meshInstance;
                    meshInstance.id = static_cast<uint32_t>(chunk.mesh_instances.size());
                    
                    // Parse transform
                    if (instance.contains("transform")) {
                        meshInstance.transform = ParseTransformJSON(instance["transform"]);
                    }
                    
                    // Parse tags
                    if (instance.contains("tags")) {
                        const auto& tags = instance["tags"];
                        meshInstance.tags.type = tags.value("type", "mesh");
                        meshInstance.tags.destructible = tags.value("destructible", false);
                        meshInstance.tags.health = tags.value("health", 100);
                        meshInstance.tags.lootable = tags.value("lootable", false);
                    }
                    
                    // Parse material params
                    if (instance.contains("material_params")) {
                        const auto& params = instance["material_params"];
                        if (params.contains("color") && params["color"].is_array()) {
                            auto color = params["color"];
                            meshInstance.color = glm::vec4(
                                color[0].get<float>(),
                                color[1].get<float>(),
                                color[2].get<float>(),
                                1.0f
                            );
                        }
                    }
                    
                    // Parse culling
                    if (instance.contains("culling")) {
                        const auto& culling = instance["culling"];
                        meshInstance.culling.radius = culling.value("radius", 1.0f);
                    }
                    
                    // Parse LOD
                    if (instance.contains("lod") && instance["lod"].contains("levels")) {
                        const auto& levels = instance["lod"]["levels"];
                        for (const auto& level : levels) {
                            MeshInstance::LOD::Level lodLevel;
                            lodLevel.distance = level.value("distance", 25.0f);
                            lodLevel.mesh_path = level.value("mesh", mesh);
                            meshInstance.lod.levels.push_back(lodLevel);
                        }
                    }
                    
                    chunk.mesh_instances.push_back(meshInstance);
                }
            }
        }
    }
    
    // Parse triggers
    if (json.contains("triggers") && json["triggers"].is_array()) {
        for (const auto& triggerJson : json["triggers"]) {
            Trigger trigger;
            trigger.id = triggerJson.value("id", "");
            trigger.type = triggerJson.value("type", "area");
            trigger.shape = triggerJson.value("shape", "sphere");
            trigger.radius = triggerJson.value("radius", 20.0f);
            trigger.repeatable = triggerJson.value("repeatable", false);
            trigger.cooldown = triggerJson.value("cooldown", 30.0f);
            
            if (triggerJson.contains("position") && triggerJson["position"].is_array()) {
                auto pos = triggerJson["position"];
                trigger.position = glm::vec3(
                    pos[0].get<float>(),
                    pos[1].get<float>(),
                    pos[2].get<float>()
                );
            }
            
            // Parse enter actions
            if (triggerJson.contains("enter_actions") && triggerJson["enter_actions"].is_array()) {
                for (const auto& actionJson : triggerJson["enter_actions"]) {
                    Trigger::Action action;
                    action.type = actionJson.value("type", "");
                    action.sound = actionJson.value("sound", "");
                    action.count = actionJson.value("count", 0);
                    action.delay = actionJson.value("delay", 0.0f);
                    action.target_level = actionJson.value("target_level", "");
                    
                    if (actionJson.contains("target_position") && actionJson["target_position"].is_array()) {
                        auto pos = actionJson["target_position"];
                        action.target_position = glm::vec3(
                            pos[0].get<float>(),
                            pos[1].get<float>(),
                            pos[2].get<float>()
                        );
                    }
                    
                    trigger.enter_actions.push_back(action);
                }
            }
            
            // Parse exit actions
            if (triggerJson.contains("exit_actions") && triggerJson["exit_actions"].is_array()) {
                for (const auto& actionJson : triggerJson["exit_actions"]) {
                    Trigger::Action action;
                    action.type = actionJson.value("type", "");
                    action.sound = actionJson.value("sound", "");
                    action.count = actionJson.value("count", 0);
                    action.delay = actionJson.value("delay", 0.0f);
                    
                    if (actionJson.contains("target_position") && actionJson["target_position"].is_array()) {
                        auto pos = actionJson["target_position"];
                        action.target_position = glm::vec3(
                            pos[0].get<float>(),
                            pos[1].get<float>(),
                            pos[2].get<float>()
                        );
                    }
                    
                    trigger.exit_actions.push_back(action);
                }
            }
            
            chunk.triggers.push_back(trigger);
        }
    }
    
    return chunk;
}

Player V73LevelManager::ParsePlayerJSON(const nlohmann::json& json) {
    Player player;
    
    player.id = json.value("id", 0);
    player.username = json.value("username", "Player");
    player.level = json.value("level", 1);
    player.xp = json.value("xp", 0);
    player.prestige = json.value("prestige", 0);
    player.region = json.value("region", "US");
    player.language = json.value("language", "en");
    
    // Parse transform
    if (json.contains("transform")) {
        player.transform = ParseTransformJSON(json["transform"]);
    }
    
    // Parse stats
    if (json.contains("stats")) {
        const auto& stats = json["stats"];
        
        if (stats.contains("health")) {
            const auto& health = stats["health"];
            player.stats.health.current = health.value("current", 100);
            player.stats.health.max = health.value("max", 100);
            player.stats.health.regen_rate = health.value("regen_rate", 2.0f);
            player.stats.health.regen_delay = health.value("regen_delay", 5.0f);
        }
        
        if (stats.contains("stamina")) {
            const auto& stamina = stats["stamina"];
            player.stats.stamina.current = stamina.value("current", 100);
            player.stats.stamina.max = stamina.value("max", 100);
            player.stats.stamina.regen_rate = stamina.value("regen_rate", 20.0f);
            player.stats.stamina.regen_delay = stamina.value("regen_delay", 1.5f);
        }
    }
    
    return player;
}

Transform V73LevelManager::ParseTransformJSON(const nlohmann::json& json) {
    Transform transform;
    
    // Position
    if (json.contains("position") && json["position"].is_array()) {
        auto pos = json["position"];
        transform.position = glm::vec3(
            pos[0].get<float>(),
            pos[1].get<float>(),
            pos[2].get<float>()
        );
    }
    
    // Rotation (YXZ order, degrees)
    if (json.contains("rotation") && json["rotation"].is_array()) {
        auto rot = json["rotation"];
        glm::vec3 eulerDegrees(
            rot[0].get<float>(),
            rot[1].get<float>(),
            rot[2].get<float>()
        );
        transform = Transform::FromEulerYXZ(transform.position, eulerDegrees, transform.scale);
    }
    
    // Scale
    if (json.contains("scale") && json["scale"].is_array()) {
        auto scale = json["scale"];
        transform.scale = glm::vec3(
            scale[0].get<float>(),
            scale[1].get<float>(),
            scale[2].get<float>()
        );
    }
    
    return transform;
}

void V73LevelManager::InitializeLevel() {
    if (!m_currentLevel) return;
    
    m_logger->LogInfo("V73LevelManager", "Initializing level systems...");
    
    // Initialize weather system
    if (!m_currentLevel->weather_system.patterns.empty()) {
        m_weatherSystem->LoadWeatherPatterns(m_currentLevel->weather_system.patterns);
        m_weatherSystem->SetWeather(m_currentLevel->weather_system.current_weather);
    }
    
    // Initialize audio zones
    std::vector<AudioZone> audioZones;
    for (const auto& chunk : m_currentLevel->chunks) {
        for (const auto& zone : chunk.audio_zones) {
            audioZones.push_back(zone);
        }
    }
    if (!audioZones.empty()) {
        m_audioManager->LoadAudioZones(audioZones);
    }
    
    // Initialize spawn points
    std::vector<SpawnPoint> allSpawnPoints;
    for (const auto& chunk : m_currentLevel->chunks) {
        for (const auto& spawn : chunk.spawn_points) {
            allSpawnPoints.push_back(spawn);
        }
    }
    if (!allSpawnPoints.empty()) {
        m_playerManager->LoadSpawnPoints(allSpawnPoints);
    }
    
    // Initialize triggers
    std::vector<Trigger> allTriggers;
    for (const auto& chunk : m_currentLevel->chunks) {
        for (const auto& trigger : chunk.triggers) {
            allTriggers.push_back(trigger);
        }
    }
    if (!allTriggers.empty() && m_triggerSystem) {
        m_triggerSystem->InitializeTriggers(allTriggers);
        m_logger->LogInfo("V73LevelManager", ("Initialized " + std::to_string(allTriggers.size()) + " triggers").c_str());
    }
    
    // Load initial chunks
    for (const auto& chunk : m_currentLevel->chunks) {
        LoadChunk(chunk.chunk_id);
    }
    
    // Start streaming thread
    m_shouldStopStreaming = false;
    m_streamingThread = std::thread(&V73LevelManager::StreamingThreadFunction, this);
    
    m_logger->LogInfo("V73LevelManager", "Level initialization complete");
}

bool V73LevelManager::UnloadLevel() {
    std::lock_guard<std::mutex> lock(m_levelMutex);
    
    if (!m_currentLevel) return true;
    
    m_levelState = LevelState::Unloading;
    
    m_logger->LogInfo("V73LevelManager", "Unloading current level...");
    
    // Stop streaming thread
    if (m_streamingThread.joinable()) {
        m_shouldStopStreaming = true;
        m_streamingThread.join();
    }
    
    CleanupLevel();
    
    // Fire level unloaded event
    LevelEvent event;
    event.type = LevelEvent::LevelUnloaded;
    event.level_id = m_currentLevel->level_id;
    FireEvent(event);
    
    m_currentLevel.reset();
    m_levelState = LevelState::Unloaded;
    m_chunkStates.clear();
    
    m_logger->LogInfo("V73LevelManager", "Level unloaded successfully");
    return true;
}

void V73LevelManager::CleanupLevel() {
    // Cleanup all subsystems
    if (m_instanceManager) {
        // TODO: Cleanup all instances
    }
    
    if (m_audioManager) {
        m_audioManager->StopAmbientTrack();
        m_audioManager->StopCombatMusic();
    }
    
    // Reset metrics
    m_metrics.Reset();
}

void V73LevelManager::Update(float deltaTime) {
    if (m_levelState != LevelState::Loaded && m_levelState != LevelState::Active) {
        return;
    }
    
    // Update subsystems
    if (m_streamingManager) {
        m_streamingManager->UpdateStreaming(deltaTime);
    }
    
    if (m_weatherSystem) {
        m_weatherSystem->UpdateWeather(deltaTime);
    }
    
    // Update trigger system
    if (m_triggerSystem) {
        m_triggerSystem->Update(deltaTime);
        
        // Check all players against triggers
        if (m_playerManager) {
            auto playerIds = m_playerManager->GetAllPlayerIds();
            for (uint32_t playerId : playerIds) {
                const Player* player = m_playerManager->GetPlayer(playerId);
                if (player) {
                    m_triggerSystem->CheckEntity(playerId, player->transform.position);
                }
            }
        }
    }
    
    // Update time of day
    UpdateTimeOfDay(deltaTime);
    
    // Update dynamic events
    UpdateDynamicEvents(deltaTime);
    
    // Update performance metrics
    UpdatePerformanceMetrics();
}

void V73LevelManager::UpdateTimeOfDay(float deltaTime) {
    if (!m_currentLevel || !m_currentLevel->time_of_day.cycle_enabled) return;
    
    // Simple time progression
    m_currentLevel->time_of_day.current_time += static_cast<int32_t>(deltaTime);
    if (m_currentLevel->time_of_day.current_time >= m_currentLevel->time_of_day.duration_seconds) {
        m_currentLevel->time_of_day.current_time = 0;
    }
}

void V73LevelManager::UpdateDynamicEvents(float deltaTime) {
    // TODO: Implement dynamic event system
}

void V73LevelManager::UpdatePerformanceMetrics() {
    if (m_instanceManager) {
        m_metrics.visible_instances = m_instanceManager->GetVisibleCount();
    }
    
    // TODO: Update other metrics
}

void V73LevelManager::StreamingThreadFunction() {
    while (!m_shouldStopStreaming) {
        UpdateChunkStreaming(0.016f); // ~60 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void V73LevelManager::FireEvent(const LevelEvent& event) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    
    for (const auto& callback : m_eventCallbacks) {
        if (callback) {
            callback(event);
        }
    }
}

// Additional methods for chunk management, player management, etc.
bool V73LevelManager::LoadChunk(const std::string& chunkId) {
    auto it = m_chunkStates.find(chunkId);
    if (it != m_chunkStates.end() && it->second != ChunkState::Unloaded) {
        return true; // Already loaded or loading
    }
    
    m_chunkStates[chunkId] = ChunkState::Loading;
    
    // TODO: Implement actual chunk loading
    m_chunkStates[chunkId] = ChunkState::Loaded;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Loaded chunk: %s", chunkId.c_str());
    m_logger->LogInfo("V73LevelManager", msg);
    
    return true;
}

uint32_t V73LevelManager::SpawnPlayer(const Player& playerData, const glm::vec3& position) {
    if (!m_playerManager) return 0;
    
    return m_playerManager->SpawnPlayer(playerData, position);
}

void V73LevelManager::RegisterEventCallback(LevelEventCallback callback) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    m_eventCallbacks.push_back(callback);
}

void V73LevelManager::PrintLevelInfo() const {
    if (!m_currentLevel) {
        m_logger->LogInfo("V73LevelManager", "No level currently loaded");
        return;
    }
    
    char msg[512];
    snprintf(msg, sizeof(msg), 
             "=== Level Info ===\n"
             "Name: %s\n"
             "ID: %s\n"
             "Version: %s\n"
             "Chunks: %zu\n"
             "Players: %zu\n"
             "State: %d",
             m_currentLevel->name.c_str(),
             m_currentLevel->level_id.c_str(),
             m_currentLevel->version.c_str(),
             m_currentLevel->chunks.size(),
             m_currentLevel->players.size(),
             static_cast<int>(m_levelState));
    
    m_logger->LogInfo("V73LevelManager", msg);
}

} // namespace SecretEngine::Levels::V73