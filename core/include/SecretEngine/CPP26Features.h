#pragma once

/**
 * C++26 Feature Detection and Fallback Macros
 * 
 * This header provides feature detection for C++26 features and fallback
 * implementations for platforms that don't support them yet (e.g., Android NDK).
 * 
 * Design Principle: Dual-path from day one. Every C++26 feature must have a
 * C++23 fallback because Android NDK lags upstream Clang by 12-18 months.
 */

// ============================================================================
// Feature Detection
// ============================================================================

// Static Reflection (P2996R8)
#if defined(__cpp_reflection) && __cpp_reflection >= 202306L
    #define SE_HAS_REFLECTION 1
#else
    #define SE_HAS_REFLECTION 0
#endif

// Contracts (P2900)
#if defined(__cpp_contracts) && __cpp_contracts >= 202506L
    #define SE_HAS_CONTRACTS 1
#else
    #define SE_HAS_CONTRACTS 0
#endif

// std::execution (P2300)
#if defined(__cpp_lib_execution) && __cpp_lib_execution >= 202600L
    #define SE_HAS_EXECUTION 1
#else
    #define SE_HAS_EXECUTION 0
#endif

// std::inplace_vector (P0843R14)
#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202406L
    #define SE_HAS_INPLACE_VECTOR 1
    #include <inplace_vector>
#else
    #define SE_HAS_INPLACE_VECTOR 0
#endif

// Pack Indexing
#if defined(__cpp_pack_indexing) && __cpp_pack_indexing >= 202311L
    #define SE_HAS_PACK_INDEXING 1
#else
    #define SE_HAS_PACK_INDEXING 0
#endif

// std::optional<T&> (P2988R12)
#if defined(__cpp_lib_optional) && __cpp_lib_optional >= 202110L
    #define SE_HAS_OPTIONAL_REF 1
#else
    #define SE_HAS_OPTIONAL_REF 0
#endif

// std::debugging (P2546)
#if defined(__cpp_lib_debugging) && __cpp_lib_debugging >= 202403L
    #define SE_HAS_DEBUGGING 1
    #include <debugging>
#else
    #define SE_HAS_DEBUGGING 0
#endif

// std::simd
#if defined(__cpp_lib_simd) && __cpp_lib_simd >= 202306L
    #define SE_HAS_SIMD 1
    #include <simd>
#else
    #define SE_HAS_SIMD 0
#endif

// std::mdspan (C++23, but relevant)
#if defined(__cpp_lib_mdspan) && __cpp_lib_mdspan >= 202207L
    #define SE_HAS_MDSPAN 1
    #include <mdspan>
#else
    #define SE_HAS_MDSPAN 0
#endif

// constexpr placement new
#if defined(__cpp_constexpr_dynamic_alloc) && __cpp_constexpr_dynamic_alloc >= 202106L
    #define SE_HAS_CONSTEXPR_PLACEMENT_NEW 1
#else
    #define SE_HAS_CONSTEXPR_PLACEMENT_NEW 0
#endif

// ============================================================================
// Contract Macros with Fallback
// ============================================================================

#if SE_HAS_CONTRACTS
    #define SE_PRECONDITION(condition) pre(condition)
    #define SE_POSTCONDITION(condition) post(condition)
    #define SE_ASSERT(condition) assert(condition)
#else
    #include <cassert>
    #define SE_PRECONDITION(condition) assert(condition)
    #define SE_POSTCONDITION(condition) assert(condition)
    #define SE_ASSERT(condition) assert(condition)
#endif

// ============================================================================
// Debugging Macros with Fallback
// ============================================================================

#if SE_HAS_DEBUGGING
    #define SE_BREAKPOINT() std::breakpoint()
    #define SE_IS_DEBUGGER_PRESENT() std::is_debugger_present()
