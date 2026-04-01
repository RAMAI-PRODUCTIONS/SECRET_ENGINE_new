// SecretEngine - v7.3 Level System Tests
// Validation of performance improvements and v7.3 compatibility

#include <gtest/gtest.h>
#include "v73/V73CoreTypes.h"
#include "v73/GPUInstanceManager.h"
#include "v73/V73JsonParser.h"
#include <chrono>
#include <fstream>

using namespace SecretEngine::Levels::V73;

// ============================================================================
// Mock Asset Manager for Testing
// ============================================================================
class MockAssetManager : public AssetManager {
public:
    uint32_t GetMeshId(const std::string& path) override {
        static uint32_t nextId = 1;
        auto it = m_meshIds.find(path);
        if (it != m_meshIds.end()) {
            return it->second;
        }
        uint32_t id = nextId++;
        m_meshIds[path] = id;
        return id;
    }
    
    uint32_t GetMaterialId(const std::string& path) override {
        static uint32_t nextId = 1;
        auto it = m_materialIds.find(path);
        if (it != m_materialIds.end()) {
            return it->second;
        }
        uint32_t id = nextId++;
        m_materialIds[path] = id;
        return id;
    }
    
    bool LoadMesh(const std::string& path) override {
        m_loadedMeshes.insert(path);
        return true;
    }
    
    bool LoadMaterial(const std::string& path) override {
        m_loadedMaterials.insert(path);
        return true;
    }
    
private:
    std::unordered_map<std::string, uint32_t> m_meshIds;
    std::unordered_map<std::string, uint32_t> m_materialIds;
    std::unordered_set<std::string> m_loadedMeshes;
    std::unordered_set<std::string> m_loadedMaterials;
};

// ============================================================================
// Core Type Tests
// ============================================================================
TEST(V73CoreTypes, TransformCompactSize) {
    // Verify memory efficiency
    EXPECT_EQ(sizeof(TransformCompact), 48);
    EXPECT_EQ(sizeof(TransformGPU), 128);
    
    // Verify alignment
    EXPECT_EQ(alignof(TransformCompact), 16);
    EXPECT_EQ(alignof(TransformGPU), 128);
}

TEST(V73CoreTypes, TransformConversion) {
    TransformCompact transform;
    transform.position = glm::vec3(10, 20, 30);
    transform.rotation = glm::quat(1, 0, 0, 0);  // Identity
    transform.scale = glm::vec3(2, 2, 2);
    
    TransformGPU gpu = transform.ToGPU(1, 2, 0, glm::vec4(1, 0, 0, 1));
    
    EXPECT_EQ(gpu.meshId, 1);
    EXPECT_EQ(gpu.materialId, 2);
    EXPECT_EQ(gpu.lodLevel, 0);
    EXPECT_EQ(gpu.color, glm::vec4(1, 0, 0, 1));
    
    // Verify matrix is correct
    glm::vec4 point(0, 0, 0, 1);
    glm::vec4 transformed = gpu.modelMatrix * point;
    EXPECT_NEAR(transformed.x, 10.0f, 0.001f);
    EXPECT_NEAR(transformed.y, 20.0f, 0.001f);
    EXPECT_NEAR(transformed.z, 30.0f, 0.001f);
}

TEST(V73CoreTypes, PlayerSystem) {
    Player player;
    player.id = 42;
    player.stats.health = 100;
    player.stats.stamina = 80;
    
    // Test damage
    player.stats.ApplyDamage(25);
    EXPECT_EQ(player.stats.health, 75);
    
    // Test healing
    player.stats.Heal(10);
    EXPECT_EQ(player.stats.health, 85);
    
    // Test skill tree bonus
    player.stats.skillTree[1].level = 3;  // Speed skill
    float speedBonus = player.stats.skillTree[1].GetBonus();
    EXPECT_NEAR(speedBonus, 0.3f, 0.001f);
}

