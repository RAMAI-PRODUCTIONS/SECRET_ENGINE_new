# EXECUTE FPS GAME - Action Plan

**Goal**: Single-player FPS with physics, movement, joystick input, shoot button, UI, and bots  
**Timeline**: 3 days (minimal viable game)  
**Status**: READY TO EXECUTE NOW

---

## 🎯 Scope (Minimal Viable FPS)

### What We're Building
- Player movement (WASD/joystick)
- Jump with ground detection
- Weapon shooting (hitscan raycast)
- 5 AI bots (basic patrol/attack)
- Simple UI (health, ammo, crosshair)
- Physics (raycasting, collision)
- Single map (flat ground + obstacles)

### What We're NOT Building (Yet)
- ❌ Weapon pickup/drop (Phase 2)
- ❌ Multiple weapons (Phase 2)
- ❌ NavMesh (Phase 2)
- ❌ Animation (Phase 3)
- ❌ Audio (Phase 3)
- ❌ Networking (Phase 4)

---

## 📋 Plugin Architecture (Following PLUGIN_MANIFEST.md)

### New Plugins to Create

```
plugins/
├── PhysicsPlugin/          ← NEW (type: physics, singleton)
│   ├── src/
│   │   ├── PhysicsPlugin.h
│   │   ├── PhysicsPlugin.cpp
│   │   └── PhysicsTypes.h
│   ├── CMakeLists.txt
│   └── plugin_manifest.json
│
├── FPSGameLogic/           ← NEW (type: game, non-singleton)
│   ├── src/
│   │   ├── FPSGamePlugin.h
│   │   ├── FPSGamePlugin.cpp
│   │   ├── FPSComponents.h
│   │   ├── FPSFastData.h
│   │   └── FPSSystems.cpp
│   ├── CMakeLists.txt
│   └── plugin_manifest.json
│
└── FPSUIPlugin/            ← NEW (type: ui, singleton)
    ├── src/
    │   ├── FPSUIPlugin.h
    │   └── FPSUIPlugin.cpp
    ├── CMakeLists.txt
    └── plugin_manifest.json
```

### Existing Plugins (Use As-Is)
- ✅ VulkanRenderer (rendering)
- ✅ AndroidInput (input)
- ✅ CameraPlugin (camera)
- ✅ DebugPlugin (debug)

---

## 📅 3-Day Execution Plan

### DAY 1: Physics + Movement (8 hours)

#### Hour 1-2: PhysicsPlugin Setup
**Task**: Create plugin structure
```bash
mkdir -p plugins/PhysicsPlugin/src
```

**Files to Create**:
1. `plugins/PhysicsPlugin/plugin_manifest.json`
2. `plugins/PhysicsPlugin/src/PhysicsPlugin.h`
3. `plugins/PhysicsPlugin/src/PhysicsPlugin.cpp`
4. `plugins/PhysicsPlugin/src/PhysicsTypes.h`
5. `plugins/PhysicsPlugin/CMakeLists.txt`

**Acceptance Criteria**:
- [ ] Plugin loads without errors
- [ ] Raycast function exists
- [ ] Ground check function exists

#### Hour 3-4: FPSGameLogic Plugin Setup
**Task**: Create game logic plugin structure
```bash
mkdir -p plugins/FPSGameLogic/src
```

**Files to Create**:
1. `plugins/FPSGameLogic/plugin_manifest.json`
2. `plugins/FPSGameLogic/src/FPSComponents.h` (POD only)
3. `plugins/FPSGameLogic/src/FPSFastData.h` (8-byte packets)
4. `plugins/FPSGameLogic/src/FPSGamePlugin.h`
5. `plugins/FPSGameLogic/src/FPSGamePlugin.cpp`
6. `plugins/FPSGameLogic/CMakeLists.txt`

**Components to Implement** (POD only, no methods):
- `KCCComponent` (movement state)
- `HealthComponent` (HP)
- `WeaponComponent` (ammo, fire rate)
- `TeamComponent` (Red/Blue)

**Acceptance Criteria**:
- [ ] Plugin loads without errors
- [ ] Components registered with core
- [ ] Fast Data queues created

#### Hour 5-6: Movement System
**Task**: Implement MovementSystem with jump

