#pragma once
#include <SecretEngine/IAllocator.h>
#include <cstdint>

namespace SecretEngine {
    class SystemAllocator : public IAllocator {
    public:
        void* Allocate(size_t size, size_t alignment) override;
        void Free(void* ptr) override;
        
        // Memory tracking (thread-safe)
        static uint64_t GetTotalAllocated();
        static uint64_t GetAllocationCount();
    };
}