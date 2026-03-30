# Create Any Game - Examples

## The Power of Tags

With the Gameplay Tag System, you can create **any game type** by just editing one JSON file. No code changes needed!

## Example 1: FPS Game (Current)

```json
{
  "id": "fps_soldier",
  "tags": [
    "Character.Class.Soldier",
    "Ability.Movement.Sprint",
    "Ability.Combat.Shoot",
    "Weapon.Type.Rifle"
  ]
}
```

**Result**: Standard FPS soldier with sprint and rifle

## Example 2: Battle Royale

### Add to GameDataTable.json:

```json
"Ability.Movement.Slide": {
  "type": "instant",
  "duration": 0.5,
  "speedMultiplier": 1.8
},
"Ability.Combat.BuildWall": {
  "type": "instant",
  "cooldown": 5.0,
  "cost": 10.0
},
"Ability.Survival.Bandage": {
  "type": "channeled",
  "duration": 3.0,
  "cooldown": 10.0,
  "healAmount": 50.0
}
```

### Create Character:

```json
{
  "id": "br_player",
  "tags": [
    "Character.Class.Soldier",
    "Ability.Movement.Sprint",
    "Ability.Movement.Slide",
    "Ability.Combat.Shoot",
    "Ability.Combat.BuildWall",
    "Ability.Survival.Bandage"
  ]
}
```

**Done!** You have a Battle Royale character with building and healing.

## Example 3: Racing Game

### Add Tags:

```json
"Character.Type.RaceCar": {
  "stats": {
    "speed": 35.0,
    "acceleration": 25.0,
    "handling": 8.0
  }
},
"Ability.Vehicle.Boost": {
  "type": "instant",
  "cooldown": 10.0,
  "speedMultiplier": 2.0,
  "duration": 3.0
},
"Ability.Vehicle.Drift": {
  "type": "toggle",
  "handlingMultiplier": 1.5
}
```

### Create Vehicle:

```json
{
  "id": "formula_car",
  "tags": [
    "Character.Type.RaceCar",
    "Ability.Vehicle.Boost",
    "Ability.Vehicle.Drift"
  ]
}
```

**Done!** Racing game with boost and drift.

## Example 4: RPG/MOBA

### Add Tags:

```json
"Character.Class.Mage": {
  "stats": {
    "health": 80,
    "mana": 200,
    "intelligence": 20,
    "strength": 5
  }
},
"Ability.Magic.Fireball": {
  "type": "instant",
  "cooldown": 5.0,
  "cost": 50.0,
  "damage": 100.0,
  "radius": 3.0
},
"Ability.Magic.Teleport": {
  "type": "instant",
  "cooldown": 15.0,
  "cost": 75.0,
  "range": 20.0
},
"Ability.Magic.Shield": {
  "type": "toggle",
  "duration": 5.0,
  "cooldown": 20.0,
  "damageReduction": 0.7
},
"Ability.Ultimate.Meteor": {
  "type": "instant",
  "cooldown": 60.0,
  "cost": 150.0,
  "damage": 500.0,
  "radius": 10.0
}
```

### Create Hero:

```json
{
  "id": "hero_mage",
  "tags": [
    "Character.Class.Mage",
    "Ability.Magic.Fireball",
    "Ability.Magic.Teleport",
    "Ability.Magic.Shield",
    "Ability.Ultimate.Meteor"
  ]
}
```

**Done!** MOBA-style mage with 4 abilities.

## Example 5: Survival Horror

### Add Tags:

```json
"Character.Class.Survivor": {
  "stats": {
    "health": 100,
    "stamina": 80,
    "fear": 0
  }
},
"Ability.Survival.Hide": {
  "type": "toggle",
  "staminaCost": 5.0,
  "detectionReduction": 0.8
},
"Ability.Survival.Flashlight": {
  "type": "toggle",
  "batteryCost": 1.0,
  "visionRange": 15.0
},
"Status.Debuff.Scared": {
  "speedMultiplier": 0.7,
  "accuracyMultiplier": 0.5
}
```

### Create Survivor:

```json
{
  "id": "survivor",
  "tags": [
    "Character.Class.Survivor",
    "Ability.Survival.Hide",
    "Ability.Survival.Flashlight",
    "Ability.Combat.Melee"
  ]
}
```

