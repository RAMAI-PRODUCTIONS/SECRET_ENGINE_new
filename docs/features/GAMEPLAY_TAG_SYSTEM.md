# Gameplay Tag System (GAS) - Complete Guide

## Overview
Unreal Engine-inspired Gameplay Tag System + Gameplay Ability System. **ONE JSON file** defines everything - no hardcoding!

## Philosophy

### ✅ Tag-Driven Design
- Tags define what entities CAN do
- Tags define what entities ARE
- Tags define what entities HAVE
- Everything is data, nothing is hardcoded

### ✅ Single Source of Truth
- **ONE** `GameDataTable.json` file
- All characters, abilities, weapons, effects
- Easy to balance, easy to modify
- No recompilation needed

## Core Concepts

### 1. Gameplay Tags
Hierarchical string-based identifiers:
```
Character.Type.Humanoid
Character.Class.Soldier
Ability.Movement.Sprint
Ability.Combat.Shoot
Status.Buff.SpeedBoost
Weapon.Type.Rifle
```

### 2. Tag Hierarchy
Tags inherit from parents:
```
"Character.Class.Soldier" matches "Character"
"Character.Class.Soldier" matches "Character.Class"
"Character.Class.Soldier" matches "Character.Class.Soldier"
```

### 3. Tag Containers
Entities have multiple tag containers:
- **Owned Tags**: Permanent (class, type)
- **Active Tags**: Temporary (buffs, debuffs)
- **Blocked Tags**: Cannot be applied

## Data Table Structure

### Single JSON File: `data/GameDataTable.json`

```json
{
  "gameplayTags": {
    // Define all tags and their properties
  },
  "characterDefinitions": [
    // Define all characters using tags
  ],
  "abilityEffects": {
    // Define what abilities do
  },
  "tagRelationships": {
    // Define tag interactions
  }
}
```

## Creating Characters

### Example: Fast Scout
```json
{
  "id": "player_scout",
  "tags": [
    "Character.Class.Scout",
    "Ability.Movement.Sprint",
    "Ability.Movement.DoubleJump",
    "Ability.Combat.Shoot",
    "Weapon.Type.Rifle"
  ],
  "team": "player"
}
```

**That's it!** The tags define everything:
- Scout class → 75 health, 7.0 speed (from tag definition)
- Sprint ability → Speed boost when active
- DoubleJump ability → Can jump twice
- Shoot ability → Can fire weapon
- Rifle weapon → 25 damage, 100 range

### Example: Heavy Tank
```json
{
  "id": "player_heavy",
  "tags": [
    "Character.Class.Heavy",
    "Ability.Defense.Shield",
    "Ability.Combat.Shoot",
    "Weapon.Type.Shotgun"
  ],
  "team": "player"
}
```

Heavy class → 150 health, 3.5 speed
Shield ability → 50% damage reduction
Shotgun → 80 damage, close range

### Example: Enemy Bot
```json
{
  "id": "enemy_soldier",
  "tags": [
    "Character.Class.Soldier",
    "Ability.Combat.Shoot",
    "Weapon.Type.Rifle"
  ],
  "team": "enemy",
  "ai": {
    "enabled": true,
    "behavior": "aggressive"
  }
}
```

## Defining Tags

### Character Class Tag
```json
"Character.Class.Soldier": {
  "parent": "Character.Type.Humanoid",
  "stats": {
    "health": 100,
    "speed": 5.0,
    "jumpForce": 7.0
  }
}
```

### Ability Tag
```json
"Ability.Movement.Sprint": {
  "type": "toggle",
  "speedMultiplier": 1.6,
  "staminaCost": 10.0
}
```

### Weapon Tag
```json
"Weapon.Type.Rifle": {
  "damage": 25.0,
  "fireRate": 0.1,
  "range": 100.0,
  "ammo": 30,
  "reloadTime": 2.0
}
```

### Status Effect Tag
```json
"Status.Buff.SpeedBoost": {
  "duration": 10.0,
  "speedMultiplier": 1.5
}
```

## Usage in Code

### Load Data Table
```cpp
GameplayDataTable dataTable(core);
dataTable.LoadDataTable("data/GameDataTable.json");
```

### Create Character
```cpp
// Create from table
Entity player = dataTable.CreateCharacterFromTable("player_scout");

// That's it! Character has:
// - All stats from tags
// - All abilities from tags
// - All weapons from tags
// - Physics, health, AI, everything
```

### Check Tags
```cpp
auto* tags = static_cast<GameplayTagComponent*>(
    world->GetComponent(entity, GameplayTagComponent::TypeID)
);

if (tags->HasTag("Ability.Movement.Sprint")) {
    // Can sprint
}

if (tags->MatchesAny("Character.Class")) {
    // Is a character class
}
```