**Files to Modify**:
- `plugins/FPSGameLogic/src/FPSSystems.cpp` (create)

**System to Implement**:
- `MovementSystem::Update()` - Process movement packets
- Ground detection via PhysicsPlugin
- Jump with coyote time
- Gravity application

**Acceptance Criteria**:
- [ ] Player moves with WASD
- [ ] Jump works
- [ ] Gravity works
- [ ] Ground detection works

#### Hour 7-8: Input Integration
**Task**: Connect AndroidInput to FPSGameLogic

**Files to Modify**:
- `plugins/AndroidInput/src/InputPlugin.cpp`

**Changes**:
- Push `PlayerMovePacket` to FPS game logic
- Push `PlayerActionPacket` for fire button

**Acceptance Criteria**:
- [ ] Joystick controls movement
- [ ] Jump button works
- [ ] Fire button sends packet

---

### DAY 2: Combat + AI (8 hours)

#### Hour 1-2: Weapon System
**Task**: Implement shooting with raycasting

**Files to Modify**:
- `plugins/FPSGameLogic/src/FPSSystems.cpp`

**System to Implement**:
- `WeaponSystem::Update()` - Process fire actions
- Raycast via PhysicsPlugin
- Damage application
- Ammo management

**Acceptance Criteria**:
- [ ] Fire button shoots
- [ ] Raycast hits enemies
- [ ] Damage applied
- [ ] Ammo decrements

#### Hour 3-4: Combat System
**Task**: Implement damage and health

**Files to Modify**:
- `plugins/FPSGameLogic/src/FPSSystems.cpp`

**System to Implement**:
- `CombatSystem::ProcessDamageEvents()` - Apply damage
- Kill detection
- Respawn logic

**Acceptance Criteria**:
- [ ] Enemies take damage
- [ ] Enemies die at 0 HP
- [ ] Player takes damage
- [ ] Player respawns

#### Hour 5-6: AI System (Basic)
**Task**: Implement bot behaviors

**Files to Modify**:
- `plugins/FPSGameLogic/src/FPSSystems.cpp`
- `plugins/FPSGameLogic/src/FPSComponents.h` (add AIComponent)

**System to Implement**:
- `AISystem::Update()` - Bot behaviors
- Idle behavior
- Patrol behavior (random walk)
- Attack behavior (shoot at player)

**Acceptance Criteria**:
- [ ] 5 bots spawn
- [ ] Bots patrol randomly
- [ ] Bots shoot at player
- [ ] Bots take damage

#### Hour 7-8: Match System
**Task**: Implement score and match state

**Files to Modify**:
- `plugins/FPSGameLogic/src/FPSSystems.cpp`
- `plugins/FPSGameLogic/src/FPSComponents.h` (add MatchState)

**System to Implement**:
- `MatchSystem::Update()` - Track kills/deaths
- Match timer
- Win condition (30 kills)

**Acceptance Criteria**:
- [ ] Kills tracked
- [ ] Deaths tracked
- [ ] Match ends at 30 kills

---

### DAY 3: UI + Polish (8 hours)

#### Hour 1-3: FPS UI Plugin
**Task**: Create UI overlay

**Files to Create**:
- `plugins/FPSUIPlugin/src/FPSUIPlugin.h`
- `plugins/FPSUIPlugin/src/FPSUIPlugin.cpp`
- `plugins/FPSUIPlugin/CMakeLists.txt`
- `plugins/FPSUIPlugin/plugin_manifest.json`

**UI Elements**:
- Crosshair (center of screen)
- Health bar (bottom left)
- Ammo counter (bottom right)
- Kill count (top right)

**Acceptance Criteria**:
- [ ] Crosshair visible
- [ ] Health updates
- [ ] Ammo updates
- [ ] Kills update

#### Hour 4-5: Visual Effects
**Task**: Add muzzle flash and hit markers

**Files to Modify**:
- `plugins/VulkanRenderer/src/RendererPlugin.cpp`

**Effects**:
- Muzzle flash (yellow sphere at gun position)
- Hit marker (red X on hit)
- Blood splatter (red particles on kill)

**Acceptance Criteria**:
- [ ] Muzzle flash on fire
- [ ] Hit marker on hit
- [ ] Blood on kill

