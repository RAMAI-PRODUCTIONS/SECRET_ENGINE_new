# Call of Duty-Style FPS Game - Complete Implementation Plan

**Project**: SecretEngine FPS Game  
**Target**: COD Mobile Parity  
**Date**: February 10, 2026  
**Status**: Ready for Implementation

---

## 📊 Executive Summary

This document consolidates two AI-generated FPS game architectures (Claude and ChatGPT) against your existing SecretEngine codebase to create a complete implementation plan for a Call of Duty-style mobile FPS game.

### Current Engine Status
- ✅ **VulkanRenderer** - 6M triangles @ 89 FPS (Adreno 619)
- ✅ **CameraPlugin** - Modular first-person camera
- ✅ **AndroidInput** - Touch input with joystick
- ✅ **DebugPlugin** - Performance monitoring
- ✅ **Fast Data Architecture** - Sub-100ns packet communication
- ✅ **ECS System** - Entity/Component/System architecture

### What's Missing for FPS Game
- ✅ Physics Plugin (collision, raycasting) - **See FPS_COMPLETE_FEATURES.md**
- ✅ Weapon Pickup/Drop System - **See FPS_COMPLETE_FEATURES.md**
- ✅ Jump & Ground Detection - **See FPS_COMPLETE_FEATURES.md**
- ✅ Complete Movement System - **See FPS_COMPLETE_FEATURES.md**
- ❌ AI/Bot System (basic in reference docs)
- ❌ NavMesh/Pathfinding
- ❌ Animation System
- ❌ Audio System
- ❌ Network Layer (Rust + C++)

---

## 🏗️ Architecture Comparison

### Claude's Approach (Data-Oriented, Recommended)

**Philosophy**: Pure ECS, Fast Data Architecture, Mobile-First

**Structure**:
```
Single Plugin: FPSGameLogic
├── FPSComponents.h      (Pure POD data, no methods)
├── FPSFastData.h        (8-byte packets, lock-free queues)
├── FPSSystems.h         (Stateless system functions)
└── FPSGamePlugin.h      (Plugin orchestration)
```

**Key Features**:
- ✅ Follows SecretEngine's strict architectural principles
- ✅ Sub-100ns communication via 8-byte packets
- ✅ Zero allocation in hot paths
- ✅ Handle-based references (no raw pointers)
- ✅ Integrates seamlessly with existing plugins
- ✅ Mobile performance budgets (2ms for all game logic)

**Components** (17 total):
- TeamComponent, HealthComponent, KCCComponent, WeaponComponent
- AIComponent, RespawnComponent, StatsComponent, PredictionComponent
- HitboxComponent, MatchState, SpawnSystemState

**Systems** (10 total):
- MovementSystem, WeaponSystem, CombatSystem, AISystem
- RespawnSystem, SpawnSystem, MatchSystem, PredictionSystem
- HitboxSystem

**Performance Budget**:
| System | Time | % of Frame |
|--------|------|------------|
| Movement | 0.5ms | 3% |
| Weapons | 0.3ms | 1.8% |
| Combat | 0.2ms | 1.2% |
| AI | 1.0ms | 6% |
| **Total** | **2.0ms** | **12%** |

---

### ChatGPT's Approach (Plugin-Based)

**Philosophy**: Multiple specialized plugins, traditional OOP

**Structure**:
```
Multiple Plugins:
├── PhysicsPlugin       (Collision, raycasting)
├── WeaponPlugin        (Fire, reload, ammo)
├── BotPlugin           (AI behaviors)
├── GameModePlugin      (Score, match state)
└── Common/             (Shared math utilities)
```

**Key Features**:
- ✅ Clear separation of concerns
- ✅ Easy to understand for traditional game devs
- ⚠️ More plugin-to-plugin dependencies
- ⚠️ Traditional method calls (slower than Fast Data)
- ⚠️ Not fully aligned with SecretEngine principles

