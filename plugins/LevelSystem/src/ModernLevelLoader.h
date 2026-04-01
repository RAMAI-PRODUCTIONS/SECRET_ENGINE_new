// SecretEngine
// Module: ModernLevelLoader
// Responsibility: Load-Once-Spawn-Dereference level loading
// Architecture: Phase 1 (Load JSON) → Phase 2 (Spawn) → Phase 3 (Dereference)

#pragma once
#include "ModernLevelTypes.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/ILogger.h>
#include <SecretEngine/IAssetProvider.h>
#include <nlohmann/json.hpp>
#include <memory>

namespace SecretEngine::Levels {

class ModernLevelLoader {
public:
    ModernLevelLoader(ICore* core);
    ~ModernLevelLoader();
    
    // ========================================================================
    // PHASE 1: Load JSON Reference Data (Once at Startup)
    // ========================================================================
    std::unique_ptr<LevelReferenceData> LoadLevelReference(const char* levelPath);
    
    // ========================================================================
    // PHASE 2: Spawn Entities from Reference Data
    // ========================================================================
    RuntimeLevel* SpawnLevelFromReference(const LevelReferenceData& refData);
    
    // ========================================================================
    // PHASE 3: Dereference (Free JSON Data)
    // ========================================================================
    void DereferenceLoadedData();
    
    // ========================================================================
    // Chunk Operations (Runtime)
    // ========================================================================
    bool LoadChunk(RuntimeLevel* level, uint32_t chunkIndex);
    bool UnloadChunk(RuntimeLevel* level, uint32_t chunkIndex);
    
    // ========================================================================
    // Memory Stats
    // ========================================================================
    size_t GetTempMemoryUsage() const;
    size_t GetMemoryFreedByDereference() const { return m_memoryFreed; }
    
private:
    ICore* m_core;
    IWorld* m_world;
    ILogger* m_logger;
    IAssetProvider* m_assetProvider;
    
    // Temporary storage during load (freed after spawn)
    std::vector<std::unique_ptr<ChunkData>> m_tempChunkData;
    size_t m_memoryFreed = 0;
    
    // ========================================================================
    // JSON Parsing Helpers
    // ========================================================================
    LevelReferenceData ParseLevelJSON(const nlohmann::json& json);
    ChunkData ParseChunkJSON(const nlohmann::json& json);
    InstanceData ParseInstanceJSON(const nlohmann::json& json, 
                                   const std::string& defaultMesh,
                                   const std::string& defaultMaterial);
    TransformSettings ParseTransformSettings(const nlohmann::json& json);
    LODConfig ParseLODConfig(const nlohmann::json& json);
    CullingConfig ParseCullingConfig(const nlohmann::json& json);
    
    // ========================================================================
    // Spawning Helpers
    // ========================================================================
    uint32_t SpawnMeshInstance(const InstanceData& instance, 
                               const TransformSettings& settings);
    uint32_t SpawnEntity(const nlohmann::json& entityData,
                        const TransformSettings& settings);
    
    // ========================================================================
    // Conversion Helpers
    // ========================================================================
    void ConvertRotation(float rotation[3], const TransformSettings& settings);
    float DegreesToRadians(float degrees) const;
    void ApplyRotationOrder(float rotation[3], TransformSettings::RotationOrder order);
    
    // ========================================================================
    // Validation
    // ========================================================================
    bool ValidateSchemaVersion(const SchemaVersion& version);
    bool ValidateChunkData(const ChunkData& chunk);
};

} // namespace SecretEngine::Levels