#### Hour 6-7: Map Creation
**Task**: Create simple test map

**Files to Create**:
- `Assets/Maps/test_map.bin` (cooked binary)

**Map Contents**:
- Flat ground plane (100x100m)
- 10 box obstacles (cover)
- 5 spawn points (player + bots)

**Acceptance Criteria**:
- [ ] Map loads
- [ ] Collision works
- [ ] Spawns work

#### Hour 8: Testing & Bug Fixes
**Task**: End-to-end testing

**Test Cases**:
1. Player spawns
2. Player moves with joystick
3. Player jumps
4. Player shoots
5. Bots spawn
6. Bots attack player
7. Player kills bot
8. Player dies
9. Player respawns
10. Match ends

**Acceptance Criteria**:
- [ ] All test cases pass
- [ ] No crashes
- [ ] 60 FPS maintained

---

## 🔧 Implementation Details

### PhysicsPlugin (Minimal)


**File**: `plugins/PhysicsPlugin/src/PhysicsPlugin.h`

```cpp
// SecretEngine
// Module: PhysicsPlugin
// Responsibility: Raycasting and collision detection
// Dependencies: Core

#pragma once
#include "SecretEngine/IPlugin.h"
#include "SecretEngine/ICore.h"
#include <glm/glm.hpp>
#include <vector>

namespace SecretEngine::Physics {

struct RaycastHit {
    bool hit = false;
    uint32_t entityId = 0;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    float distance = 0.0f;
};

class PhysicsPlugin : public IPlugin {
public:
    // IPlugin interface
    const char* GetName() const override { return "Physics"; }
    uint32_t GetVersion() const override { return 1; }
    
    void OnLoad(ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void* GetInterface(uint32_t id) override { return nullptr; }
    
    // Physics API
    RaycastHit Raycast(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance = 1000.0f
    );
    
    bool CheckGround(
        const glm::vec3& position,
        float radius,
        float checkDistance = 0.2f
    );
    
private:
    ICore* m_core = nullptr;
    IWorld* m_world = nullptr;
    ILogger* m_logger = nullptr;
};

} // namespace SecretEngine::Physics

// Plugin factory
extern "C" {
    PLUGIN_API SecretEngine::IPlugin* CreatePhysicsPlugin();
    PLUGIN_API void DestroyPhysicsPlugin(SecretEngine::IPlugin* plugin);
}
```

**File**: `plugins/PhysicsPlugin/plugin_manifest.json`

```json
{
  "name": "PhysicsPlugin",
  "version": "1.0.0",
  "type": "physics",
  "library": "PhysicsPlugin.dll",
  "dependencies": [],
  "capabilities": ["physics", "raycasting"],
  "requirements": {
    "min_engine_version": "0.1.0"
  }
}
```

---

### FPSGameLogic Plugin (Minimal)

**File**: `plugins/FPSGameLogic/src/FPSComponents.h`

```cpp
// SecretEngine
// Module: FPSGameLogic
// Responsibility: FPS game component definitions (POD only)
// Dependencies: Core

#pragma once
#include <glm/glm.hpp>
#include <cstdint>

namespace SecretEngine::FPS {

// Team assignment
struct TeamComponent {
    enum class Team : uint8_t { None = 0, Red = 1, Blue = 2 };
    Team team = Team::None;
};

// Health
struct HealthComponent {
    float current = 100.0f;
    float maximum = 100.0f;
};

// Movement
struct KCCComponent {
    glm::vec3 velocity{0.0f};
    uint8_t isGrounded : 1;
    uint8_t isJumping : 1;
    uint8_t _padding : 6;
    float moveSpeed = 5.0f;
    float jumpForce = 7.0f;
    float gravity = 9.8f;
};

// Weapon
struct WeaponComponent {
    uint32_t ammoInMag = 30;
    uint32_t ammoReserve = 90;
    float fireRate = 0.1f;
    float lastFireTime = 0.0f;
    float damage = 25.0f;
    float range = 100.0f;
};

// AI
struct AIComponent {
    enum class Behavior : uint8_t { Idle, Patrol, Attack };
    Behavior currentBehavior = Behavior::Patrol;
    uint32_t targetEntity = 0;
    glm::vec3 patrolTarget{0.0f};
    float behaviorTimer = 0.0f;
};

// Match state (global)
struct MatchState {
    uint32_t playerKills = 0;
    uint32_t playerDeaths = 0;
    uint32_t botKills = 0;
    float matchTime = 0.0f;
    uint32_t killLimit = 30;
};

} // namespace SecretEngine::FPS
```