### Activate Ability
```cpp
auto* abilityComp = static_cast<AbilitySystemComponent*>(
    world->GetComponent(entity, AbilitySystemComponent::TypeID)
);

auto* sprint = abilityComp->FindAbilityByTag("Ability.Movement.Sprint");
if (sprint && abilityComp->CanActivateAbility(sprint, tags)) {
    // Activate sprint
    sprint->isActive = true;
    sprint->isOnCooldown = true;
    sprint->cooldownRemaining = sprint->ability.cooldown;
}
```

### Apply Effect
```cpp
auto* effectComp = static_cast<GameplayEffectComponent*>(
    world->GetComponent(entity, GameplayEffectComponent::TypeID)
);

GameplayEffect speedBoost;
speedBoost.type = GameplayEffect::Duration;
speedBoost.duration = 10.0f;
// ... configure effect

effectComp->AddEffect(speedBoost);
```

## Tag Relationships

### Grants
```json
"Character.Class.Soldier": {
  "grants": ["Character.Type.Humanoid"]
}
```
Soldier automatically gets Humanoid tag

### Blocks
```json
"Status.Debuff.Stun": {
  "blocks": ["Ability.Movement.Sprint", "Ability.Combat.Shoot"]
}
```
Stunned entities can't sprint or shoot

### Overrides
```json
"Status.Buff.SpeedBoost": {
  "overrides": ["Status.Debuff.Slow"]
}
```
Speed boost removes slow effect

## Creating New Game Types

### Battle Royale
```json
{
  "id": "br_player",
  "tags": [
    "Character.Class.Soldier",
    "Ability.Movement.Sprint",
    "Ability.Movement.Slide",
    "Ability.Combat.Shoot",
    "Ability.Combat.Grenade",
    "Ability.Defense.Heal"
  ]
}
```

### Racing Game
```json
{
  "id": "race_car",
  "tags": [
    "Character.Type.Vehicle",
    "Ability.Vehicle.Boost",
    "Ability.Vehicle.Drift"
  ],
  "stats": {
    "speed": 30.0,
    "acceleration": 20.0
  }
}
```

### RPG
```json
{
  "id": "mage",
  "tags": [
    "Character.Class.Mage",
    "Ability.Magic.Fireball",
    "Ability.Magic.Teleport",
    "Ability.Magic.Shield"
  ],
  "stats": {
    "health": 80,
    "mana": 200,
    "intelligence": 20
  }
}
```

### MOBA
```json
{
  "id": "hero_assassin",
  "tags": [
    "Character.Class.Assassin",
    "Ability.Combat.Backstab",
    "Ability.Movement.Stealth",
    "Ability.Movement.Blink",
    "Ability.Ultimate.Execute"
  ]
}
```

## Benefits

✅ **No Hardcoding** - Everything in JSON
✅ **One File** - Single source of truth
✅ **Easy Balancing** - Change numbers, reload
✅ **Designer Friendly** - No programming needed
✅ **Flexible** - Create any game type
✅ **Modular** - Mix and match tags
✅ **Extensible** - Add new tags anytime
✅ **Hierarchical** - Tags inherit properties
✅ **Powerful** - Tag relationships and effects

## Advanced Features

### Dynamic Tag Addition
```cpp
// Add tag at runtime
tags->AddActiveTag("Status.Buff.SpeedBoost");

// Remove tag
tags->RemoveActiveTag("Status.Buff.SpeedBoost");

// Block tag
tags->BlockTag("Ability.Movement.Sprint");
```

### Attribute Modifiers
```cpp
auto* attributes = static_cast<AttributeSetComponent*>(
    world->GetComponent(entity, AttributeSetComponent::TypeID)
);

// Modify attributes
attributes->speed.AddModifier(2.0f);  // +2 speed
attributes->damage.MultiplyModifier(1.5f);  // 1.5x damage
```

### Gameplay Effects
```cpp
GameplayEffect poison;
poison.type = GameplayEffect::Duration;
poison.duration = 10.0f;
poison.period = 1.0f;  // Tick every second

// Add damage modifier
poison.modifiers[0].operation = GameplayEffect::Add;
strcpy(poison.modifiers[0].attributeName, "health");
poison.modifiers[0].value = -5.0f;  // -5 health per tick
poison.modifierCount = 1;

effectComp->AddEffect(poison);
```

## Comparison to Unreal

| Feature | Unreal GAS | SecretEngine GAS |
|---------|-----------|------------------|
| Gameplay Tags | ✅ | ✅ |
| Tag Hierarchy | ✅ | ✅ |
| Gameplay Abilities | ✅ | ✅ |
| Gameplay Effects | ✅ | ✅ |
| Attribute Sets | ✅ | ✅ |
| Tag Relationships | ✅ | ✅ |
| Data Tables | ✅ | ✅ (Single JSON) |
| Blueprint Support | ✅ | ❌ (JSON instead) |
| C++ Complexity | High | Low |
| Mobile Optimized | ❌ | ✅ |

## Next Steps

1. Load `GameDataTable.json`
2. Create characters with tags
3. Add new tags for your game
4. Define abilities and effects
5. Build any game you want!

**No hardcoding. No recompilation. Just tags.**
