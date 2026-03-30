// SecretEngine
// Module: GameplayAbilitySystem
// Responsibility: Unreal-like Gameplay Ability System (GAS)
// Dependencies: GameplayTags

#pragma once
#include "GameplayTags.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/ILogger.h>
#include <nlohmann/json.hpp>

namespace SecretEngine::GAS {

// Gameplay Ability Definition
struct GameplayAbility {
    char abilityId[64] = "";
    GameplayTag abilityTag;
    
    enum ActivationPolicy {
        OnInputPressed,
        OnInputHeld,
        OnInputReleased,
        OnEvent,
        Passive
    };
    
    ActivationPolicy activationPolicy = OnInputPressed;
    
    // Costs
    float cooldown = 0.0f;
    float resourceCost = 0.0f;
    float staminaCost = 0.0f;
    
    // Requirements
    GameplayTagContainer requiredTags;
    GameplayTagContainer blockedTags;
    
    // Effects
    GameplayTagContainer grantedTags;
    GameplayEffect effectToApply;
    
    // Ability data (flexible)
    float damage = 0.0f;
    float range = 0.0f;
    float radius = 0.0f;
    float duration = 0.0f;
    float speedMultiplier = 1.0f;
    float damageMultiplier = 1.0f;
};

// Active Ability Instance
struct ActiveAbility {
    GameplayAbility ability;
    float cooldownRemaining = 0.0f;
    float durationRemaining = 0.0f;
    bool isActive = false;
    bool isOnCooldown = false;
};

// Ability System Component
struct AbilitySystemComponent {
    static constexpr uint32_t TypeID = 0x4004;
    
    ActiveAbility abilities[16];
    int abilityCount = 0;
    
    void GrantAbility(const GameplayAbility& ability) {
        if (abilityCount >= 16) return;
        
        auto& active = abilities[abilityCount++];
        active.ability = ability;
        active.cooldownRemaining = 0.0f;
        active.durationRemaining = 0.0f;
        active.isActive = false;
        active.isOnCooldown = false;
    }
    
    ActiveAbility* FindAbility(const char* abilityId) {
        for (int i = 0; i < abilityCount; i++) {
            if (strcmp(abilities[i].ability.abilityId, abilityId) == 0) {
                return &abilities[i];
            }
        }
        return nullptr;
    }
    
    ActiveAbility* FindAbilityByTag(const char* tag) {
        for (int i = 0; i < abilityCount; i++) {
            if (strcmp(abilities[i].ability.abilityTag.tag, tag) == 0) {
                return &abilities[i];
            }
        }
        return nullptr;
    }
    
    bool CanActivateAbility(const ActiveAbility* ability, const GameplayTagComponent* tags) const {
        if (!ability || ability->isOnCooldown) return false;
        
        // Check required tags
        if (ability->ability.requiredTags.count > 0) {
            if (!tags || !tags->ownedTags.HasAllTags(ability->ability.requiredTags)) {
                return false;
            }
        }
        
        // Check blocked tags
        if (ability->ability.blockedTags.count > 0) {
            if (tags && tags->HasTag(ability->ability.blockedTags.tags[0])) {
                return false;
            }
        }
        
        return true;
    }
    
    void UpdateAbilities(float dt) {
        for (int i = 0; i < abilityCount; i++) {
            auto& ability = abilities[i];
            
            // Update cooldown
            if (ability.isOnCooldown) {
                ability.cooldownRemaining -= dt;
                if (ability.cooldownRemaining <= 0) {
                    ability.cooldownRemaining = 0;
                    ability.isOnCooldown = false;
                }
            }
            
            // Update duration
            if (ability.isActive && ability.durationRemaining > 0) {
                ability.durationRemaining -= dt;
                if (ability.durationRemaining <= 0) {
                    ability.durationRemaining = 0;
                    ability.isActive = false;
                }
            }
        }
    }
};

// Data Table Loader
class GameplayDataTable {
public:
    GameplayDataTable(ICore* core) 
        : m_core(core)
        , m_world(core->GetWorld())
        , m_logger(core->GetLogger()) {}
    
    // Load entire data table
    bool LoadDataTable(const char* path);
    
    // Create entity from character definition
    Entity CreateCharacterFromTable(const char* characterId);
    
    // Get ability definition by tag
    GameplayAbility GetAbilityByTag(const char* tag);
    
    // Apply tags to entity
    void ApplyTagsToEntity(Entity entity, const nlohmann::json& tags);
    
    // Get tag data
    nlohmann::json GetTagData(const char* tag);
    
private:
    ICore* m_core;
    IWorld* m_world;
    ILogger* m_logger;
    
    nlohmann::json m_dataTable;
    
    void ApplyStatsFromTags(Entity entity, const GameplayTagContainer& tags);
    void GrantAbilitiesFromTags(Entity entity, const GameplayTagContainer& tags);
    void ApplyEffectsFromTags(Entity entity, const GameplayTagContainer& tags);
};

} // namespace SecretEngine::GAS
