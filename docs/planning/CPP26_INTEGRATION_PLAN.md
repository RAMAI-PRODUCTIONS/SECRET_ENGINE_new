# C++26 Integration Plan for SECRET_ENGINE

## Executive Summary

C++26 feature freeze completed in June 2025, with final standard expected March 2026. This document outlines strategic integration of C++26 features into SECRET_ENGINE to improve performance, safety, and maintainability.

## Current Project Status

- **Language Standard**: C++20/23
- **Target Platforms**: Android (primary), with cross-platform architecture
- **Core Architecture**: Plugin-based engine with Vulkan renderer
- **Key Systems**: ECS, job system, mega-geometry rendering, physics

## ⚠️ CRITICAL: Android NDK Reality Check

**The Android NDK typically trails upstream Clang by 12-18 months.**

- Clang 19 shipped in NDK r27 (late 2024)
- Clang 20+ not yet available in NDK
- **Full C++26 support on Android may not arrive until 2027**

**Implication**: Every C++26 feature must have a `#if __cpp_feature` fallback to C++23 implementation. Design for dual-path from day one.

## C++26 Feature Freeze Status

- **Feature Complete**: June 2025 (Sofia, Bulgaria)
- **Committee Draft**: International ballot phase ongoing
- **Final Standard**: Expected March 2026
- **Compiler Support**: GCC 14+ and Clang 19+ with `-std=c++2c` flag

---

## Priority 0: Immediate Wins (Available Now)

### 0.1 Memory Safety Hardening - DO THIS NOW

**Status**: Available in GCC 14/Clang 19 (already in your toolchain)

**Action**: Enable immediately - zero code changes required.

```cmake
# Add to CMakeLists.txt NOW
if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fhardened)
        add_compile_options(-fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()
```

**Benefits**:
- Uninitialized reads become erroneous behavior (EB) instead of UB
- Immediate bug detection in debug builds
- No runtime cost in release builds

**Timeline**: Implement today

---

### 0.2 std::inplace_vector<T, N> - Top Priority

**What It Is**: Fixed-capacity vector on the stack with std::vector API.

**Why This Is Critical for SECRET_ENGINE**:
- **ECS Component Pools**: No heap allocation for component queries
- **Per-Frame Job Queues**: Stack-allocated job lists
- **Culling Results**: Fixed-size visible object lists
- **Physics Contacts**: Bounded collision pairs per frame

**Implementation Strategy**:
```cpp
// Current approach (heap allocation):
std::vector<Entity*> visibleEntities;
visibleEntities.reserve(MAX_VISIBLE);

// C++26 approach (stack allocation):
std::inplace_vector<Entity*, 1024> visibleEntities;
// Same API, zero heap overhead, cache-friendly
```

**Performance Impact**: 
- Eliminates allocator overhead in hot paths
- Better cache locality
- Predictable memory usage

**Timeline**: Q2 2026 (high priority after compiler support)

---

### 0.3 Pack Indexing (args...[I])

**What It Is**: Direct access to parameter pack elements.

**Benefits for SECRET_ENGINE**:
```cpp
// Current approach (complex):
template<typename... Components>
auto GetComponents(EntityID id) {
    return std::tuple{GetComponent<Components>(id)...};
}

// C++26 approach (simple):
template<typename... Components>
auto GetComponent(EntityID id, size_t index) {
    return GetComponent<Components...[index]>(id);
}
```

**Integration Points**: Component queries, variadic system updates

**Timeline**: Q2 2026

---

## Priority 1: High-Impact Features for Game Engines

### 1. Static Reflection (P2996R8)

**What It Is**: Compile-time introspection enabling code to inspect its own structure and automatically generate boilerplate.

**Benefits for SECRET_ENGINE**:
- **Component Registration**: Auto-register ECS components without manual macros
- **Serialization**: Automatic JSON/binary serialization for scene files
- **Plugin Discovery**: Introspect plugin interfaces at compile-time
- **ImGui Property Editors**: Auto-generate debug UI without macros
- **Network Replication**: Auto-generate replication descriptors
- **Asset Pipeline**: Compile-time asset type detection

**Critical Note**: Requires `std::meta::define_aggregate` for full ECS integration, not just `members_of`.

