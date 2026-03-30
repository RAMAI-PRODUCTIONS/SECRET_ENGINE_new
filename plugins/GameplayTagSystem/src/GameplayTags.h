// SecretEngine
// Module: GameplayTagSystem
// Responsibility: Unreal-like Gameplay Tag System
// Dependencies: Core

#pragma once
#include <cstdint>
#include <cstring>

namespace SecretEngine::GAS {

// Gameplay Tag (hierarchical string-based tag)
struct GameplayTag {
    static constexpr int MAX_TAG_LENGTH = 128;
    char tag[MAX_TAG_LENGTH] = "";
    
    GameplayTag() = default;
    GameplayTag(const char* t) {
        strncpy(tag, t, MAX_TAG_LENGTH - 1);
    }
    
    bool IsValid() const { return tag[0] != '\0'; }
    bool IsEmpty() const { return tag[0] == '\0'; }
    
    // Check if this tag matches or is a child of parent
    // e.g., "Character.Class.Soldier" matches "Character"
    bool MatchesTag(const char* parent) const {
        return strncmp(tag, parent, strlen(parent)) == 0;
    }
    
    bool operator==(const GameplayTag& other) const {
        return strcmp(tag, other.tag) == 0;
    }
};

// Tag Container (holds multiple tags)
struct GameplayTagContainer {
    static constexpr int MAX_TAGS = 32;
    GameplayTag tags[MAX_TAGS];
    int count = 0;
    
    void AddTag(const GameplayTag& tag) {
        if (count < MAX_TAGS && !HasTag(tag)) {
            tags[count++] = tag;
        }
    }
    
    void AddTag(const char* tag) {
        AddTag(GameplayTag(tag));
    }
    
    void RemoveTag(const GameplayTag& tag) {
        for (int i = 0; i < count; i++) {
            if (tags[i] == tag) {
                // Shift remaining tags
                for (int j = i; j < count - 1; j++) {
                    tags[j] = tags[j + 1];
                }
                count--;
                return;
            }
        }
    }
    
    bool HasTag(const GameplayTag& tag) const {
        for (int i = 0; i < count; i++) {
            if (tags[i] == tag) return true;
        }
        return false;
    }
    
    bool HasTag(const char* tag) const {
        return HasTag(GameplayTag(tag));
    }
    
    bool HasAnyTag(const GameplayTagContainer& other) const {
        for (int i = 0; i < other.count; i++) {
            if (HasTag(other.tags[i])) return true;
        }
        return false;
    }
    
    bool HasAllTags(const GameplayTagContainer& other) const {
        for (int i = 0; i < other.count; i++) {
            if (!HasTag(other.tags[i])) return false;
        }
        return true;
    }
    
    // Check if any tag matches parent hierarchy
    bool MatchesAny(const char* parent) const {
        for (int i = 0; i < count; i++) {
            if (tags[i].MatchesTag(parent)) return true;
        }
        return false;
    }
    
    void Clear() {
        count = 0;
    }
};

// Gameplay Tag Component (attached to entities)
struct GameplayTagComponent {
    static constexpr uint32_t TypeID = 0x4001;
    
    GameplayTagContainer ownedTags;      // Permanent tags
    GameplayTagContainer activeTags;     // Temporary tags (buffs, debuffs)
    GameplayTagContainer blockedTags;    // Tags that are blocked
    
    // Check if entity has tag (owned or active)
    bool HasTag(const char* tag) const {
        return ownedTags.HasTag(tag) || activeTags.HasTag(tag);
    }
    
    bool HasTag(const GameplayTag& tag) const {
        return ownedTags.HasTag(tag) || activeTags.HasTag(tag);
    }
    
    // Check if tag is blocked
    bool IsTagBlocked(const char* tag) const {
        return blockedTags.HasTag(tag);
    }
    
    // Add/remove tags
    void AddOwnedTag(const char* tag) {
        ownedTags.AddTag(tag);
    }
    
    void AddActiveTag(const char* tag) {
        activeTags.AddTag(tag);
    }
    
    void RemoveActiveTag(const char* tag) {
        activeTags.RemoveTag(GameplayTag(tag));
    }
    
    void BlockTag(const char* tag) {
        blockedTags.AddTag(tag);
    }
    
    void UnblockTag(const char* tag) {
        blockedTags.RemoveTag(GameplayTag(tag));
    }
};

// Gameplay Attribute (stats that can be modified)
struct GameplayAttribute {
    float baseValue = 0.0f;
    float currentValue = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1000.0f;
    