**Load Order**:
1. InputPlugin
2. CameraPlugin
3. PhysicsPlugin
4. GameModePlugin
5. WeaponPlugin
6. BotPlugin
7. RendererPlugin

---

## 🎯 Recommended Hybrid Approach

**Best of Both Worlds**: Use Claude's data-oriented architecture with ChatGPT's plugin separation

### Plugin Structure

```
plugins/
├── PhysicsPlugin/              ← NEW (ChatGPT inspired)
│   ├── src/
│   │   ├── PhysicsPlugin.h
│   │   ├── PhysicsTypes.h      (Collision, raycasting)
│   │   └── PhysicsSystem.cpp
│   └── CMakeLists.txt
│
├── FPSGameLogic/               ← NEW (Claude's architecture)
│   ├── src/
│   │   ├── FPSComponents.h     (Pure data components)
│   │   ├── FPSFastData.h       (8-byte packets)
│   │   ├── FPSSystems.h        (Game logic systems)
│   │   ├── FPSGamePlugin.h     (Main plugin)
│   │   └── FPSGamePlugin.cpp
│   └── CMakeLists.txt
│
├── AIPlugin/                   ← NEW (Bot behaviors)
│   ├── src/
│   │   ├── AIPlugin.h
│   │   ├── BehaviorTree.h
│   │   └── NavMesh.h
│   └── CMakeLists.txt
│
├── WeaponPlugin/               ← NEW (Weapon mechanics)
│   ├── src/
│   │   ├── WeaponPlugin.h
│   │   ├── WeaponTypes.h
│   │   └── BallisticsSystem.cpp
│   └── CMakeLists.txt
│
├── AudioPlugin/                ← NEW (3D audio)
│   └── src/AudioPlugin.h
│
├── AnimationPlugin/            ← NEW (Skeletal animation)
│   └── src/AnimationPlugin.h
│
└── NetworkPlugin/              ← NEW (Rust FFI)
    ├── src/NetworkPlugin.h     (C++ wrapper)
    └── rust/                   (Rust server code)
```

---

## 📦 Phase 1: Core Movement & Physics (Week 1-2)

### 1.1 Physics Plugin

**File**: `plugins/PhysicsPlugin/src/PhysicsPlugin.h`

```cpp
#pragma once
#include "SecretEngine/IPlugin.h"
#include "PhysicsTypes.h"

namespace SecretEngine::Physics {

class PhysicsPlugin : public IPlugin {
public:
    const char* GetName() const override { return "Physics"; }
    
    void OnLoad(ICore* core) override;
    void OnUpdate(float deltaTime) override;
    
    // Raycast API
    struct RaycastHit {
        bool hit;
        uint32_t entityId;
        glm::vec3 point;
        glm::vec3 normal;
        float distance;
    };
    
    RaycastHit Raycast(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        uint32_t layerMask = 0xFFFFFFFF
    );
    
    // Collision API
    bool CheckCapsule(
        const glm::vec3& start,
        const glm::vec3& end,
        float radius,
        uint32_t layerMask = 0xFFFFFFFF
    );
    
private:
    ICore* m_core = nullptr;
    IWorld* m_world = nullptr;
    
    // Spatial partitioning (octree or grid)
    struct SpatialGrid {
        // ... implementation
    };
    SpatialGrid m_grid;
};

} // namespace SecretEngine::Physics
```

**Key Features**:
- Raycasting for weapon hits
- Capsule collision for character controller
- Simple spatial partitioning (grid or octree)
- No external physics library (deterministic)

**Performance Target**: < 0.5ms for 100 entities

---

### 1.2 FPS Game Logic Plugin

**File**: `plugins/FPSGameLogic/src/FPSGamePlugin.h`

Use Claude's complete implementation (already provided in reference docs).

