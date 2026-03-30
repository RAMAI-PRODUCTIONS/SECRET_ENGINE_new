// SecretEngine
// Module: core
// Responsibility: Interface for all memory allocators
// Dependencies: <cstddef>

#pragma once
#include <cstddef>

namespace SecretEngine {

    class IAllocator {
    public:
        virtual ~IAllocator() = default;
        
        // Primary allocation method
        virtual void* Allocate(size_t size, size_t alignment) = 0;
        virtual void Free(void* ptr) = 0;
    };

    // Derived: Arena/Bump pointer
    class ILinearAllocator : public IAllocator {
    public:
        virtual void Reset() = 0;
    };

    // Derived: Fixed-size block pool
    class IPoolAllocator : public IAllocator {
    public:
        virtual size_t GetBlockSize() const = 0;
    };

}