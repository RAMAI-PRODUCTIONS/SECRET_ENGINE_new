// SecretEngine - v7.3 Ultra Full Level System
// Complete implementation based on v7_3_ultra_full specification
// Multi-disciplinary approach: Game Design, Art, Programming, Audio, Production

#pragma once
#include <SecretEngine/ICore.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/ILogger.h>
#include <SecretEngine/Components.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>
#include <array>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

namespace SecretEngine::Levels::V73 {

// ============================================================================
// v7.3 Configuration Constants
// ============================================================================
struct V73Config {
    static constexpr uint32_t MAX_PLAYERS = 100;
    static constexpr uint32_t MAX_CHUNKS = 4;
    static constexpr uint32_t MAX_INSTANCES_PER_CHUNK = 10000;
    static constexpr uint32_t MAX_LIGHTS_PER_CHUNK = 50;
    static constexpr uint32_t MAX_OBJECTIVES = 10;
    static constexpr uint32_t MAX_SPAWN_POINTS = 32;
    
    // LOD distances from specification
    static constexpr std::array<float, 4> LOD_DISTANCES = {25.0f, 75.0f, 150.0f, 300.0f};
    
    // Performance budgets
    static constexpr size_t MAX_TEXTURE_MEMORY = 2048ULL * 1024 * 1024; // 2GB
    static constexpr size_t MAX_MESH_MEMORY = 1024ULL * 1024 * 1024;    // 1GB
    static constexpr size_t MAX_AUDIO_MEMORY = 512ULL * 1024 * 1024;    // 512MB
    
    // Network settings
    static constexpr uint32_t SERVER_TICK_RATE = 60;
    static constexpr uint32_t MAX_NETWORK_PLAYERS = 64;
};

// ============================================================================
// Core Transform System (Cache-Aligned)
// ============================================================================
struct alignas(16) Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  // w, x, y, z
    glm::vec3 scale{1.0f};
    
    Transform() = default;
    Transform(const glm::vec3& pos, const glm::quat& rot = glm::quat(1,0,0,0), const glm::vec3& scl = glm::vec3(1))
        : position(pos), rotation(rot), scale(scl) {}
    
    glm::mat4 ToMatrix() const {
        return glm::translate(glm::mat4(1.0f), position) * 
               glm::mat4_cast(rotation) * 
               glm::scale(glm::mat4(1.0f), scale);
    }
    
    glm::vec3 Forward() const { return rotation * glm::vec3(0, 0, -1); }
    glm::vec3 Right() const { return rotation * glm::vec3(1, 0, 0); }
    glm::vec3 Up() const { return rotation * glm::vec3(0, 1, 0); }
    
    // Convert from Euler angles (YXZ order, degrees)
    static Transform FromEulerYXZ(const glm::vec3& pos, const glm::vec3& eulerDegrees, const glm::vec3& scl = glm::vec3(1)) {
        glm::vec3 radians = glm::radians(eulerDegrees);
        glm::quat qY = glm::angleAxis(radians.y, glm::vec3(0, 1, 0));
        glm::quat qX = glm::angleAxis(radians.x, glm::vec3(1, 0, 0));
        glm::quat qZ = glm::angleAxis(radians.z, glm::vec3(0, 0, 1));
        return Transform(pos, qY * qX * qZ, scl);
    }
};

// ============================================================================
// Player System (v7.3 Specification)
// ============================================================================
struct PlayerStats {
    struct Health {
        int32_t current = 100;
        int32_t max = 100;
        float regen_rate = 2.0f;
        float regen_delay = 5.0f;
        
        struct Armor {
            int32_t current = 50;
            int32_t max = 100;
            float damage_reduction = 0.3f;
        } armor;
        
        struct Shield {
            int32_t current = 75;
            int32_t max = 100;
            float regen_rate = 5.0f;
            float regen_delay = 3.0f;
        } shield;
    } health;
    
    struct Stamina {
        int32_t current = 100;
        int32_t max = 100;
        float drain_rate_sprint = 15.0f;
        float drain_rate_jump = 10.0f;
        float drain_rate_slide = 20.0f;
        float drain_rate_melee = 25.0f;
        float regen_rate = 20.0f;
        float regen_delay = 1.5f;
    } stamina;
    