**Implementation Strategy**:
```cpp
// Current approach (manual):
REGISTER_COMPONENT(TransformComponent);
REGISTER_COMPONENT(MeshComponent);

// C++26 approach (automatic):
template<typename T>
constexpr void auto_register_component() {
    constexpr auto members = std::meta::members_of(^T);
    // Auto-generate serialization, reflection, etc.
}
```

**Integration Points**:
- `core/include/SecretEngine/Components.h` - Component reflection
- `core/src/World.cpp` - Entity serialization
- `plugins/*/src/*.cpp` - Plugin metadata generation
- Asset pipeline - Automatic asset type detection

**Timeline**: Q3-Q4 2026 (after compiler support stabilizes)

**Android Fallback Required**: 
```cpp
#if __cpp_reflection >= 202306L
    // C++26 reflection path
#else
    // C++23 macro-based registration
    REGISTER_COMPONENT(TransformComponent)
#endif
```

---

### 2. std::execution - Senders/Receivers (P2300)

**⚠️ RISK ASSESSMENT: HIGH - Not Medium**

**What It Is**: Modern async framework that fundamentally changes error propagation and composition.

**Why This Is Complex for SECRET_ENGINE**:
- **Error Model Change**: Vulkan errors, CPU sync errors, timeout errors must all map to sender chains
- **Not a Simple Rewrite**: This changes the entire async architecture
- **Vulkan Integration**: GPU submission paths mix multiple error domains
- **6-9 Month Design Phase Required**: Not a quick migration

**Benefits for SECRET_ENGINE** (after careful design):
- **Structured Concurrency**: Better async composition
- **Type-Safe Error Handling**: Errors propagate through sender chains
- **GPU-CPU Coordination**: Cleaner Vulkan command buffer patterns (after design work)
- **Asset Loading**: Structured async asset streaming

**Implementation Strategy**:
```cpp
// Current approach:
jobSystem->Submit([](){ /* work */ });

// C++26 approach:
auto work = std::execution::schedule(scheduler)
    | std::execution::then([](){ /* work */ })
    | std::execution::on(gpu_context);
```

**Integration Points**:
- `core/include/SecretEngine/JobSystem.h` - Modernize job API
- `plugins/VulkanRenderer/src/RendererPlugin.cpp` - Async rendering
- `core/src/AssetProvider.cpp` - Async asset loading
- Physics updates - Parallel physics simulation

**Performance Impact**: 
- Better CPU utilization through structured concurrency
- Reduced thread synchronization overhead
- Cleaner error propagation

**Recommended Approach**: Shadow implementation alongside existing job system.

**Timeline**: 
- Q2 2026: Design phase - error handling strategy
- Q3 2026: Shadow implementation (run both systems in parallel)
- Q4 2026: Prove behavioral parity on all platforms including Android
- Q1 2027: Cut over after validation

**DO NOT attempt single-step rewrite while shipping.**

---

### 3. SIMD Parallelism (std::simd)

**What It Is**: Portable SIMD operations without platform-specific intrinsics.

**Benefits for SECRET_ENGINE**:
- **Math Library**: Vectorize matrix/vector operations
- **Culling**: SIMD frustum culling for mega-geometry
- **Physics**: Vectorized collision detection
- **Animation**: Parallel bone transformations

**Implementation Strategy**:
```cpp
// Current approach (scalar):
for (int i = 0; i < count; ++i) {
    result[i] = a[i] * b[i] + c[i];
}

// C++26 approach (SIMD):
using simd_float = std::simd<float, std::simd_abi::native>;
for (int i = 0; i < count; i += simd_float::size()) {
    simd_float va(&a[i]);
    simd_float vb(&b[i]);
    simd_float vc(&c[i]);
    (va * vb + vc).copy_to(&result[i]);
}
```

**Integration Points**:
- `core/include/SecretEngine/Math.h` - SIMD math operations
- `plugins/VulkanRenderer/src/MegaGeometryRenderer.cpp` - Culling
- `plugins/PhysicsPlugin/src/CollisionDetection.h` - Broadphase
- `core/include/SecretEngine/Fast/FDA_Math.h` - Fast data algorithms

