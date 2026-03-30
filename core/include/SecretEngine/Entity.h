// SecretEngine
// Module: core
// Responsibility: POD Entity handle definition
// Dependencies: <cstdint>

#pragma once
#include <cstdint>

namespace SecretEngine {

    // Entity is a handle, not an object
    struct Entity {
        uint32_t id;
        uint32_t generation;

        // Standard comparison
        bool operator==(const Entity& other) const { return id == other.id && generation == other.generation; }
        bool operator!=(const Entity& other) const { return !(*this == other); }

        // Declare static member without initializing it inside the incomplete type
        static const Entity Invalid;
    };

    // Initialize after the struct definition to resolve C2027
    inline const Entity Entity::Invalid = { 0, 0 };

}