// ============================================================================
// GPU Instance Manager Tests
// ============================================================================
TEST(GPUInstanceManager, BasicOperations) {
    GPUInstanceManager manager(1000);
    
    TransformCompact transform;
    transform.position = glm::vec3(0, 0, 0);
    
    // Create instance
    uint32_t instanceId = manager.CreateInstance(1, 1, transform);
    EXPECT_NE(instanceId, 0);
    EXPECT_EQ(manager.GetInstanceCount(), 1);
    
    // Update instance
    transform.position = glm::vec3(10, 0, 0);
    manager.UpdateInstance(instanceId, transform);
    manager.FlushPendingUpdates();
    
    // Destroy instance
    manager.DestroyInstance(instanceId);
    manager.FlushPendingUpdates();
    EXPECT_EQ(manager.GetInstanceCount(), 0);
}

TEST(GPUInstanceManager, BatchOperations) {
    GPUInstanceManager manager(10000);
    
    // Create 1000 instances in batch
    std::vector<std::tuple<uint32_t, uint32_t, TransformCompact, glm::vec4>> instances;
    instances.reserve(1000);
    
    for (int i = 0; i < 1000; ++i) {
        TransformCompact transform;
        transform.position = glm::vec3(i * 2.0f, 0, 0);
        instances.emplace_back(1, 1, transform, glm::vec4(1, 1, 1, 1));
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    manager.CreateInstances(instances);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(manager.GetInstanceCount(), 1000);
    EXPECT_LT(duration.count(), 50);  // Should complete in < 50ms
}

TEST(GPUInstanceManager, LODSystem) {
    GPUInstanceManager manager(100);
    
    TransformCompact transform;
    transform.position = glm::vec3(0, 0, 0);
    
    uint32_t instanceId = manager.CreateInstance(1, 1, transform);
    
    // Test LOD updates at different distances
    glm::vec3 cameraPos(0, 0, 0);
    manager.UpdateLODs(cameraPos);
    
    cameraPos = glm::vec3(0, 0, 50);  // Distance 50 - should be LOD 1
    manager.UpdateLODs(cameraPos);
    
    cameraPos = glm::vec3(0, 0, 100); // Distance 100 - should be LOD 2
    manager.UpdateLODs(cameraPos);
    
    // Verify performance metrics
    auto metrics = manager.GetMetrics();
    EXPECT_GT(metrics.visibleInstances, 0);
}

TEST(GPUInstanceManager, CullingSystem) {
    GPUInstanceManager manager(100);
    
    // Create instances at various distances
    for (int i = 0; i < 10; ++i) {
        TransformCompact transform;
        transform.position = glm::vec3(i * 100.0f, 0, 0);  // 0, 100, 200, ... 900
        manager.CreateInstance(1, 1, transform);
    }
    
    // Test culling with camera at origin, view distance 500
    glm::vec3 cameraPos(0, 0, 0);
    glm::vec3 cameraForward(1, 0, 0);
    manager.UpdateCulling(cameraPos, cameraForward, 60.0f, 0.1f, 500.0f);
    
    auto metrics = manager.GetMetrics();
    EXPECT_LT(metrics.visibleInstances, 10);  // Some should be culled
}

// ============================================================================
// JSON Parser Tests
// ============================================================================
TEST(V73JsonParser, YXZRotationConversion) {
    std::array<float, 3> eulerDegrees = {30, 45, 60};  // X, Y, Z in degrees
    
    glm::quat result = V73JsonParser::ConvertYXZRotation(eulerDegrees);
    
    // Convert back to verify
    glm::vec3 eulerRadians = glm::eulerAngles(result);
    
    // Should be close to original (allowing for conversion precision)
    EXPECT_NEAR(glm::degrees(eulerRadians.x), 30.0f, 1.0f);
    EXPECT_NEAR(glm::degrees(eulerRadians.y), 45.0f, 1.0f);
    EXPECT_NEAR(glm::degrees(eulerRadians.z), 60.0f, 1.0f);
}

TEST(V73JsonParser, LevelManifestParsing) {
    std::string jsonText = R"({
        "version": "7.3",
        "name": "test_level",
        "core": {
            "transform_settings": {
                "rotation_format": "euler",
                "rotation_unit": "degrees",
                "rotation_order": "YXZ"
            }
        },
        "references": {
            "players": "entities/players.json",
            "chunks": ["chunks/chunk_0.json"]
        }
    })";
    
    V73JsonParser parser;
    auto result = parser.ParseLevelManifest(jsonText);
    
    ASSERT_TRUE(result.has_value());
    
    const auto& manifest = *result;
    EXPECT_EQ(manifest.version, "7.3");
    EXPECT_EQ(manifest.name, "test_level");
    EXPECT_EQ(manifest.core.transform_settings.rotation_order, "YXZ");
    EXPECT_EQ(manifest.references.players, "entities/players.json");
    EXPECT_EQ(manifest.references.chunks.size(), 1);
}

