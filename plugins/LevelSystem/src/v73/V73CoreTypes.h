// SecretEngine - v7.3 Level System Core Types
// Modern C++ Architecture for Android Performance
// Cache-aligned, GPU-optimized, Memory-efficient

#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SecretEngine::Levels::V73 {

// ============================================================================
// v7.3 Ultra Full Scale Configuration
// ============================================================================
struct V73Config {
    static constexpr uint32_t MAX_PLAYERS = 100;
    static constexpr uint32_t MAX_CHUNKS = 4;
    static constexpr uint32_t MAX_INSTANCES_PER_CHUNK = 10000;
    static constexpr uint32_t MAX_LIGHTS_PER_CHUNK = 50;
    
    // LOD distances from v7.3 specification
    static constexpr std::array<float, 3> DEFAULT_LOD_DISTANCES = {25.0f, 75.0f, 150.0f};
    
    // Transform settings (YXZ, degrees)
    static constexpr const char* ROTATION_FORMAT = "euler";
    static constexpr const char* ROTATION_UNIT = "degrees";
    static constexpr const char* ROTATION_ORDER = "YXZ";
    
    // Player defaults
    static constexpr float PLAYER_HEIGHT = 2.0f;
    static constexpr float PLAYER_RADIUS = 0.5f;
    static constexpr float PLAYER_SPEED = 5.0f;
};

// ============================================================================
// Cache-Aligned GPU Data (128 bytes for optimal cache performance)
// ============================================================================
struct alignas(128) TransformGPU {
    glm::mat4 modelMatrix;      // 64 bytes
    glm::vec4 color;            // 16 bytes
    uint32_t meshId;            // 4 bytes
    uint32_t materialId;        // 4 bytes
    uint32_t lodLevel;          // 4 bytes
    uint32_t flags;             // 4 bytes
    uint32_t instanceId;        // 4 bytes
    uint32_t chunkId;           // 4 bytes
    uint32_t _padding[6];       // 24 bytes to reach 128 bytes
    
    // Flags for GPU processing
    enum Flags : uint32_t {
        VISIBLE = 1 << 0,
        CAST_SHADOW = 1 << 1,
        RECEIVE_SHADOW = 1 << 2,
        DYNAMIC = 1 << 3,
        TRANSPARENT = 1 << 4
    };
};

static_assert(sizeof(TransformGPU) == 128, "TransformGPU must be 128 bytes for cache efficiency");

// ============================================================================
// Compact CPU Transform (48 bytes aligned)
// ============================================================================
struct alignas(16) TransformCompact {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  // w, x, y, z
    glm::vec3 scale{1.0f};
    
    TransformCompact() = default;
    
    TransformCompact(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl)
        : position(pos), rotation(rot), scale(scl) {}
    
    // Convert to GPU format
    TransformGPU ToGPU(uint32_t meshId, uint32_t materialId, uint32_t lodLevel, 
                       const glm::vec4& color = {1,1,1,1}, uint32_t flags = 0) const {
        TransformGPU result;
        result.modelMatrix = glm::translate(glm::mat4(1.0f), position) * 
                           glm::mat4_cast(rotation) * 
                           glm::scale(glm::mat4(1.0f), scale);
        result.color = color;
        result.meshId = meshId;
        result.materialId = materialId;
        result.lodLevel = lodLevel;
        result.flags = flags;
        result.instanceId = 0;  // Set by instance manager
        result.chunkId = 0;     // Set by chunk system
        return result;
    }
    
    // Helper functions
    glm::mat4 ToMatrix() const {
        return glm::translate(glm::mat4(1.0f), position) * 
               glm::mat4_cast(rotation) * 
               glm::scale(glm::mat4(1.0f), scale);
    }
    
    glm::vec3 Forward() const {
        return rotation * glm::vec3(0, 0, -1);  // -Z is forward
    }
    
    glm::vec3 Right() const {
        return rotation * glm::vec3(1, 0, 0);   // +X is right
    }
    
    glm::vec3 Up() const {
        return rotation * glm::vec3(0, 1, 0);   // +Y is up
    }
};

static_assert(sizeof(TransformCompact) == 48, "TransformCompact should be 48 bytes");

// ============================================================================
// v7.3 Player System (Memory-Efficient)
// ============================================================================
struct PlayerStats {
    int32_t health = 100;
    int32_t stamina = 100;
    
    struct SkillNode {
        std::string id;
        int32_t level = 1;
        