**Components to Implement First**:
1. `KCCComponent` - Kinematic character controller
2. `HealthComponent` - HP tracking
3. `TeamComponent` - Red vs Blue
4. `WeaponComponent` - Basic weapon state

**Systems to Implement First**:
1. `MovementSystem` - WASD movement, jumping
2. `WeaponSystem` - Fire, reload, ammo
3. `CombatSystem` - Damage application

**Integration Points**:
```cpp
// AndroidInput → FPS Game
Fast::PlayerMovePacket packet;
packet.entityId = playerId;
packet.forward = touchDeltaY * 127;
fpsGame->GetFastStreams().moveInput.Push(packet);

// FPS Game → Physics
auto hit = physics->Raycast(origin, direction, 100.0f);
if (hit.hit) {
    ApplyDamage(hit.entityId, 25.0f);
}

// FPS Game → Renderer
Fast::MuzzleFlashPacket flash;
flash.entityId = shooterId;
fpsGame->GetFastStreams().muzzleFlashes.Push(flash);
```

---

## 📦 Phase 2: AI & Bots (Week 3-4)

### 2.1 AI Plugin

**File**: `plugins/AIPlugin/src/AIPlugin.h`

```cpp
#pragma once
#include "SecretEngine/IPlugin.h"
#include "BehaviorTree.h"

namespace SecretEngine::AI {

class AIPlugin : public IPlugin {
public:
    const char* GetName() const override { return "AI"; }
    
    void OnLoad(ICore* core) override;
    void OnUpdate(float deltaTime) override;
    
    // Bot behavior API
    void RegisterBot(uint32_t entityId, TeamComponent::Team team);
    void UnregisterBot(uint32_t entityId);
    
private:
    ICore* m_core = nullptr;
    IWorld* m_world = nullptr;
    
    struct BotState {
        enum class Behavior {
            Idle, Patrol, Chase, Attack, TakeCover, Retreat
        };
        
        Behavior current;
        uint32_t targetEntity;
        glm::vec3 targetPosition;
        float behaviorTimer;
    };
    
    std::unordered_map<uint32_t, BotState> m_bots;
    
    void UpdateBehavior(uint32_t botId, BotState& state, float dt);
    uint32_t FindNearestEnemy(uint32_t botId, float range);
    bool HasLineOfSight(uint32_t botId, uint32_t targetId);
};

} // namespace SecretEngine::AI
```

**Bot Behaviors**:
1. **Idle** - Stand still, look around
2. **Patrol** - Walk between waypoints
3. **Chase** - Move toward enemy
4. **Attack** - Fire at enemy
5. **TakeCover** - Find cover point
6. **Retreat** - Low health, fall back

**Performance Target**: < 1.0ms for 10 bots

---

### 2.2 NavMesh Plugin (Optional for Phase 2)

**File**: `plugins/NavMeshPlugin/src/NavMeshPlugin.h`

```cpp
#pragma once
#include "SecretEngine/IPlugin.h"

namespace SecretEngine::NavMesh {

class NavMeshPlugin : public IPlugin {
public:
    const char* GetName() const override { return "NavMesh"; }
    
    // Pathfinding API
    std::vector<glm::vec3> FindPath(
        const glm::vec3& start,
        const glm::vec3& end
    );
    
    // NavMesh generation
    void GenerateFromGeometry(
        const std::vector<glm::vec3>& vertices,
        const std::vector<uint32_t>& indices
    );
    
private:
    struct NavMeshNode {
        glm::vec3 center;
        std::vector<uint32_t> neighbors;
    };
    
    std::vector<NavMeshNode> m_nodes;
    
    // A* pathfinding
    std::vector<glm::vec3> AStar(uint32_t startNode, uint32_t endNode);
};

} // namespace SecretEngine::NavMesh
```

**Alternative**: Use simple waypoint system for Phase 2, implement full NavMesh in Phase 3.

---

## 📦 Phase 3: Weapons & Combat (Week 5-6)

