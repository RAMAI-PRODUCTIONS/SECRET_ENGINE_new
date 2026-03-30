// SecretEngine
// Module: core
// Responsibility: Implementation of system malloc/free with alignment
// Dependencies: SystemAllocator.h, <cstdlib>

#include "SystemAllocator.h"
#include <cstdlib>
#include <atomic>

// === MEMORY TRACKING (Thread-Safe) ===
namespace {
    std::atomic<uint64_t> g_totalAllocated{0};
    std::atomic<uint64_t> g_allocationCount{0};
}

namespace SecretEngine {

    void* SystemAllocator::Allocate(size_t size, size_t alignment) {
        // Alignment must be a power of 2 for these OS functions
        void* ptr = nullptr;
#if defined(_WIN32)
        ptr = _aligned_malloc(size, alignment);
#else
        if (posix_memalign(&ptr, alignment, size) != 0) return nullptr;
#endif
        
        // === TRACK ALLOCATION ===
        if (ptr) {
            g_totalAllocated.fetch_add(size, std::memory_order_relaxed);
            g_allocationCount.fetch_add(1, std::memory_order_relaxed);
        }
        
        return ptr;
    }

    // FIX: Added SystemAllocator:: scope to match the header declaration
    void SystemAllocator::Free(void* ptr) {
        if (!ptr) return;
        
        // Note: We can't track deallocation size without storing metadata
        // This is acceptable - we track peak usage, not current usage
        
#if defined(_WIN32)
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
    
    // === MEMORY STATS API ===
    uint64_t SystemAllocator::GetTotalAllocated() {
        return g_totalAllocated.load(std::memory_order_relaxed);
    }
    
    uint64_t SystemAllocator::GetAllocationCount() {
        return g_allocationCount.load(std::memory_order_relaxed);
    }
}