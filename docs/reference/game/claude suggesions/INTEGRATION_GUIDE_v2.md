# FPS Game Integration Guide

**Version**: 2.0  
**Target**: SecretEngine  
**Estimated Time**: 2-3 hours

---

## 📋 Prerequisites

Before starting, ensure you have:

- [x] SecretEngine core compiled
- [x] CameraPlugin functional
- [x] AndroidInput plugin functional
- [x] VulkanRenderer working
- [x] Understanding of Fast Data Architecture

---

## 🎯 Integration Steps

### Step 1: Add FPS Plugin to Build System

**File**: `plugins/CMakeLists.txt`

```cmake
# Add FPS Game plugin
add_subdirectory(FPSGameLogic)
```

**File**: `plugins/FPSGameLogic/CMakeLists.txt`

```cmake
add_library(FPSGameLogic STATIC
    src/FPSComponents.h
    src/FPSFastData.h
    src/FPSSystems.h
    src/FPSGamePlugin_New.h
)

target_link_libraries(FPSGameLogic
    PUBLIC
        SecretEngine_Core
)

target_include_directories(FPSGameLogic
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(FPSGameLogic PROPERTIES
    FOLDER "Plugins"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
```

---

### Step 2: Copy Plugin Files

```bash
mkdir -p plugins/FPSGameLogic/src

# Copy files
cp FPSComponents.h plugins/FPSGameLogic/src/
cp FPSFastData.h plugins/FPSGameLogic/src/
cp FPSSystems.h plugins/FPSGameLogic/src/
cp FPSGamePlugin_New.h plugins/FPSGameLogic/src/FPSGamePlugin.h
cp CMakeLists_FPS.txt plugins/FPSGameLogic/CMakeLists.txt
```

---

### Step 3: Register Plugin in Core

**File**: `core/src/Core.cpp`

```cpp
// Add extern declaration
extern "C" IPlugin* CreateCameraPlugin();
extern "C" IPlugin* CreateAndroidInputPlugin();
extern "C" IPlugin* CreateVulkanRenderer();
extern "C" IPlugin* CreateFPSGamePlugin();  // ADD THIS

void Core::Initialize() {
    // ... existing plugin loading ...
    
    // Load FPS game logic plugin
    m_plugins["FPSGame"] = CreateFPSGamePlugin();
    m_plugins["FPSGame"]->OnLoad(this);
    m_plugins["FPSGame"]->OnActivate();
}
```

**File**: `plugins/FPSGameLogic/src/FPSGamePlugin.cpp` (create this)

```cpp
#include "FPSGamePlugin.h"

extern "C" {
    PLUGIN_API SecretEngine::IPlugin* CreateFPSGamePlugin() {
        return new SecretEngine::FPS::FPSGamePlugin();
    }
    
    PLUGIN_API void DestroyFPSGamePlugin(SecretEngine::IPlugin* plugin) {
        delete plugin;
    }
}
```

---

### Step 4: Integrate with AndroidInput

**File**: `plugins/AndroidInput/src/InputPlugin.h`

```cpp
#include "FPSGamePlugin.h"  // Include FPS plugin

void InputPlugin::ProcessTouch(const TouchEvent& touch) {
    // Get FPS game plugin
    auto* fps = m_core->GetCapability<FPS::FPSGamePlugin>("fps_game");
    if (!fps) return;
    
    uint32_t playerId = fps->GetLocalPlayerEntity();
    if (playerId == 0) return;
    
    // Left side = movement
    if (touch.x < m_screenWidth / 2) {
        FPS::Fast::PlayerMovePacket packet;
        packet.entityId = playerId;
        packet.sequence = m_inputSequence++;
        packet.forward = static_cast<int8_t>(touch.deltaY * 127.0f);
        packet.strafe = static_cast<int8_t>(-touch.deltaX * 127.0f);
        packet.flags = 0;
        
        if (m_isJumping) packet.flags |= FPS::Fast::PlayerMovePacket::FLAG_JUMP;
        if (m_isCrouching) packet.flags |= FPS::Fast::PlayerMovePacket::FLAG_CROUCH;
        
        fps->GetFastStreams().moveInput.Push(packet);
    }
    
    // Right side = look (handled by CameraPlugin already)
    
    // Tap right side = fire
    if (touch.justPressed && touch.x >= m_screenWidth / 2) {
        FPS::Fast::PlayerActionPacket packet;
        packet.entityId = playerId;
        packet.sequence = m_inputSequence++;
        packet.action = FPS::Fast::PlayerActionPacket::ACTION_FIRE;
        packet.weaponSlot = 0;
        packet.padding = 0;
        
        fps->GetFastStreams().actionInput.Push(packet);
    }
}
```

