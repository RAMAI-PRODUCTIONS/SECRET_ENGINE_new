# Gameplay Tag System Integration Example

## Current Status
✅ GameplayTagSystem plugin compiled successfully
✅ Integrated into build system
✅ Loaded by Core on startup
✅ Data table ready at `data/GameDataTable.json`

## How to Use in FPSGamePlugin

### Before (Hardcoded):
```cpp
void FPSGamePlugin::CreatePlayerEntity() {
    m_localPlayerEntity = m_world->CreateEntity();
    
    auto* transform = new TransformComponent();
    transform->position[0] = 0;
    transform->position[1] = 1;
    transform->position[2] = 0;
    m_world->AddComponent(m_localPlayerEntity, TransformComponent::TypeID, transform);
    
    auto* team = new TeamComponent();
    team->team = TeamComponent::Team::Blue;
    m_world->AddComponent(m_localPlayerEntity, TeamComponent::TypeID, team);
    
    auto* health = new HealthComponent();
    health->current = 100.0f;
    health->maximum = 100.0f;
    m_world->AddComponent(m_localPlayerEntity, HealthComponent::TypeID, health);
    
    // ... more hardcoded components
}
```

### After (Tag-Based):
```cpp
void FPSGamePlugin::CreatePlayerEntity() {
    // Get GameplayTagSystem plugin
    auto* tagPlugin = static_cast<SecretEngine::GAS::GameplayTagPlugin*>(
        m_core->GetCapability("gameplay_tags")
    );
    
    if (!tagPlugin) {
        m_logger->LogError("FPSGameLogic", "GameplayTagSystem not found!");
        return;
    }
    
    auto* dataTable = tagPlugin->GetDataTable();
    
    // Create player from data table - ONE LINE!
    m_localPlayerEntity = dataTable->CreateCharacterFromTable("player_soldier");
    
    m_logger->LogInfo("FPSGameLogic", "player entity created from data table");
}

void FPSGamePlugin::CreateBotEntities(int count) {
    auto* tagPlugin = static_cast<SecretEngine::GAS::GameplayTagPlugin*>(
        m_core->GetCapability("gameplay_tags")
    );
    
    if (!tagPlugin) return;
    
    auto* dataTable = tagPlugin->GetDataTable();
    
    for (int i = 0; i < count; ++i) {
        // Create bot from data table - ONE LINE!
        Entity botId = dataTable->CreateCharacterFromTable("enemy_soldier");
        
        // Optionally randomize position
        auto* transform = static_cast<TransformComponent*>(
            m_world->GetComponent(botId, TransformComponent::TypeID)
        );
        if (transform) {
            transform->position[0] = (rand() % 20) - 10.0f;
            transform->position[2] = (rand() % 20) - 10.0f;
        }
    }
    
    m_logger->LogInfo("FPSGameLogic", "bots created from data table");
}
```

## Benefits

### Before:
- 50+ lines of hardcoded component creation
- Need to recompile to change stats
- Difficult to add new character types
- No designer-friendly workflow

### After:
- 2 lines of code per character
- Change stats in JSON, no recompile
- Add new characters by editing JSON
- Designers can balance without programming

## Creating New Characters

### Want a fast scout?
Edit `data/GameDataTable.json`:
```json
{
  "id": "player_scout",
  "tags": [
    "Character.Class.Scout",
    "Ability.Movement.Sprint",
    "Ability.Movement.DoubleJump",
    "Ability.Combat.Shoot"
  ],
  "team": "player"
}
```

Then in code:
```cpp
Entity scout = dataTable->CreateCharacterFromTable("player_scout");
```

Done! Scout has:
- 75 health (from Scout class tag)
- 7.0 speed (from Scout class tag)
- Sprint ability
- Double jump ability
- Shoot ability
- All physics, rendering, everything

### Want a heavy tank?
```json
{
  "id": "player_heavy",
  "tags": [
    "Character.Class.Heavy",
    "Ability.Defense.Shield",
    "Ability.Combat.Shoot"
  ],
  "team": "player"
}
```

```cpp
Entity heavy = dataTable->CreateCharacterFromTable("player_heavy");
```

Heavy has:
- 150 health
- 3.5 speed
- Shield ability (50% damage reduction)
- Shoot ability

## Next Steps

1. **Update FPSGamePlugin.cpp** to use tag-based character creation
2. **Test** that characters spawn correctly
3. **Add more character types** to GameDataTable.json
4. **Implement ability activation** in FPS systems
5. **Add gameplay effects** (buffs, debuffs)

## Full Documentation

See:
- `docs/GAMEPLAY_TAG_SYSTEM.md` - Complete system guide
- `docs/CREATE_ANY_GAME.md` - Examples for different game types
- `data/GameDataTable.json` - Data table with all definitions