### 3.1 Weapon Plugin

**File**: `plugins/WeaponPlugin/src/WeaponPlugin.h`

```cpp
#pragma once
#include "SecretEngine/IPlugin.h"
#include "WeaponTypes.h"

namespace SecretEngine::Weapons {

class WeaponPlugin : public IPlugin {
public:
    const char* GetName() const override { return "Weapons"; }
    
    void OnLoad(ICore* core) override;
    void OnUpdate(float deltaTime) override;
    
    // Weapon API
    void Fire(uint32_t entityId, const glm::vec3& origin, const glm::vec3& direction);
    void Reload(uint32_t entityId);
    void SwitchWeapon(uint32_t entityId, uint8_t slot);
    
    // Weapon definitions
    struct WeaponDef {
        const char* name;
        float damage;
        float fireRate;
        float range;
        float spread;
        uint32_t magSize;
        float reloadTime;
    };
    
    static const WeaponDef WEAPON_RIFLE;
    static const WeaponDef WEAPON_SHOTGUN;
    static const WeaponDef WEAPON_SNIPER;
    
private:
    ICore* m_core = nullptr;
    IWorld* m_world = nullptr;
    
    void ProcessHitscan(
        uint32_t shooterId,
        const glm::vec3& origin,
        const glm::vec3& direction,
        const WeaponDef& weapon
    );
};

} // namespace SecretEngine::Weapons
```

**Weapon Types**:
1. **Assault Rifle** - 30 rounds, 600 RPM, medium damage
2. **Shotgun** - 8 rounds, 60 RPM, high damage, spread
3. **Sniper** - 5 rounds, 40 RPM, very high damage, no spread
4. **Pistol** - 15 rounds, 300 RPM, low damage

**Ballistics**:
- Hitscan (instant) for rifles/pistols
- Projectile (simulated) for grenades (future)
- Spread cone for shotguns
- Recoil patterns

---

## 📦 Phase 4: Audio & Polish (Week 7-8)

### 4.1 Audio Plugin

**File**: `plugins/AudioPlugin/src/AudioPlugin.h`

```cpp
#pragma once
#include "SecretEngine/IPlugin.h"

namespace SecretEngine::Audio {

class AudioPlugin : public IPlugin {
public:
    const char* GetName() const override { return "Audio"; }
    
    void OnLoad(ICore* core) override;
    void OnUpdate(float deltaTime) override;
    
    // 3D audio API
    void PlaySound(
        const char* soundName,
        const glm::vec3& position,
        float volume = 1.0f,
        float pitch = 1.0f
    );
    
    void PlayMusic(const char* musicName, bool loop = true);
    void StopMusic();
    
private:
    ICore* m_core = nullptr;
    
    // Audio backend (OpenSL ES on Android, XAudio2 on Windows)
    void* m_audioEngine = nullptr;
    
    struct SoundInstance {
        uint32_t id;
        glm::vec3 position;
        float volume;
        bool is3D;
    };
    
    std::vector<SoundInstance> m_activeSounds;
};

} // namespace SecretEngine::Audio
```

**Sound Effects**:
- Gunfire (per weapon type)
- Footsteps (per surface type)
- Bullet impacts
- Reload sounds
- UI sounds (menu clicks)

**3D Audio**:
- Distance attenuation
- Doppler effect (optional)
- Occlusion (simple raycast)

---

### 4.2 Animation Plugin

**File**: `plugins/AnimationPlugin/src/AnimationPlugin.h`

