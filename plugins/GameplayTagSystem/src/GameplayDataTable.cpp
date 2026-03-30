// SecretEngine
// Module: GameplayDataTable
// Responsibility: Load and apply gameplay data from single JSON
// Dependencies: GameplayAbilitySystem

#include "GameplayAbilitySystem.h"
#include <SecretEngine/Components.h>
#include "../../PhysicsPlugin/src/PhysicsTypes.h"
#include "../../FPSGameLogic/src/FPSComponents.h"
#include <fstream>

using namespace SecretEngine::FPS;

namespace SecretEngine::GAS {

bool GameplayDataTable::LoadDataTable(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Failed to load data table: %s", path);
        m_logger->LogError("GameplayDataTable", msg);
        return false;
    }
    
    file >> m_dataTable;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Loaded data table: %s", path);
    m_logger->LogInfo("GameplayDataTable", msg);
    
    return true;
}

Entity GameplayDataTable::CreateCharacterFromTable(const char* characterId) {
    if (!m_dataTable.contains("characterDefinitions")) {
        m_logger->LogError("GameplayDataTable", "No characterDefinitions in data table");
        return Entity::Invalid;
    }
    
    // Find character definition
    nlohmann::json charDef;
    for (const auto& def : m_dataTable["characterDefinitions"]) {
        if (def["id"] == characterId) {
            charDef = def;
            break;
        }
    }
    
    if (charDef.empty()) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Character not found: %s", characterId);
        m_logger->LogError("GameplayDataTable", msg);
        return Entity::Invalid;
    }
    
    // Create entity
    Entity entity = m_world->CreateEntity();
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Creating character from table: %s", characterId);
    m_logger->LogInfo("GameplayDataTable", msg);
    
    // Add Transform
    auto* transform = new TransformComponent();
    transform->position[0] = 0;
    transform->position[1] = 1;
    transform->position[2] = 0;
    m_world->AddComponent(entity, TransformComponent::TypeID, transform);
    
    // Add Gameplay Tag Component
    auto* tagComp = new GameplayTagComponent();
    
    // Add tags from definition
    if (charDef.contains("tags")) {
        for (const auto& tag : charDef["tags"]) {
            std::string tagStr = tag.get<std::string>();
            tagComp->AddOwnedTag(tagStr.c_str());
        }
    }
    
    m_world->AddComponent(entity, GameplayTagComponent::TypeID, tagComp);
    
    // Add Attribute Set
    auto* attributes = new AttributeSetComponent();
    attributes->Initialize();
    
    // Apply stats from tags
    ApplyStatsFromTags(entity, tagComp->ownedTags);
    
    // Apply custom stats from definition
    if (charDef.contains("stats")) {
        auto stats = charDef["stats"];
        if (stats.contains("health")) {
            attributes->health.SetBaseValue(stats["health"]);
            attributes->maxHealth.SetBaseValue(stats["health"]);
        }
        if (stats.contains("speed")) {
            attributes->speed.SetBaseValue(stats["speed"]);
        }
        if (stats.contains("stamina")) {
            attributes->stamina.SetBaseValue(stats["stamina"]);
            attributes->maxStamina.SetBaseValue(stats["stamina"]);
        }
    }
    
    m_world->AddComponent(entity, AttributeSetComponent::TypeID, attributes);
    
    // Add Ability System Component
    auto* abilityComp = new AbilitySystemComponent();
    GrantAbilitiesFromTags(entity, tagComp->ownedTags);
    m_world->AddComponent(entity, AbilitySystemComponent::TypeID, abilityComp);
    
    // Add Gameplay Effect Component
    auto* effectComp = new GameplayEffectComponent();
    m_world->AddComponent(entity, GameplayEffectComponent::TypeID, effectComp);
    
    // Add Physics Body (basic)
    auto* body = new Physics::PhysicsBody();
    body->bodyType = Physics::BodyType::Kinematic;
    body->shapeType = Physics::ShapeType::Capsule;
    body->shapeData[0] = 0.4f;  // radius
    body->shapeData[1] = 0.9f;  // half height
    body->mass = 75.0f;
    body->inverseMass = 1.0f / 75.0f;
    body->useGravity = true;
    
    // Set collision based on team
    std::string team = charDef.value("team", "player");
    if (team == "player") {
        body->collisionLayer = Physics::CollisionLayer::Player;
    } else if (team == "enemy") {
        body->collisionLayer = Physics::CollisionLayer::Enemy;
    } else {
        body->collisionLayer = Physics::CollisionLayer::Default;
    }
    body->collisionMask = Physics::CollisionLayer::All;
    
    m_world->AddComponent(entity, Physics::PhysicsBody::TypeID, body);
    
    // Add Health Component (for compatibility)
    auto* health = new HealthComponent();
    health->maximum = attributes->maxHealth.currentValue;
    health->current = attributes->health.currentValue;
    m_world->AddComponent(entity, HealthComponent::TypeID, health);
    
    // Add Team Component
    auto* teamComp = new TeamComponent();
    teamComp->team = (team == "player") ? TeamComponent::Team::Blue : TeamComponent::Team::Red;
    m_world->AddComponent(entity, TeamComponent::TypeID, teamComp);
    
    // Add AI if specified
    if (charDef.contains("ai") && charDef["ai"].value("enabled", false)) {
        auto* ai = new AIComponent();
        
        std::string behavior = charDef["ai"].value("behavior", "patrol");
        if (behavior == "aggressive") {
            ai->currentBehavior = AIComponent::Behavior::Attack;
        } else {
            ai->currentBehavior = AIComponent::Behavior::Patrol;
        }
        
        m_world->AddComponent(entity, AIComponent::TypeID, ai);
    }
    
    // Add Weapon Component if has combat ability
    if (tagComp->ownedTags.MatchesAny("Ability.Combat") || 
        tagComp->ownedTags.MatchesAny("Weapon.Type")) {
        auto* weapon = new WeaponComponent();
        weapon->ammoInMag = 30;
        weapon->ammoReserve = 90;
        
        // Get weapon stats from tags
        for (int i = 0; i < tagComp->ownedTags.count; i++) {
            const char* tag = tagComp->ownedTags.tags[i].tag;
            if (strncmp(tag, "Weapon.Type.", 12) == 0) {
                nlohmann::json weaponData = GetTagData(tag);
                if (!weaponData.empty()) {
                    weapon->damage = weaponData.value("damage", 25.0f);
                    weapon->fireRate = weaponData.value("fireRate", 0.1f);
                    weapon->range = weaponData.value("range", 100.0f);
                    weapon->magSize = weaponData.value("ammo", 30);
                    weapon->ammoInMag = weapon->magSize;
                }
            }
        }
        
        m_world->AddComponent(entity, WeaponComponent::TypeID, weapon);
    }
    
    return entity;
}