    struct Energy {
        int32_t current = 100;
        int32_t max = 100;
        float regen_rate = 10.0f;
    } energy;
};

struct PlayerMovement {
    struct Speed {
        float walk = 3.5f;
        float run = 5.5f;
        float sprint = 7.5f;
        float crouch = 2.0f;
        float aim = 2.5f;
        float slide = 12.0f;
    } speed;
    
    struct Jump {
        float height = 1.5f;
        float air_control = 0.3f;
        bool double_jump = false;
        bool wall_run = true;
        bool mantle = true;
    } jump;
};

struct WeaponAttachment {
    std::string slot;  // "sight", "barrel", "grip"
    std::string id;    // "red_dot", "compensator", "vertical"
};

struct Weapon {
    std::string id;
    std::string name;
    
    struct Ammo {
        int32_t current = 30;
        int32_t reserve = 120;
    } ammo;
    
    std::vector<WeaponAttachment> attachments;
    std::string skin = "default";
    
    struct Stats {
        int32_t damage = 32;
        int32_t fire_rate = 800;
        float range = 50.0f;
        float accuracy = 0.85f;
        float reload_time = 2.2f;
    } stats;
};

struct PlayerLoadout {
    Weapon primary;
    Weapon secondary;
    Weapon melee;
    
    struct Throwables {
        std::string id;
        int32_t count;
    };
    std::vector<Throwables> throwables;
};

struct SkillNode {
    std::string id;
    std::string name;
    int32_t level = 1;
    int32_t max_level = 5;
    
    struct Effects {
        float accuracy = 0.0f;
        float recoil = 0.0f;
        float reload_time = 0.0f;
        int32_t max_armor = 0;
        float regen_rate = 0.0f;
        float move_speed = 0.0f;
        float sprint_duration = 0.0f;
        float slide_distance = 0.0f;
    } effects;
};

struct SkillTree {
    int32_t total_points = 15;
    
    struct Branch {
        std::string name;
        std::vector<SkillNode> nodes;
    };
    
    std::array<Branch, 3> branches; // Assault, Survival, Mobility
};

struct Player {
    uint32_t id;
    std::string username;
    int32_t level = 42;
    int32_t xp = 12450;
    int32_t prestige = 2;
    std::string region = "EU";
    std::string language = "en";
    
    Transform transform;
    PlayerStats stats;
    PlayerMovement movement;
    PlayerLoadout loadout;
    SkillTree skill_tree;
    
    // Progression
    struct Challenge {
        std::string id;
        int32_t progress;
        int32_t target;
        int32_t reward_xp;
    };
    std::vector<Challenge> challenges;
    
    // Social
    std::vector<uint32_t> friends;
    std::string clan_name;
    std::string clan_tag;
    
    // Settings
    struct Controls {
        float mouse_sensitivity = 5.0f;
        float controller_sensitivity = 4.0f;
        float aim_sensitivity_multiplier = 0.7f;
    } controls;
};

// ============================================================================
// Level Environment System
// ============================================================================
struct WeatherPattern {
    std::string name;
    int32_t duration_min = 180;
    int32_t duration_max = 300;
    float visibility = 1.0f;
    float wind_speed = 2.0f;
    float precipitation = 0.0f;
    float lightning_frequency = 0.0f;
    float cloud_coverage = 0.1f;
    std::string particle_effect;
    std::string sound_effect;
    float damage_per_second = 0.0f;
};

struct TimeOfDay {
    bool cycle_enabled = true;
    int32_t duration_seconds = 1800;
    int32_t current_time = 43200; // noon
    
    struct Sun {
        glm::vec3 direction{-0.5f, -0.8f, 0.3f};
        glm::vec3 color{1.0f, 0.95f, 0.85f};
        float intensity = 1.0f;
        int32_t shadow_cascades = 4;
        float shadow_distance = 200.0f;
    } sun;
    
    struct Moon {
        glm::vec3 direction{0.5f, 0.3f, -0.6f};
        glm::vec3 color{0.7f, 0.75f, 1.0f};
        float intensity = 0.3f;
    } moon;
};

struct DynamicEvent {
    std::string name;
    float probability = 0.05f;
    std::array<float, 2> intensity_range{3.0f, 7.0f};
    std::array<float, 2> duration_range{5.0f, 15.0f};
    std::vector<std::string> effects;
};