**File**: `plugins/FPSGameLogic/src/FPSFastData.h`

```cpp
// SecretEngine
// Module: FPSGameLogic
// Responsibility: 8-byte packet definitions
// Dependencies: Core Fast Data

#pragma once
#include <cstdint>
#include <atomic>

namespace SecretEngine::FPS::Fast {

// Movement packet (8 bytes)
struct PlayerMovePacket {
    uint32_t entityId : 24;
    uint32_t sequence : 8;
    int8_t forward;
    int8_t strafe;
    uint8_t flags;
    uint8_t padding;
    
    static constexpr uint8_t FLAG_JUMP = 0x01;
};
static_assert(sizeof(PlayerMovePacket) == 8);

// Action packet (8 bytes)
struct PlayerActionPacket {
    uint32_t entityId : 24;
    uint32_t sequence : 8;
    uint8_t action;
    uint8_t padding[3];
    
    static constexpr uint8_t ACTION_FIRE = 0;
};
static_assert(sizeof(PlayerActionPacket) == 8);

// Damage packet (8 bytes)
struct DamageEventPacket {
    uint32_t victimId : 24;
    uint32_t amount : 8;
    uint32_t attackerId : 24;
    uint32_t padding : 8;
};
static_assert(sizeof(DamageEventPacket) == 8);

// Lock-free queue (SPSC)
template<typename PacketT, uint32_t Capacity = 1024>
class FastPacketQueue {
public:
    FastPacketQueue() : m_head(0), m_tail(0) {}
    
    bool Push(const PacketT& packet) {
        uint32_t head = m_head.load(std::memory_order_relaxed);
        uint32_t nextHead = (head + 1) % Capacity;
        uint32_t tail = m_tail.load(std::memory_order_acquire);
        if (nextHead == tail) return false;
        m_packets[head] = packet;
        m_head.store(nextHead, std::memory_order_release);
        return true;
    }
    
    bool Pop(PacketT& packet) {
        uint32_t tail = m_tail.load(std::memory_order_relaxed);
        uint32_t head = m_head.load(std::memory_order_acquire);
        if (tail == head) return false;
        packet = m_packets[tail];
        m_tail.store((tail + 1) % Capacity, std::memory_order_release);
        return true;
    }
    
private:
    alignas(64) std::atomic<uint32_t> m_head;
    alignas(64) std::atomic<uint32_t> m_tail;
    PacketT m_packets[Capacity];
};

// Fast data streams
struct FPSFastStreams {
    FastPacketQueue<PlayerMovePacket> moveInput;
    FastPacketQueue<PlayerActionPacket> actionInput;
    FastPacketQueue<DamageEventPacket> damageEvents;
};

} // namespace SecretEngine::FPS::Fast
```

**File**: `plugins/FPSGameLogic/plugin_manifest.json`

```json
{
  "name": "FPSGameLogic",
  "version": "1.0.0",
  "type": "game",
  "library": "FPSGameLogic.dll",
  "dependencies": ["physics"],
  "capabilities": ["fps_game"],
  "requirements": {
    "min_engine_version": "0.1.0"
  }
}
```

---

### FPSUIPlugin (Minimal)

**File**: `plugins/FPSUIPlugin/src/FPSUIPlugin.h`

```cpp
// SecretEngine
// Module: FPSUIPlugin
// Responsibility: FPS game UI overlay
// Dependencies: Core, FPSGameLogic

#pragma once
#include "SecretEngine/IPlugin.h"
#include "SecretEngine/ICore.h"

namespace SecretEngine::FPSUI {

class FPSUIPlugin : public IPlugin {
public:
    const char* GetName() const override { return "FPSUI"; }
    uint32_t GetVersion() const override { return 1; }
    
    void OnLoad(ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void* GetInterface(uint32_t id) override { return nullptr; }
    
private:
    ICore* m_core = nullptr;
    ILogger* m_logger = nullptr;
    
    void RenderCrosshair();
    void RenderHealthBar();
    void RenderAmmoCounter();
    void RenderKillCount();
};

} // namespace SecretEngine::FPSUI

extern "C" {
    PLUGIN_API SecretEngine::IPlugin* CreateFPSUIPlugin();
}
```