void GameplayDataTable::ApplyStatsFromTags(Entity entity, const GameplayTagContainer& tags) {
    auto* attributes = static_cast<AttributeSetComponent*>(
        m_world->GetComponent(entity, AttributeSetComponent::TypeID)
    );
    if (!attributes) return;
    
    // Apply stats from each tag
    for (int i = 0; i < tags.count; i++) {
        const char* tag = tags.tags[i].tag;
        nlohmann::json tagData = GetTagData(tag);
        
        if (tagData.contains("stats")) {
            auto stats = tagData["stats"];
            if (stats.contains("health")) {
                attributes->health.SetBaseValue(stats["health"]);
                attributes->maxHealth.SetBaseValue(stats["health"]);
            }
            if (stats.contains("speed")) {
                attributes->speed.SetBaseValue(stats["speed"]);
            }
            if (stats.contains("jumpForce")) {
                // Store in custom attribute or component
            }
        }
    }
}

void GameplayDataTable::GrantAbilitiesFromTags(Entity entity, const GameplayTagContainer& tags) {
    auto* abilityComp = static_cast<AbilitySystemComponent*>(
        m_world->GetComponent(entity, AbilitySystemComponent::TypeID)
    );
    if (!abilityComp) return;
    
    // Grant abilities from tags
    for (int i = 0; i < tags.count; i++) {
        const char* tag = tags.tags[i].tag;
        
        // Check if tag is an ability
        if (strncmp(tag, "Ability.", 8) == 0) {
            GameplayAbility ability = GetAbilityByTag(tag);
            if (ability.abilityTag.IsValid()) {
                abilityComp->GrantAbility(ability);
            }
        }
    }
}

void GameplayDataTable::ApplyEffectsFromTags(Entity entity, const GameplayTagContainer& tags) {
    // Apply any initial effects from tags
    // (Implementation depends on specific game needs)
}

GameplayAbility GameplayDataTable::GetAbilityByTag(const char* tag) {
    GameplayAbility ability;
    
    if (!m_dataTable.contains("gameplayTags")) return ability;
    
    auto& gameplayTags = m_dataTable["gameplayTags"];
    if (!gameplayTags.contains(tag)) return ability;
    
    auto abilityData = gameplayTags[tag];
    
    // Set ability properties
    strncpy(ability.abilityId, tag, sizeof(ability.abilityId) - 1);
    ability.abilityTag = GameplayTag(tag);
    
    std::string type = abilityData.value("type", "instant");
    if (type == "toggle") {
        ability.activationPolicy = GameplayAbility::OnInputHeld;
    } else if (type == "channeled") {
        ability.activationPolicy = GameplayAbility::OnInputHeld;
    } else {
        ability.activationPolicy = GameplayAbility::OnInputPressed;
    }
    
    ability.cooldown = abilityData.value("cooldown", 0.0f);
    ability.resourceCost = abilityData.value("cost", 0.0f);
    ability.staminaCost = abilityData.value("staminaCost", 0.0f);
    
    ability.damage = abilityData.value("damage", 0.0f);
    ability.range = abilityData.value("range", 0.0f);
    ability.radius = abilityData.value("radius", 0.0f);
    ability.duration = abilityData.value("duration", 0.0f);
    ability.speedMultiplier = abilityData.value("speedMultiplier", 1.0f);
    ability.damageMultiplier = abilityData.value("damageMultiplier", 1.0f);
    
    return ability;
}

nlohmann::json GameplayDataTable::GetTagData(const char* tag) {
    if (!m_dataTable.contains("gameplayTags")) return nlohmann::json::object();
    
    auto& gameplayTags = m_dataTable["gameplayTags"];
    if (!gameplayTags.contains(tag)) return nlohmann::json::object();
    
    return gameplayTags[tag];
}

void GameplayDataTable::ApplyTagsToEntity(Entity entity, const nlohmann::json& tags) {
    auto* tagComp = static_cast<GameplayTagComponent*>(
        m_world->GetComponent(entity, GameplayTagComponent::TypeID)
    );
    if (!tagComp) return;
    
    for (const auto& tag : tags) {
        std::string tagStr = tag.get<std::string>();
        tagComp->AddOwnedTag(tagStr.c_str());
    }
    
    // Reapply stats and abilities
    ApplyStatsFromTags(entity, tagComp->ownedTags);
    GrantAbilitiesFromTags(entity, tagComp->ownedTags);
}

} // namespace SecretEngine::GAS
