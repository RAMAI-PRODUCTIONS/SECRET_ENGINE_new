// SecretEngine
// Module: ModernLevelLoader
// Implementation: Load-Once-Spawn-Dereference

#include "ModernLevelLoader.h"
#include <SecretEngine/Components.h>
#include <cmath>
#include <algorithm>

namespace SecretEngine::Levels {

ModernLevelLoader::ModernLevelLoader(ICore* core)
    : m_core(core)
    , m_world(core->GetWorld())
    , m_logger(core->GetLogger())
    , m_assetProvider(core->GetAssetProvider())
    , m_memoryFreed(0)
{
    m_logger->LogInfo("ModernLevelLoader", "Initialized (Load-Once-Spawn-Dereference)");
}

ModernLevelLoader::~ModernLevelLoader() {
    DereferenceLoadedData();
}

// ============================================================================
// PHASE 1: Load JSON Reference Data
// ============================================================================

std::unique_ptr<LevelReferenceData> ModernLevelLoader::LoadLevelReference(
    const char* levelPath) {
    
    m_logger->LogInfo("ModernLevelLoader", "=== PHASE 1: Loading JSON Reference ===");
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Loading level from: %s", levelPath);
    m_logger->LogInfo("ModernLevelLoader", msg);
    
    // Load main level JSON
    if (!m_assetProvider) {
        m_logger->LogError("ModernLevelLoader", "AssetProvider not available");
        return nullptr;
    }
    
    std::string jsonText = m_assetProvider->LoadText(levelPath);
    if (jsonText.empty()) {
        m_logger->LogError("ModernLevelLoader", "Failed to load level JSON");
        return nullptr;
    }
    
    nlohmann::json levelJson;
    try {
        levelJson = nlohmann::json::parse(jsonText);
    } catch (const std::exception& e) {
        snprintf(msg, sizeof(msg), "JSON parse error: %s", e.what());
        m_logger->LogError("ModernLevelLoader", msg);
        return nullptr;
    }
    
    // Parse reference data
    auto refData = std::make_unique<LevelReferenceData>();
    *refData = ParseLevelJSON(levelJson);
    
    // Validate schema version
    if (!ValidateSchemaVersion(refData->schemaVersion)) {
        m_logger->LogError("ModernLevelLoader", "Incompatible schema version");
        return nullptr;
    }
    
    // Load chunk JSONs into temporary storage
    m_logger->LogInfo("ModernLevelLoader", "Loading chunk data...");
    
    for (const auto& chunkPath : refData->chunkPaths) {
        std::string chunkText = m_assetProvider->LoadText(chunkPath.c_str());
        if (chunkText.empty()) {
            snprintf(msg, sizeof(msg), "Failed to load chunk: %s", chunkPath.c_str());
            m_logger->LogWarning("ModernLevelLoader", msg);
            continue;
        }
        
        nlohmann::json chunkJson;
        try {
            chunkJson = nlohmann::json::parse(chunkText);
        } catch (const std::exception& e) {
            snprintf(msg, sizeof(msg), "Chunk parse error: %s", e.what());
            m_logger->LogWarning("ModernLevelLoader", msg);
            continue;
        }
        
        auto chunkData = std::make_unique<ChunkData>();
        *chunkData = ParseChunkJSON(chunkJson);
        
        if (ValidateChunkData(*chunkData)) {
            m_tempChunkData.push_back(std::move(chunkData));
        }
    }
    
    snprintf(msg, sizeof(msg), 
             "Loaded %zu chunks, temp memory: ~%zu KB",
             m_tempChunkData.size(), GetTempMemoryUsage() / 1024);
    m_logger->LogInfo("ModernLevelLoader", msg);
    
    m_logger->LogInfo("ModernLevelLoader", "=== PHASE 1 COMPLETE ===");
    return refData;
}

// ============================================================================
// PHASE 2: Spawn Entities from Reference
// ============================================================================

RuntimeLevel* ModernLevelLoader::SpawnLevelFromReference(
    const LevelReferenceData& refData) {
    
    m_logger->LogInfo("ModernLevelLoader", "=== PHASE 2: Spawning Entities ===");
    
    auto runtimeLevel = new RuntimeLevel();
    runtimeLevel->name = refData.levelName;
    runtimeLevel->schemaVersion = refData.schemaVersion;
    runtimeLevel->transformSettings = refData.transformSettings;
    runtimeLevel->chunkSize = refData.chunkSize;
    runtimeLevel->loadRadius = refData.loadRadius;
    runtimeLevel->unloadRadius = refData.unloadRadius;
    
    char msg[256];
    
    // Spawn entities from each chunk
    for (const auto& chunkData : m_tempChunkData) {
        RuntimeChunk runtimeChunk;
        runtimeChunk.chunkId = std::hash<std::string>{}(chunkData->chunkId);
        memcpy(runtimeChunk.boundsCenter, chunkData->boundsCenter, sizeof(float) * 3);
        memcpy(runtimeChunk.boundsExtents, chunkData->boundsExtents, sizeof(float) * 3);
        
        // Spawn mesh instances
        for (const auto& meshGroup : chunkData->meshGroups) {
            for (const auto& instance : meshGroup.instances) {
                uint32_t entityId = SpawnMeshInstance(instance, refData.transformSettings);
                if (entityId != 0) {
                    runtimeChunk.entityIds.push_back(entityId);
                    runtimeLevel->totalEntities++;
                }
            }
        }
        
        // Spawn entities
        for (const auto& entityJson : chunkData->entities) {
            uint32_t entityId = SpawnEntity(entityJson, refData.transformSettings);
            if (entityId != 0) {
                runtimeChunk.entityIds.push_back(entityId);
                runtimeLevel->totalEntities++;
            }
        }
        
        runtimeChunk.isLoaded = true;
        runtimeLevel->chunks.push_back(runtimeChunk);
        runtimeLevel->loadedChunks++;
    }
    
    snprintf(msg, sizeof(msg), 
             "Spawned %u entities across %zu chunks",
             runtimeLevel->totalEntities, runtimeLevel->chunks.size());
    m_logger->LogInfo("ModernLevelLoader", msg);
    
    m_logger->LogInfo("ModernLevelLoader", "=== PHASE 2 COMPLETE ===");
    return runtimeLevel;
}

// ============================================================================
// PHASE 3: Dereference JSON Data
// ============================================================================

void ModernLevelLoader::DereferenceLoadedData() {
    m_logger->LogInfo("ModernLevelLoader", "=== PHASE 3: Dereferencing JSON ===");
    
    // Calculate memory to be freed
    m_memoryFreed = GetTempMemoryUsage();
    
    // Clear all temporary chunk data
    m_tempChunkData.clear();
    m_tempChunkData.shrink_to_fit();
    
    char msg[128];
    snprintf(msg, sizeof(msg), "Dereferenced JSON data, freed ~%zu KB", 
             m_memoryFreed / 1024);
    m_logger->LogInfo("ModernLevelLoader", msg);
    
    m_logger->LogInfo("ModernLevelLoader", "=== PHASE 3 COMPLETE ===");
}

// ============================================================================
// JSON Parsing
// ============================================================================

LevelReferenceData ModernLevelLoader::ParseLevelJSON(const nlohmann::json& json) {
    LevelReferenceData refData;
    
    // Schema version
    if (json.contains("schema_version")) {
        std::string versionStr = json["schema_version"].get<std::string>();
        size_t dotPos = versionStr.find('.');
        if (dotPos != std::string::npos) {
            refData.schemaVersion.major = std::stoi(versionStr.substr(0, dotPos));
            refData.schemaVersion.minor = std::stoi(versionStr.substr(dotPos + 1));
        }
    }
    
    // Basic info
    refData.levelName = json.value("level_name", "unnamed");
    
    // Metadata
    if (json.contains("metadata")) {
        auto meta = json["metadata"];
        refData.author = meta.value("author", "");
        refData.created = meta.value("created", "");
        refData.engineVersion = meta.value("engine_version", "");
    }
    
    // Core settings
    if (json.contains("core_settings")) {
        auto core = json["core_settings"];
        if (core.contains("transform")) {
            refData.transformSettings = ParseTransformSettings(core["transform"]);
        }
        if (core.contains("rendering")) {
            auto rendering = core["rendering"];
            if (rendering.contains("default_lod_distances")) {
                refData.defaultLODDistances = 
                    rendering["default_lod_distances"].get<std::vector<float>>();
            }
            refData.cullingEnabled = rendering.value("culling_enabled", true);
        }
    }
    
    // References
    if (json.contains("references")) {
        auto refs = json["references"];
        if (refs.contains("entities")) {
            refData.entityPaths = refs["entities"].get<std::vector<std::string>>();
        }
        if (refs.contains("chunks")) {
            refData.chunkPaths = refs["chunks"].get<std::vector<std::string>>();
        }
        if (refs.contains("navigation")) {
            refData.navigationPath = refs["navigation"].get<std::string>();
        }
    }
    
    // Streaming
    if (json.contains("streaming")) {
        auto streaming = json["streaming"];
        refData.chunkSize = streaming.value("chunk_size", 100.0f);
        refData.loadRadius = streaming.value("load_radius", 2);
        refData.unloadRadius = streaming.value("unload_radius", 3);
        refData.streamingMethod = streaming.value("method", "distance");
    }
    
    return refData;
}

ChunkData ModernLevelLoader::ParseChunkJSON(const nlohmann::json& json) {
    ChunkData chunk;
    
    chunk.chunkId = json.value("chunk_id", "0");
    
    // Bounds
    if (json.contains("bounds")) {
        auto bounds = json["bounds"];
        if (bounds.contains("center")) {
            auto center = bounds["center"];
            chunk.boundsCenter[0] = center[0].get<float>();
            chunk.boundsCenter[1] = center[1].get<float>();
            chunk.boundsCenter[2] = center[2].get<float>();
        }
        if (bounds.contains("extents")) {
            auto extents = bounds["extents"];
            chunk.boundsExtents[0] = extents[0].get<float>();
            chunk.boundsExtents[1] = extents[1].get<float>();
            chunk.boundsExtents[2] = extents[2].get<float>();
        }
    }
    
    // Mesh groups
    if (json.contains("mesh_groups")) {
        for (const auto& groupJson : json["mesh_groups"]) {
            MeshGroup group;
            group.meshPath = groupJson.value("mesh", "");
            group.materialPath = groupJson.value("material", "");
            
            if (groupJson.contains("instances")) {
                for (const auto& instJson : groupJson["instances"]) {
                    InstanceData instance = ParseInstanceJSON(instJson, 
                                                             group.meshPath,
                                                             group.materialPath);
                    group.instances.push_back(instance);
                }
            }
            
            chunk.meshGroups.push_back(group);
        }
    }
    
    // Entities (store raw JSON for later parsing)
    if (json.contains("entities")) {
        for (const auto& entityJson : json["entities"]) {
            chunk.entities.push_back(entityJson);
        }
    }
    
    return chunk;
}

InstanceData ModernLevelLoader::ParseInstanceJSON(
    const nlohmann::json& json,
    const std::string& defaultMesh,
    const std::string& defaultMaterial) {
    
    InstanceData instance;
    instance.meshPath = defaultMesh;
    instance.materialPath = defaultMaterial;
    
    // Transform
    if (json.contains("transform")) {
        auto transform = json["transform"];
        if (transform.contains("position")) {
            auto pos = transform["position"];
            instance.position[0] = pos[0].get<float>();
            instance.position[1] = pos[1].get<float>();
            instance.position[2] = pos[2].get<float>();
        }
        if (transform.contains("rotation")) {
            auto rot = transform["rotation"];
            instance.rotation[0] = rot[0].get<float>();
            instance.rotation[1] = rot[1].get<float>();
            instance.rotation[2] = rot[2].get<float>();
        }
        if (transform.contains("scale")) {
            auto scale = transform["scale"];
            instance.scale[0] = scale[0].get<float>();
            instance.scale[1] = scale[1].get<float>();
            instance.scale[2] = scale[2].get<float>();
        }
    }
    
    // Material params
    if (json.contains("material_params")) {
        auto params = json["material_params"];
        if (params.contains("color")) {
            auto color = params["color"];
            instance.materialColor[0] = color[0].get<float>();
            instance.materialColor[1] = color[1].get<float>();
            instance.materialColor[2] = color[2].get<float>();
        }
    }
    
    // Tags
    if (json.contains("tags")) {
        instance.tags = json["tags"];
    }
    
    // LOD
    if (json.contains("lod")) {
        instance.lod = ParseLODConfig(json["lod"]);
    }
    
    // Culling
    if (json.contains("culling")) {
        instance.culling = ParseCullingConfig(json["culling"]);
    }
    
    return instance;
}

TransformSettings ModernLevelLoader::ParseTransformSettings(
    const nlohmann::json& json) {
    
    TransformSettings settings;
    
    std::string format = json.value("rotation_format", "euler");
    settings.rotationFormat = TransformSettings::ParseRotationFormat(format);
    
    std::string unit = json.value("rotation_unit", "degrees");
    settings.rotationUnit = TransformSettings::ParseRotationUnit(unit);
    
    std::string order = json.value("rotation_order", "YXZ");
    settings.rotationOrder = TransformSettings::ParseRotationOrder(order);
    
    return settings;
}

LODConfig ModernLevelLoader::ParseLODConfig(const nlohmann::json& json) {
    LODConfig lod;
    
    lod.enabled = json.value("enabled", true);
    
    if (json.contains("levels")) {
        for (const auto& levelJson : json["levels"]) {
            LODLevel level;
            level.distance = levelJson.value("distance", 0.0f);
            level.meshPath = levelJson.value("mesh", "");
            lod.levels.push_back(level);
        }
    }
    
    return lod;
}

CullingConfig ModernLevelLoader::ParseCullingConfig(const nlohmann::json& json) {
    CullingConfig culling;
    
    culling.enabled = json.value("enabled", true);
    culling.radius = json.value("radius", 1.0f);
    
    std::string method = json.value("method", "sphere");
    culling.method = CullingConfig::ParseMethod(method);
    
    if (json.contains("bounds")) {
        auto bounds = json["bounds"];
        for (int i = 0; i < 6 && i < bounds.size(); ++i) {
            culling.bounds[i] = bounds[i].get<float>();
        }
    }
    
    return culling;
}

// ============================================================================
// PHASE 2: Spawn Entities
// ============================================================================

uint32_t ModernLevelLoader::SpawnMeshInstance(
    const InstanceData& instance,
    const TransformSettings& settings) {
    
    // Create entity
    Entity entity = m_world->CreateEntity();
    if (entity.id == 0) {
        m_logger->LogWarning("ModernLevelLoader", "Failed to create entity");
        return 0;
    }
    
    // Add Transform component
    TransformComponent transform;
    memcpy(transform.position, instance.position, sizeof(float) * 3);
    memcpy(transform.rotation, instance.rotation, sizeof(float) * 3);
    memcpy(transform.scale, instance.scale, sizeof(float) * 3);
    
    // Convert rotation based on settings
    ConvertRotation(transform.rotation, settings);
    
    m_world->AddComponent(entity, TransformComponent::TypeID, &transform);
    
    // Add Mesh component
    MeshComponent mesh;
    strncpy(mesh.meshPath, instance.meshPath.c_str(), 
            sizeof(mesh.meshPath) - 1);
    // Note: MeshComponent doesn't have materialPath, materials are handled separately
    
    // Material color override
    mesh.color[0] = instance.materialColor[0];
    mesh.color[1] = instance.materialColor[1];
    mesh.color[2] = instance.materialColor[2];
    mesh.color[3] = 1.0f;
    
    m_world->AddComponent(entity, MeshComponent::TypeID, &mesh);
    
    // Add LOD component if enabled
    if (instance.lod.enabled && !instance.lod.levels.empty()) {
        // TODO: Add LODComponent when available
        // LODComponent lodComp;
        // lodComp.levels = instance.lod.levels;
        // m_world->AddComponent(entity, lodComp);
    }
    
    // Add Culling component if enabled
    if (instance.culling.enabled) {
        // TODO: Add CullingComponent when available
        // CullingComponent cullComp;
        // cullComp.method = instance.culling.method;
        // cullComp.radius = instance.culling.radius;
        // m_world->AddComponent(entity, cullComp);
    }
    
    return entity.id;
}

uint32_t ModernLevelLoader::SpawnEntity(
    const nlohmann::json& entityData,
    const TransformSettings& settings) {
    
    // Create entity
    Entity entity = m_world->CreateEntity();
    if (entity.id == 0) {
        return 0;
    }
    
    // Parse and add transform
    if (entityData.contains("transform")) {
        TransformComponent transform;
        auto t = entityData["transform"];
        
        if (t.contains("position")) {
            auto pos = t["position"];
            transform.position[0] = pos[0].get<float>();
            transform.position[1] = pos[1].get<float>();
            transform.position[2] = pos[2].get<float>();
        }
        if (t.contains("rotation")) {
            auto rot = t["rotation"];
            transform.rotation[0] = rot[0].get<float>();
            transform.rotation[1] = rot[1].get<float>();
            transform.rotation[2] = rot[2].get<float>();
            ConvertRotation(transform.rotation, settings);
        }
        if (t.contains("scale")) {
            auto scale = t["scale"];
            transform.scale[0] = scale[0].get<float>();
            transform.scale[1] = scale[1].get<float>();
            transform.scale[2] = scale[2].get<float>();
        }
        
        m_world->AddComponent(entity, TransformComponent::TypeID, &transform);
    }
    
    // Parse components
    if (entityData.contains("components")) {
        auto components = entityData["components"];
        
        // Render component
        if (components.contains("render")) {
            auto render = components["render"];
            MeshComponent mesh;
            
            std::string meshPath = render.value("mesh", "");
            // Note: material path not used in MeshComponent
            
            strncpy(mesh.meshPath, meshPath.c_str(), sizeof(mesh.meshPath) - 1);
            
            m_world->AddComponent(entity, MeshComponent::TypeID, &mesh);
        }
        
        // Physics component
        if (components.contains("physics")) {
            // TODO: Add PhysicsComponent when available
        }
        
        // AI component
        if (components.contains("ai")) {
            // TODO: Add AIComponent when available
        }
    }
    
    return entity.id;
}

// ============================================================================
// Conversion Helpers
// ============================================================================

void ModernLevelLoader::ConvertRotation(
    float rotation[3],
    const TransformSettings& settings) {
    
    // Convert degrees to radians if needed
    if (settings.rotationUnit == TransformSettings::RotationUnit::Degrees) {
        rotation[0] = DegreesToRadians(rotation[0]);
        rotation[1] = DegreesToRadians(rotation[1]);
        rotation[2] = DegreesToRadians(rotation[2]);
    }
    
    // Apply rotation order if needed
    ApplyRotationOrder(rotation, settings.rotationOrder);
}

float ModernLevelLoader::DegreesToRadians(float degrees) const {
    return degrees * 3.14159265359f / 180.0f;
}

void ModernLevelLoader::ApplyRotationOrder(
    float rotation[3],
    TransformSettings::RotationOrder order) {
    
    // Reorder rotation components based on order
    // Default YXZ is already correct for most engines
    // Implement swizzling if needed for other orders
    
    switch (order) {
        case TransformSettings::RotationOrder::YXZ:
            // Already in correct order
            break;
        case TransformSettings::RotationOrder::XYZ:
            std::swap(rotation[0], rotation[1]);
            break;
        // Add other orders as needed
        default:
            break;
    }
}

// ============================================================================
// Validation
// ============================================================================

bool ModernLevelLoader::ValidateSchemaVersion(const SchemaVersion& version) {
    SchemaVersion current{1, 0};
    
    if (!current.IsCompatible(version)) {
        char msg[128];
        snprintf(msg, sizeof(msg), 
                 "Schema version mismatch: expected %s, got %s",
                 current.ToString().c_str(), version.ToString().c_str());
        m_logger->LogError("ModernLevelLoader", msg);
        return false;
    }
    
    return true;
}

bool ModernLevelLoader::ValidateChunkData(const ChunkData& chunk) {
    if (chunk.chunkId.empty()) {
        m_logger->LogWarning("ModernLevelLoader", "Chunk has no ID");
        return false;
    }
    
    if (chunk.meshGroups.empty() && chunk.entities.empty()) {
        m_logger->LogWarning("ModernLevelLoader", "Chunk is empty");
        return false;
    }
    
    return true;
}

// ============================================================================
// Memory Stats
// ============================================================================

size_t ModernLevelLoader::GetTempMemoryUsage() const {
    size_t total = 0;
    for (const auto& chunk : m_tempChunkData) {
        total += chunk->EstimateMemoryUsage();
    }
    return total;
}

} // namespace SecretEngine::Levels