---

### Step 5: Implement Core Systems

**File**: `plugins/FPSGameLogic/src/FPSSystems.cpp`

```cpp
#include "FPSSystems.h"
#include "SecretEngine/Core/IWorld.h"

namespace SecretEngine::FPS {

// ============================================================================
// MOVEMENT SYSTEM IMPLEMENTATION
// ============================================================================

void MovementSystem::Update(
    IWorld* world,
    float deltaTime,
    Fast::FastPacketQueue<Fast::PlayerMovePacket>& moveQueue,
    Fast::FastPacketQueue<Fast::PlayerLookPacket>& lookQueue
) {
    // Drain movement input queue
    Fast::PlayerMovePacket movePacket;
    while (moveQueue.Pop(movePacket)) {
        // Get entity components
        if (!world->HasComponent<KCCComponent>(movePacket.entityId)) continue;
        if (!world->HasComponent<TransformComponent>(movePacket.entityId)) continue;
        
        auto& kcc = world->GetComponent<KCCComponent>(movePacket.entityId);
        auto& transform = world->GetComponent<TransformComponent>(movePacket.entityId);
        
        // Process input
        ProcessMovementInput(movePacket.entityId, movePacket, kcc, deltaTime);
        
        // Apply velocity to position
        transform.position += kcc.velocity * deltaTime;
    }
    
    // Apply gravity to all KCC entities
    auto kccEntities = world->Query<KCCComponent, TransformComponent>();
    for (auto entityId : kccEntities) {
        auto& kcc = world->GetComponent<KCCComponent>(entityId);
        ApplyGravity(kcc, deltaTime);
    }
}

void MovementSystem::ProcessMovementInput(
    uint32_t entityId,
    const Fast::PlayerMovePacket& packet,
    KCCComponent& kcc,
    float deltaTime
) {
    // Convert input to velocity
    float forwardInput = packet.forward / 127.0f;
    float strafeInput = packet.strafe / 127.0f;
    
    // Apply movement speed
    float speed = kcc.moveSpeed;
    if (packet.flags & Fast::PlayerMovePacket::FLAG_SPRINT) {
        speed *= kcc.sprintMultiplier;
    }
    if (packet.flags & Fast::PlayerMovePacket::FLAG_CROUCH) {
        speed *= kcc.crouchMultiplier;
    }
    
    // Set velocity (camera forward/right will be applied by transform)
    kcc.velocity.z = forwardInput * speed;
    kcc.velocity.x = strafeInput * speed;
    
    // Handle jump
    if (packet.flags & Fast::PlayerMovePacket::FLAG_JUMP && kcc.isGrounded) {
        kcc.velocity.y = 5.0f;  // Jump velocity
        kcc.isGrounded = false;
    }
}

void MovementSystem::ApplyGravity(KCCComponent& kcc, float deltaTime) {
    if (!kcc.isGrounded) {
        kcc.velocity.y -= 9.8f * deltaTime;  // Gravity
    }
    
    // Ground check (simple Y=0 plane for now)
    if (kcc.velocity.y < 0 && /* position.y <= 0 */) {
        kcc.velocity.y = 0;
        kcc.isGrounded = true;
    }
}

// ============================================================================
// WEAPON SYSTEM IMPLEMENTATION
// ============================================================================

void WeaponSystem::Update(
    IWorld* world,
    float currentTime,
    Fast::FastPacketQueue<Fast::PlayerActionPacket>& actionQueue,
    Fast::FastPacketQueue<Fast::WeaponFirePacket>& fireEvents
) {
    // Process action inputs
    Fast::PlayerActionPacket actionPacket;
    while (actionQueue.Pop(actionPacket)) {
        if (!world->HasComponent<WeaponComponent>(actionPacket.entityId)) continue;
        
        auto& weapon = world->GetComponent<WeaponComponent>(actionPacket.entityId);
        
        switch (actionPacket.action) {
            case Fast::PlayerActionPacket::ACTION_FIRE:
                if (CanFire(weapon, currentTime)) {
                    // Fire weapon (raycast in game logic)
                    weapon.lastFireTime = currentTime;
                    weapon.ammoInMag--;
                    
                    // Push fire event
                    Fast::WeaponFirePacket firePacket;
                    firePacket.entityId = actionPacket.entityId;
                    firePacket.weaponType = static_cast<uint8_t>(weapon.type);
                    fireEvents.Push(firePacket);
                }
                break;
                
            case Fast::PlayerActionPacket::ACTION_RELOAD:
                if (!weapon.isReloading && weapon.ammoReserve > 0) {
                    weapon.isReloading = true;
                    weapon.reloadStartTime = currentTime;
                }
                break;
        }
    }
    
    // Update reloads
    auto weaponEntities = world->Query<WeaponComponent>();
    for (auto entityId : weaponEntities) {
        auto& weapon = world->GetComponent<WeaponComponent>(entityId);
        ProcessReload(weapon, currentTime);
    }
}

void WeaponSystem::ProcessReload(WeaponComponent& weapon, float currentTime) {
    if (!weapon.isReloading) return;
    
    float elapsed = currentTime - weapon.reloadStartTime;
    if (elapsed >= weapon.reloadTime) {
        // Reload complete
        uint32_t needed = weapon.magSize - weapon.ammoInMag;
        uint32_t toReload = std::min(needed, weapon.ammoReserve);
        
        weapon.ammoInMag += toReload;
        weapon.ammoReserve -= toReload;
        weapon.isReloading = false;
        weapon.reloadStartTime = -1.0f;
    }
}

bool WeaponSystem::CanFire(const WeaponComponent& weapon, float currentTime) {
    if (weapon.isReloading) return false;
    if (weapon.ammoInMag == 0) return false;
    
    float timeSinceLastFire = currentTime - weapon.lastFireTime;
    return timeSinceLastFire >= weapon.fireRate;
}

} // namespace SecretEngine::FPS
```