        float GetBonus() const { 
            return static_cast<float>(level) * 0.1f; 
        }
    };
    
    std::array<SkillNode, 3> skillTree;  // aim, speed, armor
    
    // Helper functions
    void ApplyDamage(int32_t damage) {
        health = std::max(0, health - damage);
    }
    
    void Heal(int32_t amount) {
        health = std::min(100, health + amount);
    }
    
    void ConsumeStamina(int32_t amount) {
        stamina = std::max(0, stamina - amount);
    }
    
    void RestoreStamina(int32_t amount) {
        stamina = std::min(100, stamina + amount);
    }
};

struct PlayerLoadout {
    std::string primary = "rifle";
    std::string secondary = "pistol";
    std::string tertiary = "knife";
    
    // Weapon stats (could be expanded)
    struct WeaponStats {
        float damage = 25.0f;
        float range = 100.0f;
        float fireRate = 1.0f;
    };
    
    WeaponStats GetPrimaryStats() const {
        // In production, would lookup from weapon database
        if (primary == "rifle") return {30.0f, 150.0f, 0.8f};
        if (primary == "shotgun") return {60.0f, 50.0f, 0.3f};
        return {25.0f, 100.0f, 1.0f};  // Default
    }
};

struct Player {
    uint32_t id;
    TransformCompact transform;
    PlayerStats stats;
    PlayerLoadout loadout;
    
    // Runtime state (not serialized)
    glm::vec3 velocity{0.0f};
    bool isActive = true;
    float lastUpdateTime = 0.0f;
    
    // Movement functions
    void UpdateMovement(float dt, const glm::vec3& input) {
        if (!isActive) return;
        
        // Apply input with speed bonus from skill tree
        float speedBonus = stats.skillTree[1].GetBonus();  // speed skill
        glm::vec3 movement = input * (V73Config::PLAYER_SPEED * (1.0f + speedBonus));
        
        // Update velocity (could add physics integration here)
        velocity = movement;
        
        // Update position
        transform.position += velocity * dt;
        
        lastUpdateTime += dt;
    }
    
    void LookAt(const glm::vec3& target) {
        glm::vec3 direction = glm::normalize(target - transform.position);
        transform.rotation = glm::quatLookAt(direction, glm::vec3(0, 1, 0));
    }
};

// ============================================================================
// LOD System
// ============================================================================
struct LODLevel {
    float distance;
    uint32_t meshId;
    uint32_t materialId;
    float screenSize;  // For dynamic LOD switching
    
    bool ShouldUse(float cameraDistance, float screenCoverage) const {
        return cameraDistance >= distance && screenCoverage < screenSize;
    }
};

struct LODConfig {
    bool enabled = true;
    std::vector<LODLevel> levels;
    float bias = 1.0f;  // LOD bias for quality adjustment
    
    int32_t GetLODLevel(float distance, float screenCoverage = 1.0f) const {
        if (!enabled || levels.empty()) return 0;
        
        float adjustedDistance = distance / bias;
        
        for (size_t i = 0; i < levels.size(); ++i) {
            if (levels[i].ShouldUse(adjustedDistance, screenCoverage)) {
                return static_cast<int32_t>(i);
            }
        }
        
        return static_cast<int32_t>(levels.size() - 1);  // Lowest LOD
    }
};

// ============================================================================
// Culling System
// ============================================================================
struct CullingConfig {
    enum class Method { Sphere, AABB, OBB };
    
    bool enabled = true;
    Method method = Method::Sphere;
    float radius = 1.0f;
    glm::vec3 boundsMin{-1.0f};
    glm::vec3 boundsMax{1.0f};
    
    bool IsVisible(const glm::vec3& position, const glm::vec3& cameraPos, 
                   const glm::vec3& cameraForward, float viewDistance) const {
        if (!enabled) return true;
        
        float distance = glm::distance(position, cameraPos);
        if (distance > viewDistance) return false;
        
        switch (method) {
            case Method::Sphere:
                return distance <= radius;
                
            case Method::AABB: {
                glm::vec3 min = position + boundsMin;
                glm::vec3 max = position + boundsMax;
                // Simplified AABB vs camera frustum check
                return distance <= viewDistance;
            }
            
            case Method::OBB:
                // More complex OBB culling would go here
                return distance <= viewDistance;
        }
        
        return true;
    }
};