// ============================================================================
// Chunk System (Spatial Partitioning)
// ============================================================================
struct MeshInstance {
    uint32_t id;
    Transform transform;
    uint32_t mesh_id;
    uint32_t material_id;
    glm::vec4 color{1.0f};
    
    struct LOD {
        struct Level {
            float distance;
            std::string mesh_path;
        };
        std::vector<Level> levels;
    } lod;
    
    struct Culling {
        float radius = 1.0f;
        glm::vec3 bounds_min{-1.0f};
        glm::vec3 bounds_max{1.0f};
    } culling;
    
    struct Tags {
        std::string type = "mesh";
        bool destructible = false;
        int32_t health = 100;
        bool lootable = false;
    } tags;
};

struct LightSource {
    enum Type { Directional, Point, Spot, Area };
    
    Type type = Point;
    Transform transform;
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float radius = 15.0f;
    bool cast_shadows = true;
    
    // Spot light specific
    float inner_cone = 30.0f;
    float outer_cone = 45.0f;
    
    // Area light specific
    float width = 10.0f;
    float height = 10.0f;
    
    // Dynamic properties
    struct Flicker {
        bool enabled = false;
        float speed = 2.0f;
        std::array<float, 2> intensity_range{2.5f, 3.5f};
    } flicker;
};

struct ParticleSystem {
    std::string id;
    std::string type; // "ambient", "effect", "weather"
    glm::vec3 position;
    float radius = 100.0f;
    int32_t particle_count = 500;
    std::string texture;
    glm::vec3 color{0.7f, 0.65f, 0.6f};
    std::array<float, 2> size_range{0.05f, 0.2f};
    glm::vec3 velocity{0.1f, 0.05f, 0.1f};
    float lifetime = 5.0f;
};

struct AudioZone {
    std::string id;
    glm::vec3 position;
    float radius = 150.0f;
    std::string ambient_track;
    float volume = 0.7f;
    std::string reverb_type = "outdoor";
    
    struct Reverb {
        std::string type = "cave";
        float amount = 0.8f;
        float decay = 3.0f;
    } reverb;
    
    std::string footstep_override;
};

struct Trigger {
    std::string id;
    std::string type; // "area", "proximity", "interaction"
    std::string shape; // "sphere", "box", "cylinder"
    glm::vec3 position;
    float radius = 20.0f;
    
    struct Action {
        std::string type; // "play_sound", "spawn_enemies", "teleport", "load_level"
        std::string sound;
        int32_t count = 0;
        float delay = 0.0f;
        glm::vec3 target_position;
        std::string target_level; // For level switching
    };
    
    std::vector<Action> enter_actions;
    std::vector<Action> exit_actions;
    bool repeatable = false;
    float cooldown = 30.0f;
};

struct ObjectiveZone {
    std::string id;
    glm::vec3 position;
    float radius = 15.0f;
    std::string type; // "capture_point", "king_of_the_hill", "flag"
    float capture_time = 10.0f;
    int32_t score_per_second = 5;
    float team_control_multiplier = 1.5f;
    int32_t current_team = -1; // -1 = neutral
};

struct SpawnPoint {
    glm::vec3 position;
    glm::vec3 rotation; // Euler angles
    int32_t team = -1; // -1 = any team
    std::string type = "player"; // "player", "vehicle", "pickup"
    float respawn_time = 0.0f;
    bool enabled = true;
};

struct Chunk {
    std::string chunk_id;
    glm::vec3 bounds_min;
    glm::vec3 bounds_max;
    glm::vec3 bounds_center;
    
    std::vector<MeshInstance> mesh_instances;
    std::vector<LightSource> lights;
    std::vector<ParticleSystem> particles;
    std::vector<AudioZone> audio_zones;
    std::vector<Trigger> triggers;
    std::vector<ObjectiveZone> objectives;
    std::vector<SpawnPoint> spawn_points;
    
    // Performance data
    struct Performance {
        int32_t max_visible_instances = 100;
        std::string update_priority = "high";
        float culling_distance = 500.0f;
        bool occlusion_culling = true;
        bool instancing_enabled = true;
    } performance;
    
    // Streaming data
    struct Streaming {
        int32_t priority = 1;
        float preload_radius = 100.0f;
        float unload_delay = 5.0f;
    } streaming;
    