    void SetBaseValue(float value) {
        baseValue = value;
        currentValue = value;
        Clamp();
    }
    
    void SetCurrentValue(float value) {
        currentValue = value;
        Clamp();
    }
    
    void AddModifier(float value) {
        currentValue += value;
        Clamp();
    }
    
    void MultiplyModifier(float multiplier) {
        currentValue *= multiplier;
        Clamp();
    }
    
    void Clamp() {
        if (currentValue < minValue) currentValue = minValue;
        if (currentValue > maxValue) currentValue = maxValue;
    }
    
    float GetPercent() const {
        if (maxValue <= 0) return 0;
        return currentValue / maxValue;
    }
};

// Attribute Set Component
struct AttributeSetComponent {
    static constexpr uint32_t TypeID = 0x4002;
    
    // Core attributes
    GameplayAttribute health;
    GameplayAttribute maxHealth;
    GameplayAttribute stamina;
    GameplayAttribute maxStamina;
    GameplayAttribute speed;
    GameplayAttribute damage;
    GameplayAttribute defense;
    
    // Resource
    GameplayAttribute resource;  // Mana, energy, etc.
    GameplayAttribute maxResource;
    
    void Initialize() {
        health.SetBaseValue(100.0f);
        maxHealth.SetBaseValue(100.0f);
        stamina.SetBaseValue(100.0f);
        maxStamina.SetBaseValue(100.0f);
        speed.SetBaseValue(5.0f);
        damage.SetBaseValue(25.0f);
        defense.SetBaseValue(0.0f);
        resource.SetBaseValue(100.0f);
        maxResource.SetBaseValue(100.0f);
    }
};

// Gameplay Effect (temporary modifier)
struct GameplayEffect {
    enum Type {
        Instant,        // Apply once
        Duration,       // Apply for duration
        Infinite        // Apply until removed
    };
    
    enum ModifierOp {
        Add,
        Multiply,
        Override
    };
    
    Type type = Instant;
    float duration = 0.0f;
    float period = 0.0f;  // For periodic effects
    
    GameplayTagContainer applicationTags;  // Tags required to apply
    GameplayTagContainer grantedTags;      // Tags granted while active
    GameplayTagContainer blockedTags;      // Tags blocked while active
    
    // Modifiers (simplified - just store values)
    struct Modifier {
        char attributeName[32] = "";
        ModifierOp operation = Add;
        float value = 0.0f;
    };
    
    Modifier modifiers[8];
    int modifierCount = 0;
};

// Active Gameplay Effect (instance of effect on entity)
struct ActiveGameplayEffect {
    GameplayEffect effect;
    float timeRemaining = 0.0f;
    float periodTimer = 0.0f;
    bool isActive = true;
    uint32_t sourceEntityId = 0;
};

// Gameplay Effect Component
struct GameplayEffectComponent {
    static constexpr uint32_t TypeID = 0x4003;
    
    ActiveGameplayEffect activeEffects[16];
    int effectCount = 0;
    
    void AddEffect(const GameplayEffect& effect, uint32_t sourceId = 0) {
        if (effectCount >= 16) return;
        
        auto& active = activeEffects[effectCount++];
        active.effect = effect;
        active.timeRemaining = effect.duration;
        active.periodTimer = effect.period;
        active.isActive = true;
        active.sourceEntityId = sourceId;
    }
    
    void RemoveEffect(int index) {
        if (index < 0 || index >= effectCount) return;
        
        // Shift remaining effects
        for (int i = index; i < effectCount - 1; i++) {
            activeEffects[i] = activeEffects[i + 1];
        }
        effectCount--;
    }
    
    void UpdateEffects(float dt) {
        for (int i = effectCount - 1; i >= 0; i--) {
            auto& effect = activeEffects[i];
            
            if (effect.effect.type == GameplayEffect::Duration) {
                effect.timeRemaining -= dt;
                if (effect.timeRemaining <= 0) {
                    RemoveEffect(i);
                }
            }
            
            // Handle periodic effects
            if (effect.effect.period > 0) {
                effect.periodTimer -= dt;
                if (effect.periodTimer <= 0) {
                    effect.periodTimer = effect.effect.period;
                    // Apply periodic effect (handled by system)
                }
            }
        }
    }
};

} // namespace SecretEngine::GAS