**File**: `plugins/FPSUIPlugin/plugin_manifest.json`

```json
{
  "name": "FPSUIPlugin",
  "version": "1.0.0",
  "type": "ui",
  "library": "FPSUIPlugin.dll",
  "dependencies": ["fps_game", "rendering"],
  "capabilities": ["fps_ui"],
  "requirements": {
    "min_engine_version": "0.1.0"
  }
}
```

---

## 🔗 Plugin Integration

### Core Integration

**File**: `core/src/Core.cpp` (modify)

```cpp
// Add plugin factory declarations
extern "C" IPlugin* CreatePhysicsPlugin();
extern "C" IPlugin* CreateFPSGamePlugin();
extern "C" IPlugin* CreateFPSUIPlugin();

void Core::Initialize() {
    // Existing plugins
    m_plugins["Camera"] = CreateCameraPlugin();
    m_plugins["Camera"]->OnLoad(this);
    
    m_plugins["Input"] = CreateAndroidInputPlugin();
    m_plugins["Input"]->OnLoad(this);
    
    // NEW: Physics plugin
    m_plugins["Physics"] = CreatePhysicsPlugin();
    m_plugins["Physics"]->OnLoad(this);
    m_plugins["Physics"]->OnActivate();
    
    // NEW: FPS game logic
    m_plugins["FPSGame"] = CreateFPSGamePlugin();
    m_plugins["FPSGame"]->OnLoad(this);
    m_plugins["FPSGame"]->OnActivate();
    
    // Existing renderer
    m_plugins["Renderer"] = CreateVulkanRenderer();
    m_plugins["Renderer"]->OnLoad(this);
    
    // NEW: FPS UI
    m_plugins["FPSUI"] = CreateFPSUIPlugin();
    m_plugins["FPSUI"]->OnLoad(this);
    m_plugins["FPSUI"]->OnActivate();
}
```

### Build System Integration

**File**: `plugins/CMakeLists.txt` (modify)

```cmake
# Existing plugins
add_subdirectory(VulkanRenderer)
add_subdirectory(AndroidInput)
add_subdirectory(CameraPlugin)
add_subdirectory(DebugPlugin)

# NEW: FPS plugins
add_subdirectory(PhysicsPlugin)
add_subdirectory(FPSGameLogic)
add_subdirectory(FPSUIPlugin)
```

**File**: `CMakeLists.txt` (modify Android section)

```cmake
if(ANDROID)
    add_library(SecretEngine SHARED 
        core/src/platform/android/AndroidMain.cpp
        "${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c"
    )
    
    target_link_libraries(SecretEngine 
        PRIVATE 
        SecretEngine_Core 
        VulkanRenderer 
        AndroidInput   
        CameraPlugin   
        DebugPlugin
        PhysicsPlugin      # NEW
        FPSGameLogic       # NEW
        FPSUIPlugin        # NEW
        android 
        log
        vulkan 
    )
endif()
```

---

## ✅ Acceptance Criteria (End of Day 3)

### Functional Requirements
- [ ] Player spawns in map
- [ ] Player moves with joystick (WASD)
- [ ] Player jumps with button
- [ ] Player shoots with fire button
- [ ] 5 AI bots spawn
- [ ] Bots patrol randomly
- [ ] Bots shoot at player
- [ ] Damage system works
- [ ] Health decreases on hit
- [ ] Player/bots die at 0 HP
- [ ] Respawn works
- [ ] UI shows health, ammo, kills
- [ ] Match ends at 30 kills

### Performance Requirements
- [ ] 60 FPS on Moto G34
- [ ] < 2ms game logic time
- [ ] < 200MB memory usage
- [ ] No frame drops
- [ ] No crashes