**Performance Impact**: 2-8x speedup on math-heavy operations

**Timeline**: Q4 2026

---

## Priority 2: Safety and Reliability

### 4. Contracts (P2900)

**⚠️ RISK ASSESSMENT: MEDIUM-HIGH - Not Low**

**What It Is**: Native precondition/postcondition checking with compile-time and runtime enforcement.

**Why Risk Is Higher Than Expected**:
- Violation handler semantics changed late in standardization
- Must decide on violation handler strategy before touching APIs
- Build configuration complexity across debug/release/shipping

**Benefits for SECRET_ENGINE**:
- **API Safety**: Enforce valid input ranges (e.g., normalized vectors)
- **Resource Management**: Validate handle lifetimes
- **Physics Constraints**: Ensure valid physics parameters
- **Memory Safety**: Catch buffer overruns earlier

**Implementation Strategy**:
```cpp
// Current approach:
void SetVelocity(const Vec3& v) {
    assert(v.Length() < MAX_VELOCITY);
    velocity = v;
}

// C++26 approach:
void SetVelocity(const Vec3& v)
    pre(v.Length() < MAX_VELOCITY)
    post(velocity == v)
{
    velocity = v;
}
```

**Integration Points**:
- All public APIs in `core/include/SecretEngine/*.h`
- Plugin interfaces
- Math library functions
- Resource allocation functions

**Build Modes**:
- **Debug**: Full contract checking
- **Release**: Audit-level contracts only
- **Shipping**: Contracts compiled out

**Timeline**: Q2 2026

---

### 5. Memory Safety Improvements

**What It Is**: Uninitialized variable reads become "erroneous behavior" (EB) instead of undefined behavior (UB).

**Benefits for SECRET_ENGINE**:
- **Predictable Debugging**: Easier to track down initialization bugs
- **Static Analysis**: Better tooling support
- **Hardened STL**: Standard library with bounds checking

**Implementation Strategy**:
- Enable `-fhardened` or equivalent compiler flags
- Use `std::span` instead of raw pointers where possible
- Leverage standard library hardening modes
- Integrate with sanitizers for development builds

**Integration Points**:
- All memory allocation paths
- Buffer management in renderer
- Asset loading and parsing
- Plugin data exchange

**Timeline**: Q1 2026 (compiler support available now)

---

## Priority 3: Modern C++ Enhancements

### 6. std::optional<T&> (P2988R12)

**What It Is**: Optional references, enabling cleaner nullable reference semantics.

**Benefits for SECRET_ENGINE**:
```cpp
// Current approach:
Entity* FindEntity(EntityID id); // nullptr if not found

// C++26 approach:
std::optional<Entity&> FindEntity(EntityID id); // cleaner semantics
```

**Integration Points**:
- Entity lookup functions
- Component queries
- Resource management
- Plugin queries

**Timeline**: Q1 2026 (already in beman.optional library)

---

### 7. std::optional Range Support (P3168R2)

**What It Is**: Treat optional as a 0-or-1 element range.

**Benefits for SECRET_ENGINE**:
```cpp
// Cleaner optional handling:
for (auto& entity : FindEntity(id)) {
    entity.Update();
}
```

**Timeline**: Q1 2026

---

## Priority 3: Additional High-Value Features

### 8. std::debugging (P2546)

**What It Is**: Standard breakpoint and debugger detection.

**Benefits for SECRET_ENGINE**:
```cpp
void Assert(bool condition, const char* msg) {
    if (!condition) {
        LogError(msg);
        if (std::is_debugger_present()) {
            std::breakpoint(); // Break into debugger
        }
    }
}
```

**Integration Points**: Debug plugin, assertion system

**Timeline**: Q2 2026

---

### 9. std::span and std::mdspan

**Status**: C++23 feature, but landing in toolchains alongside C++26

**What It Is**: Multi-dimensional span for array views.

**Benefits for SECRET_ENGINE**:
```cpp
// Mega-geometry vertex buffer access
void ProcessVertices(std::mdspan<float, std::dextents<size_t, 2>> vertices) {
    // vertices[i, 0] = x, vertices[i, 1] = y, vertices[i, 2] = z
    for (size_t i = 0; i < vertices.extent(0); ++i) {
        ProcessVertex(vertices[i, 0], vertices[i, 1], vertices[i, 2]);
    }
}
```