#else
    // Platform-specific fallbacks
    #if defined(_MSC_VER)
        #define SE_BREAKPOINT() __debugbreak()
        #define SE_IS_DEBUGGER_PRESENT() (::IsDebuggerPresent() != 0)
    #elif defined(__GNUC__) || defined(__clang__)
        #include <signal.h>
        #define SE_BREAKPOINT() raise(SIGTRAP)
        #define SE_IS_DEBUGGER_PRESENT() false  // No portable way
    #else
        #define SE_BREAKPOINT() ((void)0)
        #define SE_IS_DEBUGGER_PRESENT() false
    #endif
#endif

// ============================================================================
// inplace_vector Fallback
// ============================================================================

#if !SE_HAS_INPLACE_VECTOR
    #include <vector>
    
    namespace std {
        // Minimal fallback using std::vector
        // TODO: Replace with proper stack-based implementation
        template<typename T, size_t N>
        class inplace_vector {
            std::vector<T> data_;
        public:
            inplace_vector() { data_.reserve(N); }
            
            void push_back(const T& value) { 
                assert(data_.size() < N && "inplace_vector capacity exceeded");
                data_.push_back(value); 
            }
            
            void push_back(T&& value) { 
                assert(data_.size() < N && "inplace_vector capacity exceeded");
                data_.push_back(std::move(value)); 
            }
            
            auto begin() { return data_.begin(); }
            auto end() { return data_.end(); }
            auto begin() const { return data_.begin(); }
            auto end() const { return data_.end(); }
            
            size_t size() const { return data_.size(); }
            bool empty() const { return data_.empty(); }
            void clear() { data_.clear(); }
            
            T& operator[](size_t i) { return data_[i]; }
            const T& operator[](size_t i) const { return data_[i]; }
            
            static constexpr size_t capacity() { return N; }
        };
    }
#endif

// ============================================================================
// Platform Information
// ============================================================================

namespace SecretEngine {
    struct CPP26Support {
        static constexpr bool hasReflection = SE_HAS_REFLECTION;
        static constexpr bool hasContracts = SE_HAS_CONTRACTS;
        static constexpr bool hasExecution = SE_HAS_EXECUTION;
        static constexpr bool hasInplaceVector = SE_HAS_INPLACE_VECTOR;
        static constexpr bool hasPackIndexing = SE_HAS_PACK_INDEXING;
        static constexpr bool hasOptionalRef = SE_HAS_OPTIONAL_REF;
        static constexpr bool hasDebugging = SE_HAS_DEBUGGING;
        static constexpr bool hasSIMD = SE_HAS_SIMD;
        static constexpr bool hasMdspan = SE_HAS_MDSPAN;
        
        static void PrintSupport() {
            // Log which C++26 features are available
            // Useful for debugging platform differences
        }
    };
}

// ============================================================================
// Usage Examples
// ============================================================================

/*

// Example 1: Using inplace_vector
#include <SecretEngine/CPP26Features.h>

void CullObjects() {
    std::inplace_vector<Entity*, 1024> visible;  // Works on C++26 and C++23
    
    for (auto* entity : allEntities) {
        if (IsVisible(entity)) {
            visible.push_back(entity);
        }
    }
}

// Example 2: Using contracts
void SetVelocity(const Vec3& v)
    SE_PRECONDITION(v.Length() < MAX_VELOCITY)
{
    velocity = v;
}

// Example 3: Using debugging
void CustomAssert(bool condition, const char* msg) {
    if (!condition) {
        LogError(msg);
        if (SE_IS_DEBUGGER_PRESENT()) {
            SE_BREAKPOINT();
        }
    }
}

// Example 4: Conditional compilation
#if SE_HAS_REFLECTION
    // Use static reflection
    template<typename T>
    void AutoSerialize(const T& obj) {
        constexpr auto members = std::meta::members_of(^T);
        // ...
    }
#else
    // Use macro-based registration
    #define REGISTER_COMPONENT(T) // ...
#endif

*/