---

### Step 6: Build and Test

```bash
# Build
mkdir build && cd build
cmake ..
make

# Deploy to Android
adb install -r build/android/SecretEngine.apk

# Check logs
adb logcat -s FPSGameLogic
```

**Expected Output:**
```
FPSGameLogic: Plugin loaded
FPSGameLogic: Plugin activated
FPSGameLogic: Player entity created
FPSGameLogic: Created 3 bots for team Red
FPSGameLogic: Created 4 bots for team Blue
FPSGameLogic: Match started
```

---

### Step 7: Verify Fast Data Flow

**Test Input → Game Logic:**

1. Touch left side of screen
2. Check logcat for movement packets
3. Verify player entity position updates

**Test Weapon Fire:**

1. Tap right side of screen
2. Check for fire event packets
3. Verify ammo decrements

---

## 🔍 Troubleshooting

### Issue: Plugin Not Loading

**Solution**: Check Core.cpp has CreateFPSGamePlugin() declared

### Issue: No Movement

**Solution**: Verify AndroidInput is pushing packets to moveInput queue

### Issue: Entities Not Spawning

**Solution**: Check SpawnSystem initialization in OnActivate()

---

## ✅ Integration Checklist

- [ ] Files copied to plugins/FPSGameLogic/
- [ ] CMakeLists.txt updated
- [ ] Plugin registered in Core.cpp
- [ ] AndroidInput integration added
- [ ] Systems implemented
- [ ] Build successful
- [ ] Plugin loads on device
- [ ] Entities spawn
- [ ] Input works
- [ ] Fast Data packets flowing

---

## 🚀 Next Steps

After integration:

1. Implement remaining systems (AI, Combat)
2. Add visual effects integration
3. Tune gameplay parameters
4. Add HUD rendering
5. Implement networking

---

**Setup Time**: 2-3 hours  
**Difficulty**: Medium  
**Prerequisites**: Understanding of ECS and Fast Data Architecture