**Integration Points**: 
- `MegaGeometryRenderer.cpp` - Vertex/index buffer views
- Texture data access
- Physics collision meshes

**Timeline**: Q1 2026 (available now in most compilers)

---

### 10. constexpr placement new

**What It Is**: Enables compile-time object construction in pre-allocated storage.

**Benefits for SECRET_ENGINE**:
- Compile-time asset pipeline initialization
- Constexpr object pools
- Static component registration

**Timeline**: Q3 2026

---

### 11. Hazard Pointers and RCU (std::hazard_pointer, std::rcu)

**What It Is**: Lock-free memory reclamation primitives.

**Benefits for SECRET_ENGINE**:
- Lock-free job queue improvements
- Safe concurrent data structure access
- Better multi-threaded performance

**Caution**: Only if you need lock-free structures. Current job system may not need this.

**Timeline**: Q4 2026 (low priority)

---

## Implementation Roadmap

### Phase 1: Immediate Actions (NOW - Q1 2026)
- [x] Create C++26 experimental branch
- [ ] **CRITICAL**: Enable `-fhardened` and sanitizers in debug builds (zero code changes)
- [ ] Update build system to support `-std=c++2c` with feature detection
- [ ] Test compiler compatibility (GCC 14+, Clang 19+)
- [ ] **CRITICAL**: Verify Android NDK version and C++26 support timeline
- [ ] Create `#if __cpp_feature` fallback macros for all C++26 features
- [ ] Document dual-path strategy (C++26 + C++23 fallback)
- [ ] Set up CI with C++26 builds

### Phase 2: Low-Risk Features (Q2 2026)
- [ ] Integrate `std::optional<T&>` and range support
- [ ] Adopt `std::inplace_vector<T, N>` for ECS and job queues (HIGH PRIORITY)
- [ ] Use pack indexing for component queries
- [ ] Add `std::mdspan` to renderer (already available)
- [ ] Add `std::debugging` to debug plugin
- [ ] **Define contract violation handler strategy**
- [ ] Begin adding contracts to public APIs (after strategy defined)

### Phase 3: Design and Shadow Implementation (Q3 2026)
- [ ] **std::execution design phase**: Map error domains to sender chains
- [ ] Shadow implementation of std::execution alongside existing job system
- [ ] Implement static reflection for simple components (with C++23 fallback)
- [ ] Add SIMD to math library (with scalar fallback)
- [ ] Profile SIMD performance improvements
- [ ] Test all features on Android NDK

### Phase 4: Validation and Optimization (Q4 2026)
- [ ] Prove std::execution behavioral parity on all platforms
- [ ] Full static reflection for serialization (with fallback)
- [ ] SIMD physics and rendering optimizations
- [ ] Advanced async patterns with senders/receivers
- [ ] Comprehensive performance profiling
- [ ] Document migration patterns

### Phase 5: Production Cutover (Q1 2027)
- [ ] Cut over to std::execution after validation
- [ ] Remove shadow implementations
- [ ] Final performance optimization pass
- [ ] Update all documentation
- [ ] Team training on C++26 patterns

---

## Compiler Support Matrix

| Feature | GCC 14 | GCC 15 | Clang 19 | Clang 20+ | MSVC 2026 |
|---------|--------|--------|----------|-----------|-----------|
| Static Reflection | Partial | Yes | Partial | Yes | TBD |
| Contracts | Partial | Yes | Partial | Yes | TBD |
| std::execution | Partial | Yes | Yes | Yes | TBD |
| std::simd | Yes | Yes | Yes | Yes | Partial |
| optional<T&> | Yes | Yes | Yes | Yes | Yes |
| Memory Safety | Yes | Yes | Yes | Yes | Partial |

**Recommendation**: Target GCC 15 and Clang 20+ for full C++26 support.

---

## Resources and References