// ============================================================================
// Chunk Instance
// ============================================================================
struct ChunkInstance {
    TransformCompact transform;
    uint32_t baseMeshId;
    uint32_t baseMaterialId;
    glm::vec4 colorOverride{1.0f};
    LODConfig lodConfig;
    CullingConfig cullingConfig;
    
    // Runtime state
    uint32_t runtimeInstanceId = 0;
    int32_t currentLOD = -1;
    bool isVisible = true;
    bool isDirty = true;
    
    void UpdateLOD(float cameraDistance, float screenCoverage = 1.0f) {
        int32_t newLOD = lodConfig.GetLODLevel(cameraDistance, screenCoverage);
        if (newLOD != currentLOD) {
            currentLOD = newLOD;
            isDirty = true;
        }
    }
    
    uint32_t GetCurrentMeshId() const {
        if (currentLOD >= 0 && currentLOD < static_cast<int32_t>(lodConfig.levels.size())) {
            return lodConfig.levels[currentLOD].meshId;
        }
        return baseMeshId;
    }
    
    uint32_t GetCurrentMaterialId() const {
        if (currentLOD >= 0 && currentLOD < static_cast<int32_t>(lodConfig.levels.size())) {
            return lodConfig.levels[currentLOD].materialId;
        }
        return baseMaterialId;
    }
};

// ============================================================================
// Light Data
// ============================================================================
struct LightData {
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float radius = 10.0f;
    bool castsShadow = true;
    
    enum Type : uint32_t {
        DIRECTIONAL = 0,
        POINT = 1,
        SPOT = 2
    } type = POINT;
    
    // For spot lights
    glm::vec3 direction{0, -1, 0};
    float innerCone = 30.0f;
    float outerCone = 45.0f;
};

// ============================================================================
// Chunk Bounds
// ============================================================================
struct ChunkBounds {
    glm::vec3 center{0.0f};
    glm::vec3 extents{50.0f};
    
    bool Contains(const glm::vec3& point) const {
        return std::abs(point.x - center.x) <= extents.x &&
               std::abs(point.y - center.y) <= extents.y &&
               std::abs(point.z - center.z) <= extents.z;
    }
    
    float DistanceTo(const glm::vec3& point) const {
        glm::vec3 diff = point - center;
        float dx = std::max(0.0f, std::abs(diff.x) - extents.x);
        float dy = std::max(0.0f, std::abs(diff.y) - extents.y);
        float dz = std::max(0.0f, std::abs(diff.z) - extents.z);
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
    
    bool Intersects(const ChunkBounds& other) const {
        return std::abs(center.x - other.center.x) <= (extents.x + other.extents.x) &&
               std::abs(center.y - other.center.y) <= (extents.y + other.extents.y) &&
               std::abs(center.z - other.center.z) <= (extents.z + other.extents.z);
    }
    
    // Get all 8 corners of the bounding box
    std::array<glm::vec3, 8> GetCorners() const {
        return {{
            center + glm::vec3(-extents.x, -extents.y, -extents.z),
            center + glm::vec3( extents.x, -extents.y, -extents.z),
            center + glm::vec3(-extents.x,  extents.y, -extents.z),
            center + glm::vec3( extents.x,  extents.y, -extents.z),
            center + glm::vec3(-extents.x, -extents.y,  extents.z),
            center + glm::vec3( extents.x, -extents.y,  extents.z),
            center + glm::vec3(-extents.x,  extents.y,  extents.z),
            center + glm::vec3( extents.x,  extents.y,  extents.z)
        }};
    }
};

// ============================================================================
// Performance Metrics
// ============================================================================
struct PerformanceMetrics {
    uint32_t visibleChunks = 0;
    uint32_t visibleInstances = 0;
    uint32_t drawCalls = 0;
    uint32_t triangles = 0;
    float gpuMemoryMB = 0.0f;
    float cpuTimeMs = 0.0f;
    float gpuTimeMs = 0.0f;
    float fps = 0.0f;
    
    // Memory breakdown
    size_t instanceMemoryBytes = 0;
    size_t chunkMemoryBytes = 0;
    size_t playerMemoryBytes = 0;
    
    void Reset() {
        *this = PerformanceMetrics{};
    }
    
    float GetTotalMemoryMB() const {
        return static_cast<float>(instanceMemoryBytes + chunkMemoryBytes + playerMemoryBytes) / (1024.0f * 1024.0f);
    }
};

} // namespace SecretEngine::Levels::V73