    bool is_loaded = false;
    bool is_visible = false;
};

// ============================================================================
// Level Definition (Complete v7.3 Specification)
// ============================================================================
struct Level {
    std::string level_id;
    std::string version = "2.0";
    std::string name;
    std::string description;
    int32_t difficulty_rating = 3;
    std::array<int32_t, 3> recommended_players{4, 8, 12};
    
    // Environment
    struct Environment {
        std::string terrain_type = "ruins";
        std::string heightmap;
        glm::vec3 terrain_scale{1000.0f, 50.0f, 1000.0f};
        std::vector<std::string> ground_materials{"stone", "sand", "gravel", "grass"};
        
        struct Skybox {
            std::string cubemap;
            std::string clouds;
            bool stars_visible = false;
        } skybox;
        
        struct AmbientLighting {
            glm::vec3 color{0.3f, 0.35f, 0.4f};
            float intensity = 0.5f;
            float occlusion_strength = 0.8f;
        } ambient_lighting;
        
        struct Fog {
            bool enabled = true;
            glm::vec3 color{0.6f, 0.55f, 0.5f};
            float start_distance = 50.0f;
            float end_distance = 300.0f;
            float density = 0.02f;
        } fog;
    } environment;
    
    TimeOfDay time_of_day;
    
    struct WeatherSystem {
        std::string current_weather = "clear";
        float transition_time = 30.0f;
        std::vector<WeatherPattern> patterns;
    } weather_system;
    
    std::vector<DynamicEvent> dynamic_events;
    
    // Boundaries
    struct Boundaries {
        glm::vec3 playable_min{0.0f, -10.0f, 0.0f};
        glm::vec3 playable_max{400.0f, 100.0f, 400.0f};
        float out_of_bounds_damage = 10.0f;
        float return_timeout = 5.0f;
        
        struct KillZone {
            glm::vec3 position;
            float radius;
            float damage;
            std::string type; // "lava", "void", "radiation"
        };
        std::vector<KillZone> kill_zones;
        
        struct SafeZone {
            glm::vec3 position;
            float radius;
            float heal_rate;
            float shield_regen;
        };
        std::vector<SafeZone> safe_zones;
    } boundaries;
    
    // Audio environment
    struct AudioEnvironment {
        std::string ambient_track;
        std::string combat_music;
        std::vector<AudioZone> reverb_zones;
        std::unordered_map<std::string, std::string> footstep_materials;
    } audio_environment;
    
    // Visual post-processing
    struct PostProcessing {
        struct Bloom {
            float intensity = 0.4f;
            float threshold = 0.9f;
        } bloom;
        
        struct ColorGrading {
            std::string lut;
            float saturation = 1.1f;
            float contrast = 1.05f;
        } color_grading;
        
        struct DepthOfField {
            bool enabled = false;
            float focus_distance = 100.0f;
            float blur_strength = 2.0f;
        } depth_of_field;
        
        struct MotionBlur {
            bool enabled = true;
            float intensity = 0.3f;
        } motion_blur;
        
        struct Vignette {
            float intensity = 0.2f;
            float smoothness = 0.5f;
        } vignette;
    } post_processing;
    
    // Performance budgets
    struct PerformanceBudgets {
        int32_t max_visible_instances = 500;
        int32_t max_lights = 50;
        int32_t max_particles = 10000;
        
        struct CullingDistances {
            float player = 300.0f;
            float weapon = 200.0f;
            float pickup = 150.0f;
            float effect = 100.0f;
            float decoration = 80.0f;
        } culling_distances;
        
        float lod_bias = 1.0f;
        int32_t shadow_cascades = 4;
    } performance_budgets;
    
    // Server configuration
    struct ServerConfig {
        int32_t tick_rate = 60;
        int32_t max_players = 32;
        bool allow_spectators = true;
        int32_t warmup_time = 30;
        int32_t round_time = 600;
        int32_t overtime = 60;
        int32_t score_limit = 100;
    } server_config;
    
    // Chunks
    std::vector<Chunk> chunks;
    
    // Players
    std::vector<Player> players;
    
    // Game mode specific data
    nlohmann::json gameplay_data;
    nlohmann::json progression_data;
    nlohmann::json monetization_data;
};

} // namespace SecretEngine::Levels::V73