### Official Proposals
- [P2996R8 - Static Reflection](https://isocpp.org/files/papers/P2996R8.html)
- [P2300 - std::execution](https://github.com/NVIDIA/stdexec)
- [P2900 - Contracts](https://open-std.org/jtc1/sc22/wg21/docs/papers/2025/)
- [P2988R12 - optional<T&>](https://github.com/bemanproject/optional)
- [P3168R2 - optional Range Support](https://github.com/bemanproject/optional)

### Learning Resources
- [Modern C++ Programming Course](https://github.com/federico-busato/Modern-CPP-Programming) - C++26 coverage
- [ModernesCpp Blog](https://www.modernescpp.com) - C++26 feature articles
- [InfoQ C++26 Overview](https://www.infoq.com/news/2025/06/cpp-26-feature-complete/)

### Implementation Libraries
- [beman.optional](https://github.com/bemanproject/optional) - Early std::optional extensions
- [NVIDIA stdexec](https://github.com/NVIDIA/stdexec) - Reference std::execution implementation

---

## Risk Assessment

### Low Risk ✅
- `std::optional<T&>` - Drop-in replacement, available now
- Memory safety flags (`-fhardened`) - Compiler-level, zero code changes
- Range-based optional - Syntactic sugar
- `std::inplace_vector<T, N>` - Stack allocation, std::vector API
- Pack indexing - Simplifies template code
- `std::debugging` - Simple utility functions

### Medium Risk ⚠️
- **Contracts** - Violation handler strategy must be defined first, build config complexity
- **SIMD** - Platform-specific testing needed, requires scalar fallbacks
- **Static reflection** - Large refactoring, requires C++23 fallback for Android
- **std::mdspan** - API changes for buffer access patterns

### High Risk ⛔
- **std::execution** - 6-9 month design + shadow implementation required
  - Changes entire error propagation model
  - Vulkan integration complexity
  - Must prove parity before cutover
  - **DO NOT attempt single-step rewrite**
- **Full static reflection** - Affects serialization, plugins, tools, editor
  - Android NDK may not support until 2027
  - Requires dual-path maintenance

### Critical Risk 🚨
- **Android NDK Lag** - Clang 20+ not yet in NDK
  - Every feature needs `#if __cpp_feature` fallback
  - May delay production use until 2027
  - Must maintain two code paths for 12-18 months

**Mitigation Strategy**: 
1. Dual-path from day one (C++26 + C++23 fallback)
2. Shadow implementations for high-risk features
3. Extensive testing on all platforms including Android
4. Feature flags for gradual rollout
5. Performance validation before cutover

---

## Performance Expectations

Based on industry benchmarks and proposals:

| Feature | Expected Improvement | Area | Notes |
|---------|---------------------|------|-------|
| std::inplace_vector | 20-50% | ECS queries, job queues | Eliminates heap allocation |
| SIMD Math | 2-8x | Vector/matrix operations | Platform-dependent |
| std::execution | 10-30% | Multi-threaded workloads | After 6-9 month migration |
| Static Reflection | 0% runtime, faster compile | Compile-time only | Reduces boilerplate |
| Contracts | -5% (debug), 0% (release) | Runtime checks | Debug builds only |
| Memory Safety | -2-5% (debug) | Bounds checking | Negligible in release |

**Net Impact**: 
- **Immediate** (std::inplace_vector + SIMD): 25-60% in hot paths
- **Long-term** (std::execution): Additional 10-30% in async workloads
- **Total**: 35-90% performance improvement in CPU-bound scenarios

**Compile-Time Impact**: Static reflection may increase compile times by 10-20%.

---

## Conclusion

C++26 offers significant opportunities for SECRET_ENGINE:

1. **Performance**: SIMD and std::execution provide measurable speedups
2. **Safety**: Contracts and memory safety reduce bugs
3. **Productivity**: Static reflection eliminates boilerplate
4. **Modernization**: Aligns with industry best practices

**Recommended Action**: Begin Phase 1 preparation in Q1 2026, targeting full adoption by Q4 2026.

---

## Next Steps

1. Review this plan with the team
2. Set up C++26 experimental branch
3. Test compiler compatibility on target platforms
4. Prioritize features based on project needs
5. Create detailed implementation specs for each feature

---

*Document Version: 1.0*  
*Last Updated: March 30, 2026*  
*Author: AI Assistant*  
*Status: Draft - Awaiting Review*