**Done!** Survival horror character with hiding and limited combat.

## Example 6: Tower Defense

### Add Tags:

```json
"Character.Type.Tower": {
  "stats": {
    "health": 500,
    "range": 15.0,
    "fireRate": 1.0
  }
},
"Ability.Tower.AutoTarget": {
  "type": "passive",
  "targetPriority": "closest"
},
"Ability.Tower.Upgrade": {
  "type": "instant",
  "damageIncrease": 1.5,
  "rangeIncrease": 1.2
}
```

### Create Tower:

```json
{
  "id": "basic_tower",
  "tags": [
    "Character.Type.Tower",
    "Ability.Tower.AutoTarget",
    "Ability.Tower.Upgrade",
    "Weapon.Type.MachineGun"
  ],
  "ai": {
    "enabled": true,
    "behavior": "defensive"
  }
}
```

**Done!** Tower defense tower with auto-targeting.

## Example 7: Fighting Game

### Add Tags:

```json
"Character.Class.Fighter": {
  "stats": {
    "health": 100,
    "speed": 6.0,
    "comboMeter": 0
  }
},
"Ability.Combat.LightPunch": {
  "type": "instant",
  "damage": 10.0,
  "speed": 0.1,
  "comboPoints": 1
},
"Ability.Combat.HeavyPunch": {
  "type": "instant",
  "damage": 30.0,
  "speed": 0.5,
  "comboPoints": 3
},
"Ability.Combat.Special": {
  "type": "instant",
  "cooldown": 10.0,
  "damage": 50.0,
  "requiredCombo": 5
}
```

### Create Fighter:

```json
{
  "id": "fighter_brawler",
  "tags": [
    "Character.Class.Fighter",
    "Ability.Combat.LightPunch",
    "Ability.Combat.HeavyPunch",
    "Ability.Combat.Special",
    "Ability.Movement.Dash"
  ]
}
```

**Done!** Fighting game character with combo system.

## Example 8: Platformer

### Add Tags:

```json
"Character.Class.Platformer": {
  "stats": {
    "health": 3,
    "speed": 7.0,
    "jumpForce": 12.0
  }
},
"Ability.Movement.DoubleJump": {
  "type": "instant",
  "forceMultiplier": 0.8
},
"Ability.Movement.WallJump": {
  "type": "instant",
  "forceMultiplier": 1.2
},
"Ability.Movement.Glide": {
  "type": "toggle",
  "fallSpeedMultiplier": 0.3
}
```

### Create Character:

```json
{
  "id": "platformer_hero",
  "tags": [
    "Character.Class.Platformer",
    "Ability.Movement.DoubleJump",
    "Ability.Movement.WallJump",
    "Ability.Movement.Glide"
  ]
}
```

**Done!** Platformer with advanced movement.

## The Process

### Step 1: Define Tags (Once)
Add new tags to `GameDataTable.json` with their properties

### Step 2: Create Characters
Assign tags to characters - tags define everything

### Step 3: Play
Load character from table, everything works automatically

## Mix and Match

Want a **racing FPS**? Combine tags:
```json
{
  "id": "combat_racer",
  "tags": [
    "Character.Type.RaceCar",
    "Ability.Vehicle.Boost",
    "Ability.Combat.Shoot",
    "Weapon.Type.MachineGun"
  ]
}
```

Want a **magic platformer**? Combine tags:
```json
{
  "id": "magic_jumper",
  "tags": [
    "Character.Class.Platformer",
    "Ability.Movement.DoubleJump",
    "Ability.Magic.Fireball",
    "Ability.Magic.Teleport"
  ]
}
```

## No Limits

The system is **completely flexible**:
- ✅ Any genre
- ✅ Any mechanic
- ✅ Any combination
- ✅ No code changes
- ✅ Just edit JSON

## Real Example: From FPS to Battle Royale

**Current FPS** (5 minutes):
1. Open `GameDataTable.json`
2. Add slide, build, heal tags
3. Create new character with those tags
4. Done!

**No compilation. No code. Just tags.**

## Summary

With the Gameplay Tag System:
- **One JSON file** controls everything
- **Tags define** what entities can do
- **Mix and match** to create any game
- **No hardcoding** ever needed
- **Designer friendly** - no programming required

**You can literally create any game you want by just editing one JSON file.**