```cpp
#pragma once
#include "SecretEngine/IPlugin.h"

namespace SecretEngine::Animation {

class AnimationPlugin : public IPlugin {
public:
    const char* GetName() const override { return "Animation"; }
    
    void OnLoad(ICore* core) override;
    void OnUpdate(float deltaTime) override;
    
    // Animation API
    void PlayAnimation(uint32_t entityId, const char* animName, bool loop = false);
    void BlendAnimation(uint32_t entityId, const char* animName, float blendTime);
    
    // Skeletal animation
    struct Skeleton {
        std::vector<glm::mat4> boneTransforms;
        std::vector<uint32_t> boneParents;
    };
    
    void UpdateSkeleton(uint32_t entityId, Skeleton& skeleton, float deltaTime);
    
private:
    ICore* m_core = nullptr;
    IWorld* m_world = nullptr;
    
    struct AnimationState {
        const char* currentAnim;
        float time;
        float blendFactor;
    };
    
    std::unordered_map<uint32_t, AnimationState> m_animStates;
};

} // namespace SecretEngine::Animation
```

**Animations Needed**:
- Idle
- Walk/Run
- Sprint
- Crouch
- Jump
- Fire weapon
- Reload
- Death

---

## 📦 Phase 5: Networking (Week 9-12)

### 5.1 Network Architecture (Rust + C++)

**Rust Server** (`network_rs/src/lib.rs`):

```rust
use tokio::net::UdpSocket;
use quinn::{Endpoint, ServerConfig};
use serde::{Serialize, Deserialize};

#[derive(Serialize, Deserialize)]
struct PlayerState {
    entity_id: u32,
    position: [f32; 3],
    rotation: [f32; 4],
    health: f32,
    team: u8,
}

#[derive(Serialize, Deserialize)]
struct GameState {
    tick: u64,
    players: Vec<PlayerState>,
    match_time: f32,
    red_score: u32,
    blue_score: u32,
}

pub struct GameServer {
    endpoint: Endpoint,
    game_state: GameState,
    tick_rate: u64, // 60 Hz
}

impl GameServer {
    pub async fn run(&mut self) {
        let mut interval = tokio::time::interval(
            tokio::time::Duration::from_millis(16) // 60 Hz
        );
        
        loop {
            interval.tick().await;
            self.update_game_state();
            self.broadcast_state().await;
        }
    }
    
    fn update_game_state(&mut self) {
        // Server-authoritative game logic
        // - Validate client inputs
        // - Apply physics
        // - Detect hits (server-side rewind)
        // - Update scores
    }
    
    async fn broadcast_state(&self) {
        // Send game state to all clients
        let serialized = bincode::serialize(&self.game_state).unwrap();
        // ... send via QUIC
    }
}

// FFI exports for C++
#[no_mangle]
pub extern "C" fn server_create() -> *mut GameServer {
    Box::into_raw(Box::new(GameServer::new()))
}

#[no_mangle]
pub extern "C" fn server_start(server: *mut GameServer) {
    // Start async runtime
}
```

**C++ Client** (`plugins/NetworkPlugin/src/NetworkPlugin.h`):

```cpp
#pragma once
#include "SecretEngine/IPlugin.h"

namespace SecretEngine::Network {

class NetworkPlugin : public IPlugin {
public:
    const char* GetName() const override { return "Network"; }
    
    void OnLoad(ICore* core) override;
    void OnUpdate(float deltaTime) override;
    
    // Connection API
    bool Connect(const char* serverIP, uint16_t port);
    void Disconnect();
    bool IsConnected() const;
    
    // Send/Receive
    void SendPlayerInput(const PlayerInputPacket& input);
    bool ReceiveGameState(GameStatePacket& outState);
    
private:
    ICore* m_core = nullptr;
    void* m_rustClient = nullptr; // Rust FFI handle
    
    // Client-side prediction
    struct PredictionState {
        uint32_t lastAckedTick;
        std::vector<PlayerInputPacket> pendingInputs;
    };
    PredictionState m_prediction;
    
    void Reconcile(const GameStatePacket& serverState);
};

} // namespace SecretEngine::Network
```

**Network Features**:
- Client-side prediction
- Server-side rewind (lag compensation)
- Input buffering
- State reconciliation
- Interpolation for remote players