TEST(V73JsonParser, ChunkDataParsing) {
    std::string jsonText = R"({
        "chunk_id": "0",
        "mesh_groups": [
            {
                "mesh": "test.mesh",
                "material": "test.mat",
                "instances": [
                    {
                        "transform": {
                            "position": [10, 20, 30],
                            "rotation": [0, 90, 0],
                            "scale": [1, 1, 1]
                        },
                        "material_params": {
                            "color": [1.0, 0.5, 0.0]
                        },
                        "culling": {
                            "radius": 2.0
                        },
                        "lod": {
                            "levels": [
                                {"distance": 25},
                                {"distance": 75}
                            ]
                        }
                    }
                ]
            }
        ]
    })";
    
    MockAssetManager assetManager;
    V73JsonParser parser(&assetManager);
    
    auto result = parser.ParseChunkData(jsonText);
    ASSERT_TRUE(result.has_value());
    
    const auto& chunk = *result;
    EXPECT_EQ(chunk.chunk_id, "0");
    EXPECT_EQ(chunk.mesh_groups.size(), 1);
    
    const auto& group = chunk.mesh_groups[0];
    EXPECT_EQ(group.mesh, "test.mesh");
    EXPECT_EQ(group.material, "test.mat");
    EXPECT_EQ(group.instances.size(), 1);
    
    const auto& instance = group.instances[0];
    EXPECT_EQ(instance.transform.position[0], 10.0f);
    EXPECT_EQ(instance.transform.rotation[1], 90.0f);  // Y rotation
    EXPECT_EQ(instance.culling.radius, 2.0f);
    EXPECT_EQ(instance.lod.levels.size(), 2);
}

TEST(V73JsonParser, PlayerDataParsing) {
    std::string jsonText = R"({
        "players": [
            {
                "id": "player_0",
                "transform": {
                    "position": [54, 2, 130],
                    "rotation": [0, 29, 0],
                    "scale": [1, 1, 1]
                },
                "stats": {
                    "health": 100,
                    "stamina": 80
                },
                "loadout": {
                    "primary": "rifle",
                    "secondary": "pistol",
                    "tertiary": "knife"
                },
                "skill_tree": {
                    "nodes": [
                        {"id": "aim", "level": 2},
                        {"id": "speed", "level": 1},
                        {"id": "armor", "level": 3}
                    ]
                }
            }
        ]
    })";
    
    V73JsonParser parser;
    auto result = parser.ParsePlayerData(jsonText);
    
    ASSERT_TRUE(result.has_value());
    
    const auto& playerData = *result;
    EXPECT_EQ(playerData.players.size(), 1);
    
    const auto& player = playerData.players[0];
    EXPECT_EQ(player.id, 0);  // Extracted from "player_0"
    EXPECT_EQ(player.stats.health, 100);
    EXPECT_EQ(player.stats.stamina, 80);
    EXPECT_EQ(player.loadout.primary, "rifle");
    EXPECT_EQ(player.stats.skillTree[0].id, "aim");
    EXPECT_EQ(player.stats.skillTree[0].level, 2);
}

// ============================================================================
// Performance Tests (Android Focus)
// ============================================================================
TEST(V73Performance, MemoryEfficiency) {
    // Test memory usage of core types
    size_t playerSize = sizeof(Player);
    size_t transformSize = sizeof(TransformCompact);
    size_t gpuTransformSize = sizeof(TransformGPU);
    
    // Verify reasonable memory usage
    EXPECT_LT(playerSize, 200);  // Player should be < 200 bytes
    EXPECT_EQ(transformSize, 48);  // Compact transform should be 48 bytes
    EXPECT_EQ(gpuTransformSize, 128);  // GPU transform should be 128 bytes
    
    // Test 100 players memory usage
    size_t hundredPlayers = 100 * playerSize;
    EXPECT_LT(hundredPlayers, 20000);  // < 20KB for 100 players
}