### Code Quality Requirements
- [ ] All plugins follow PLUGIN_MANIFEST.md
- [ ] All code follows LLM_CODING_RULES.md
- [ ] All code follows DESIGN_PRINCIPLES.md
- [ ] No direct plugin-to-plugin calls
- [ ] All communication via Fast Data or Core
- [ ] No heap allocations in hot paths
- [ ] All components are POD
- [ ] All systems are stateless

---

## 🚀 Execution Commands

### Day 1: Setup & Movement

```bash
# Create plugin directories
mkdir -p plugins/PhysicsPlugin/src
mkdir -p plugins/FPSGameLogic/src
mkdir -p plugins/FPSUIPlugin/src

# Create CMakeLists files
touch plugins/PhysicsPlugin/CMakeLists.txt
touch plugins/FPSGameLogic/CMakeLists.txt
touch plugins/FPSUIPlugin/CMakeLists.txt

# Create manifest files
touch plugins/PhysicsPlugin/plugin_manifest.json
touch plugins/FPSGameLogic/plugin_manifest.json
touch plugins/FPSUIPlugin/plugin_manifest.json

# Build
cmake -B build
cmake --build build

# Deploy to Android
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk

# Check logs
adb logcat -s SecretEngine PhysicsPlugin FPSGameLogic
```

### Day 2: Combat & AI

```bash
# Rebuild after adding systems
cmake --build build

# Deploy
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk

# Test
adb logcat -s FPSGameLogic | grep -E "Bot|Damage|Kill"
```

### Day 3: UI & Polish

```bash
# Final build
cmake --build build --config Release

# Deploy release
cd android
./gradlew assembleRelease
adb install -r app/build/outputs/apk/release/app-release.apk

# Performance test
adb shell dumpsys gfxinfo com.secretengine | grep -A 5 "Frame"
```

---

## 📊 Progress Tracking

### Day 1 Checklist
- [ ] PhysicsPlugin compiles
- [ ] FPSGameLogic compiles
- [ ] Plugins load without errors
- [ ] Player entity spawns
- [ ] Movement works
- [ ] Jump works
- [ ] Input integration works

### Day 2 Checklist
- [ ] Weapon system works
- [ ] Raycasting works
- [ ] Damage system works
- [ ] AI bots spawn
- [ ] Bots patrol
- [ ] Bots attack
- [ ] Match system works

### Day 3 Checklist
- [ ] UI plugin compiles
- [ ] Crosshair renders
- [ ] Health bar renders
- [ ] Ammo counter renders
- [ ] Visual effects work
- [ ] Map loads
- [ ] All tests pass

---

## 🐛 Troubleshooting

### Plugin Not Loading
```bash
# Check logs
adb logcat -s SecretEngine | grep "Plugin"

# Verify manifest
cat plugins/PhysicsPlugin/plugin_manifest.json

# Check dependencies
ldd build/lib/libPhysicsPlugin.so
```

### Movement Not Working
```bash
# Check input packets
adb logcat -s AndroidInput | grep "Move"

# Check FPS game logic
adb logcat -s FPSGameLogic | grep "Movement"

# Verify Fast Data queue
# Add debug logs in MovementSystem::Update()
```

### Raycasting Not Working
```bash
# Check physics plugin
adb logcat -s PhysicsPlugin | grep "Raycast"

# Verify entity positions
# Add debug logs in WeaponSystem::Update()
```

---

## 📝 Notes

### Following SecretEngine Rules
- ✅ All plugins are replaceable
- ✅ No direct plugin-to-plugin calls
- ✅ All communication via Core or Fast Data
- ✅ Components are POD only
- ✅ Systems are stateless
- ✅ No heap allocations in hot paths
- ✅ Mobile-first design
- ✅ Performance budgets respected

### Deviations from Full Plan
- Simplified AI (no NavMesh)
- Single weapon type
- No weapon pickup/drop
- No animation
- No audio
- Basic UI only

### Future Enhancements (Phase 2)
- Add NavMesh for better AI
- Add weapon pickup/drop
- Add multiple weapon types
- Add animation system
- Add audio system
- Add more maps

---

**Status**: READY TO EXECUTE  
**Start Date**: NOW  
**End Date**: 3 days from now  
**Expected Result**: Playable single-player FPS game