**Protocol**: QUIC (via Rust's `quinn` library)
- Lower latency than TCP
- Built-in encryption
- Multiplexed streams
- Packet loss recovery

---

## 🔧 Build System Integration

### Root CMakeLists.txt

```cmake
# Add new plugins
add_subdirectory(plugins/PhysicsPlugin)
add_subdirectory(plugins/FPSGameLogic)
add_subdirectory(plugins/AIPlugin)
add_subdirectory(plugins/WeaponPlugin)
add_subdirectory(plugins/AudioPlugin)
add_subdirectory(plugins/AnimationPlugin)
add_subdirectory(plugins/NetworkPlugin)

# Build Rust network library
add_custom_target(RustNetwork ALL
    COMMAND cargo build --release
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/network_rs
)
```

### Plugin Load Order

```cpp
// core/src/Core.cpp
void Core::Initialize() {
    // Phase 1: Core systems
    m_plugins["Camera"] = CreateCameraPlugin();
    m_plugins["Camera"]->OnLoad(this);
    
    m_plugins["Input"] = CreateAndroidInputPlugin();
    m_plugins["Input"]->OnLoad(this);
    
    // Phase 2: Game systems
    m_plugins["Physics"] = CreatePhysicsPlugin();
    m_plugins["Physics"]->OnLoad(this);
    
    m_plugins["FPSGame"] = CreateFPSGamePlugin();
    m_plugins["FPSGame"]->OnLoad(this);
    
    m_plugins["AI"] = CreateAIPlugin();
    m_plugins["AI"]->OnLoad(this);
    
    m_plugins["Weapons"] = CreateWeaponPlugin();
    m_plugins["Weapons"]->OnLoad(this);
    
    // Phase 3: Presentation
    m_plugins["Audio"] = CreateAudioPlugin();
    m_plugins["Audio"]->OnLoad(this);
    
    m_plugins["Animation"] = CreateAnimationPlugin();
    m_plugins["Animation"]->OnLoad(this);
    
    m_plugins["Renderer"] = CreateVulkanRenderer();
    m_plugins["Renderer"]->OnLoad(this);
    
    // Phase 4: Network (optional)
    if (m_enableMultiplayer) {
        m_plugins["Network"] = CreateNetworkPlugin();
        m_plugins["Network"]->OnLoad(this);
    }
}
```

---

## 📊 Performance Budgets

### Frame Budget (16.6ms @ 60 FPS)

| System | Budget | Notes |
|--------|--------|-------|
| **Input** | 0.1ms | Touch processing |
| **Physics** | 0.5ms | Collision, raycasting |
| **Game Logic** | 2.0ms | Movement, weapons, combat |
| **AI** | 1.0ms | 10 bots |
| **Animation** | 1.5ms | Skeletal animation |
| **Audio** | 0.5ms | 3D audio mixing |
| **Rendering** | 10.0ms | 6M triangles |
| **Network** | 0.5ms | Send/receive packets |
| **Misc** | 0.5ms | Debug, profiling |
| **Total** | **16.6ms** | **60 FPS** |

### Memory Budget (200MB Total)

| System | Budget | Notes |
|--------|--------|-------|
| **Renderer** | 100MB | Textures, meshes, buffers |
| **Audio** | 30MB | Sound effects, music |
| **Game Logic** | 20MB | Entities, components |
| **Physics** | 10MB | Collision data |
| **AI** | 10MB | NavMesh, behavior trees |
| **Network** | 10MB | Packet buffers |
| **Animation** | 10MB | Skeletal data |
| **Misc** | 10MB | Debug, profiling |
| **Total** | **200MB** | |

---

## 🎯 Implementation Roadmap

### Month 1: Core Gameplay
- Week 1: Physics Plugin + Movement System
- Week 2: Weapon System + Combat
- Week 3: AI Plugin + Bot Behaviors
- Week 4: Testing & Polish

### Month 2: Content & Features
- Week 5: Weapon Variety (3+ weapon types)
- Week 6: Map Design + Spawn Points
- Week 7: Audio Integration
- Week 8: Animation System

### Month 3: Networking
- Week 9: Rust Server Setup
- Week 10: Client-Side Prediction
- Week 11: Server-Side Rewind
- Week 12: Multiplayer Testing

### Month 4: Polish & Optimization
- Week 13: Performance Optimization
- Week 14: UI/UX Polish
- Week 15: Bug Fixes
- Week 16: Release Preparation

---

## ✅ Success Criteria

### Technical
- [ ] 60 FPS on Moto G34 (Adreno 619)
- [ ] < 200MB memory usage
- [ ] 10+ bots with AI
- [ ] 4+ weapon types
- [ ] Client-server networking
- [ ] < 50ms network latency

### Gameplay
- [ ] Smooth movement (WASD + jump + crouch)
- [ ] Responsive shooting
- [ ] Accurate hit detection
- [ ] Challenging AI opponents
- [ ] Team deathmatch mode
- [ ] Score tracking

### Quality
- [ ] No crashes
- [ ] No memory leaks
- [ ] Consistent frame times
- [ ] Good audio feedback
- [ ] Clear UI/HUD

---

## 📚 Code Reference Summary

### Claude's Contributions (Recommended Base)
- ✅ **FPSComponents.h** - 17 pure data components
- ✅ **FPSFastData.h** - 8-byte packet system
- ✅ **FPSSystems.h** - 10 stateless systems
- ✅ **FPSGamePlugin.h** - Main plugin orchestration
- ✅ **ARCHITECTURE_v2.md** - Complete architecture docs
- ✅ **INTEGRATION_GUIDE_v2.md** - Step-by-step setup

**Why Claude's Approach**:
- Fully aligned with SecretEngine principles
- Data-oriented design (ECS)
- Fast Data Architecture (sub-100ns)
- Mobile-first performance
- Zero allocation hot paths

### ChatGPT's Contributions (Supplementary)
- ✅ **PhysicsPlugin** - Collision & raycasting
- ✅ **WeaponPlugin** - Weapon mechanics
- ✅ **BotPlugin** - AI behaviors
- ✅ **GameModePlugin** - Match state
- ✅ Clear plugin separation

**Why ChatGPT's Approach**:
- Easy to understand
- Clear separation of concerns
- Good for traditional game devs
- Modular plugin structure

---

## 🚀 Next Steps

### Immediate Actions (This Week)
1. **Review** this plan with team
2. **Create** git branch: `feature/fps-game`
3. **Copy** Claude's files to `plugins/FPSGameLogic/`
4. **Implement** PhysicsPlugin (raycasting first)
5. **Test** basic movement on device

### Week 1 Goals
- [ ] PhysicsPlugin compiles
- [ ] FPSGameLogic compiles
- [ ] Player entity spawns
- [ ] Movement works (WASD)
- [ ] Camera follows player
- [ ] Basic raycast weapon fire

### Week 2 Goals
- [ ] Weapon system complete
- [ ] Damage/health system
- [ ] Bot entities spawn
- [ ] Basic AI (idle, patrol)
- [ ] Team system (Red vs Blue)

---

## 📞 Support & Resources

### Documentation
- SecretEngine: `docs/architecture/`
- Claude's FPS: `docs/reference/game/claude suggesions/`
- ChatGPT's FPS: `docs/reference/game/chatgpt suggesions/`

### External Resources
- Rust Networking: https://github.com/quinn-rs/quinn
- ECS Patterns: https://github.com/SanderMertens/ecs-faq
- COD Tech: GDC talks on Call of Duty engine

---

**Status**: Ready for Implementation ✅  
**Estimated Time**: 3-4 months  
**Risk Level**: MEDIUM (networking is complex)  
**Impact**: HIGH (complete FPS game)