TEST(V73Performance, BatchCreationSpeed) {
    GPUInstanceManager manager(10000);
    
    // Create 5000 instances (typical for a chunk)
    std::vector<std::tuple<uint32_t, uint32_t, TransformCompact, glm::vec4>> instances;
    instances.reserve(5000);
    
    for (int i = 0; i < 5000; ++i) {
        TransformCompact transform;
        transform.position = glm::vec3(
            (i % 100) * 2.0f,
            0,
            (i / 100) * 2.0f
        );
        instances.emplace_back(1, 1, transform, glm::vec4(1, 1, 1, 1));
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    manager.CreateInstances(instances);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(manager.GetInstanceCount(), 5000);
    EXPECT_LT(duration.count(), 100);  // Should complete in < 100ms (Android target)
}

TEST(V73Performance, CullingSpeed) {
    GPUInstanceManager manager(10000);
    
    // Create instances in a grid
    for (int x = 0; x < 100; ++x) {
        for (int z = 0; z < 100; ++z) {
            TransformCompact transform;
            transform.position = glm::vec3(x * 10.0f, 0, z * 10.0f);
            manager.CreateInstance(1, 1, transform);
        }
    }
    
    // Test culling performance
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int frame = 0; frame < 60; ++frame) {  // Simulate 60 frames
        glm::vec3 cameraPos(frame * 5.0f, 0, 0);
        glm::vec3 cameraForward(1, 0, 0);
        manager.UpdateCulling(cameraPos, cameraForward, 60.0f, 0.1f, 500.0f);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should handle 60 frames of culling in < 1000ms (16.67ms per frame target)
    EXPECT_LT(duration.count(), 1000);
    
    auto metrics = manager.GetMetrics();
    EXPECT_GT(metrics.visibleInstances, 0);
    EXPECT_LT(metrics.visibleInstances, 10000);  // Some culling should occur
}

// ============================================================================
// Integration Tests
// ============================================================================
TEST(V73Integration, FullWorkflow) {
    // Test the complete Load-Spawn-Dereference workflow
    MockAssetManager assetManager;
    V73JsonParser parser(&assetManager);
    GPUInstanceManager instanceManager(1000);
    
    // 1. Parse level manifest
    std::string manifestJson = R"({
        "version": "7.3",
        "name": "integration_test",
        "references": {
            "chunks": ["chunk_0.json"]
        }
    })";
    
    auto manifestResult = parser.ParseLevelManifest(manifestJson);
    ASSERT_TRUE(manifestResult.has_value());
    
    // 2. Parse chunk data
    std::string chunkJson = R"({
        "chunk_id": "0",
        "mesh_groups": [
            {
                "mesh": "test.mesh",
                "material": "test.mat",
                "instances": [
                    {
                        "transform": {
                            "position": [0, 0, 0],
                            "rotation": [0, 0, 0],
                            "scale": [1, 1, 1]
                        },
                        "material_params": {
                            "color": [1, 0, 0]
                        },
                        "culling": {"radius": 1.0},
                        "lod": {"levels": [{"distance": 25}]}
                    }
                ]
            }
        ]
    })";
    
    auto chunkResult = parser.ParseChunkData(chunkJson);
    ASSERT_TRUE(chunkResult.has_value());
    
    // 3. Convert to internal format
    auto instancesResult = parser.ConvertChunkInstances(*chunkResult);
    ASSERT_TRUE(instancesResult.has_value());
    
    // 4. Create instances in GPU manager
    const auto& instances = *instancesResult;
    EXPECT_EQ(instances.size(), 1);
    
    const auto& instance = instances[0];
    uint32_t instanceId = instanceManager.CreateInstance(
        instance.baseMeshId,
        instance.baseMaterialId,
        instance.transform,
        instance.colorOverride
    );
    
    EXPECT_NE(instanceId, 0);
    EXPECT_EQ(instanceManager.GetInstanceCount(), 1);
    
    // 5. Test runtime operations
    instanceManager.UpdateLODs(glm::vec3(0, 0, 0));
    instanceManager.UpdateCulling(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), 60.0f);
    
    auto metrics = instanceManager.GetMetrics();
    EXPECT_EQ(metrics.visibleInstances, 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}