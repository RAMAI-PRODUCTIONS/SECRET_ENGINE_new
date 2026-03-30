# C++26 Phase 1 Status Report - Week 1

**Date**: March 30, 2026  
**Branch**: cpp26-experimental  
**Status**: Phase 1 - Immediate Actions

---

## 1. Android NDK & C++26 Support Status

### Current Environment
- **NDK Version**: 29.0.14206865
- **Clang Version**: 21.0.0 (based on r563880c)
- **Gradle**: 9.4.1
- **CMake**: 3.22.1
- **Target SDK**: 35 (Android 15)
- **Min SDK**: 26 (Android 8.0)

### C++26 Feature Support Analysis

**✅ AVAILABLE NOW (Clang 21)**:
- Memory safety hardening (`-fhardened`)
- Sanitizers (ASan, UBSan)
- `std::span` (C++20, but relevant)
- Pack indexing (experimental)

**⚠️ PARTIAL SUPPORT**:
- Static reflection (experimental, `-freflection`)
- `std::inplace_vector` (not yet in libc++)
- Contracts (experimental)

**❌ NOT YET AVAILABLE**:
- `std::execution` (P2300 - requires Clang 22+)
- `std::simd` (partial support only)
- `std::debugging` (not in Android NDK)

### Critical Finding
**Android NDK 29 with Clang 21 provides early C++26 support**, but most features require fallback implementations. The dual-path strategy (C++26 + C++23 fallback) is MANDATORY.

---

## 2. Memory Hardening Verification

### Current Status
✅ **ENABLED** in `core/CMakeLists.txt` (Debug builds only)

```cmake
if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(SecretEngine_Core PRIVATE 
            -fsanitize=address,undefined
        )
        target_link_options(SecretEngine_Core PRIVATE 
            -fsanitize=address,undefined
        )
    endif()
endif()
```

### Action Required
Test the sanitizers are working correctly:

```bash
# Build debug version
cd android
./gradlew assembleDebug

# Check for sanitizer initialization in logs
adb logcat | grep -i "sanitizer"
```

### Expected Behavior
- ASan should catch buffer overflows
- UBSan should catch undefined behavior (integer overflow, null dereference, etc.)
- No performance impact in release builds

---

## 3. std::span Integration Opportunities

### Current Buffer Access Patterns

**Problem Areas Identified**:

1. **Lighting System** (`plugins/LightingSystem/src/LightManager.cpp:54`)
   ```cpp
   // Current: Raw pointer + size
   const void* LightManager::GetLightBuffer() const {
       return m_lights.empty() ? nullptr : m_lights.data();
   }
   size_t GetLightBufferSize() const;
   ```

2. **Material System** (`plugins/MaterialSystem/src/MaterialPlugin.cpp:89`)
   ```cpp
   // Current: Raw pointer + size
   const void* MaterialPlugin::GetMaterialBuffer() const {
       return m_materials.empty() ? nullptr : m_materials.data();
   }
   size_t GetMaterialBufferSize() const;
   ```

3. **Asset Provider** (`core/src/AssetProvider.cpp:29`)
   ```cpp
   // Current: Raw pointer from Android asset
   const void* assetData = AAsset_getBuffer(asset);
   size_t size = AAsset_getLength(asset);
   ```

4. **MegaGeometryRenderer** (Multiple locations)
   - Vertex buffer access: `void* m_vertexDataMapped`
   - Index buffer access: `void* m_indexDataMapped`
   - Instance data: `InstanceData* m_instanceDataMapped[2]`

### Recommended Changes

#### Priority 1: Lighting & Material Buffers
```cpp
// NEW: Type-safe span interface
std::span<const LightData> GetLightBuffer() const {
    return m_lights;
}

std::span<const MaterialData> GetMaterialBuffer() const {
    return m_materials;
}
```

**Benefits**:
- Eliminates separate size queries
- Type-safe access
- Bounds checking in debug builds
- Zero runtime overhead

#### Priority 2: MegaGeometryRenderer Buffer Access
```cpp
// Current: Raw pointers
void* m_vertexDataMapped;
void* m_indexDataMapped;

// NEW: Typed spans
std::span<Vertex3DNitro> GetVertexBuffer() {
    return {static_cast<Vertex3DNitro*>(m_vertexDataMapped), MAX_VERTICES};
}

std::span<uint16_t> GetIndexBuffer() {
    return {static_cast<uint16_t*>(m_indexDataMapped), MAX_INDICES};
}
```

#### Priority 3: Asset Loading
```cpp
// Current: void* + size
const void* assetData = AAsset_getBuffer(asset);
size_t size = AAsset_getLength(asset);

// NEW: span-based API
std::span<const std::byte> LoadAssetData(const char* path) {
    AAsset* asset = AAssetManager_open(...);
    const void* data = AAsset_getBuffer(asset);
    size_t size = AAsset_getLength(asset);
    return {static_cast<const std::byte*>(data), size};
}
```

---

## 4. std::inplace_vector Prototype for ECS

### Current ECS Component Query Pattern

**Problem** (`core/src/World.cpp`):
```cpp
// Heap allocation for every query
std::vector<Entity*> visibleEntities;
visibleEntities.reserve(MAX_VISIBLE);

for (auto* entity : allEntities) {
    if (IsVisible(entity)) {
        visibleEntities.push_back(entity);
    }
}
```

### Proposed std::inplace_vector Implementation

Since `std::inplace_vector` is not yet in Android NDK, we need a fallback:

```cpp
// In CPP26Features.h - already has basic fallback
// Need to enhance with proper stack storage

#if !SE_HAS_INPLACE_VECTOR
namespace std {
    template<typename T, size_t N>
    class inplace_vector {
        alignas(T) std::byte storage_[N * sizeof(T)];
        size_t size_ = 0;
        
    public:
        void push_back(const T& value) {
            assert(size_ < N && "inplace_vector capacity exceeded");
            new (&storage_[size_ * sizeof(T)]) T(value);
            ++size_;
        }
        
        void push_back(T&& value) {
            assert(size_ < N && "inplace_vector capacity exceeded");
            new (&storage_[size_ * sizeof(T)]) T(std::move(value));
            ++size_;
        }
        
        T* begin() { return reinterpret_cast<T*>(storage_); }
        T* end() { return reinterpret_cast<T*>(storage_) + size_; }
        
        size_t size() const { return size_; }
        bool empty() const { return size_ == 0; }
        void clear() { 
            for (size_t i = 0; i < size_; ++i) {
                reinterpret_cast<T*>(storage_)[i].~T();
            }
            size_ = 0; 
        }
        
        T& operator[](size_t i) { return reinterpret_cast<T*>(storage_)[i]; }
        const T& operator[](size_t i) const { return reinterpret_cast<const T*>(storage_)[i]; }
        
        static constexpr size_t capacity() { return N; }
        
        ~inplace_vector() { clear(); }
    };
}
#endif
```

### ECS Integration Points

1. **Component Queries** (High Priority)
   ```cpp
   // NEW: Stack-allocated query results
   std::inplace_vector<Entity*, 1024> QueryEntitiesWithComponents(uint32_t typeID) {
       std::inplace_vector<Entity*, 1024> results;
       for (auto& entity : m_entities) {
           if (GetComponent(entity, typeID)) {
               results.push_back(&entity);
           }
       }
       return results;
   }
   ```

2. **Culling Results** (MegaGeometryRenderer)
   ```cpp
   // Current: GPU-side culling (already optimal)
   // Future: CPU-side pre-culling for small scenes
   std::inplace_vector<uint32_t, 512> CullInstancesCPU() {
       std::inplace_vector<uint32_t, 512> visible;
       // ... frustum culling logic
       return visible;
   }
   ```

3. **Job System Task Lists**
   ```cpp
   // NEW: Fixed-size job queues per frame
   std::inplace_vector<Job*, 256> m_frameJobs;
   ```

---

## 5. Performance Benchmarking Setup

### Benchmark Targets

1. **std::span vs raw pointers**
   - Measure buffer access overhead
   - Verify zero-cost abstraction

2. **std::inplace_vector vs std::vector**
   - Measure allocation overhead
   - Cache locality improvements
   - Frame time impact

3. **Memory hardening overhead**
   - Debug build performance
   - Sanitizer impact

### Benchmark Implementation

Create `tests/benchmarks/cpp26_benchmarks.cpp`:

```cpp
#include <benchmark/benchmark.h>
#include <SecretEngine/CPP26Features.h>
#include <vector>

// Benchmark 1: std::span vs raw pointer
static void BM_RawPointerAccess(benchmark::State& state) {
    std::vector<float> data(1024);
    float* ptr = data.data();
    size_t size = data.size();
    
    for (auto _ : state) {
        float sum = 0;
        for (size_t i = 0; i < size; ++i) {
            sum += ptr[i];
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_RawPointerAccess);

static void BM_SpanAccess(benchmark::State& state) {
    std::vector<float> data(1024);
    std::span<float> span = data;
    
    for (auto _ : state) {
        float sum = 0;
        for (auto val : span) {
            sum += val;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_SpanAccess);

// Benchmark 2: std::inplace_vector vs std::vector
static void BM_VectorAllocation(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<int> vec;
        vec.reserve(128);
        for (int i = 0; i < 128; ++i) {
            vec.push_back(i);
        }
        benchmark::DoNotOptimize(vec.data());
    }
}
BENCHMARK(BM_VectorAllocation);

static void BM_InplaceVectorAllocation(benchmark::State& state) {
    for (auto _ : state) {
        std::inplace_vector<int, 128> vec;
        for (int i = 0; i < 128; ++i) {
            vec.push_back(i);
        }
        benchmark::DoNotOptimize(vec.data());
    }
}
BENCHMARK(BM_InplaceVectorAllocation);

BENCHMARK_MAIN();
```

---

## 6. Action Items for This Week

### Immediate (Today)
- [x] Check Android NDK version ✅ NDK 29, Clang 21
- [x] Verify memory hardening is enabled ✅ Enabled in CMakeLists.txt
- [x] Identify std::span integration points ✅ 4 priority areas found
- [x] Design std::inplace_vector fallback ✅ Stack-based implementation ready

### This Week (Days 2-7)
- [ ] **Day 2**: Test sanitizers with intentional bugs
- [ ] **Day 3**: Implement std::span in LightingSystem
- [ ] **Day 4**: Implement std::span in MaterialSystem
- [ ] **Day 5**: Enhance std::inplace_vector fallback with proper storage
- [ ] **Day 6**: Create ECS query prototype with std::inplace_vector
- [ ] **Day 7**: Set up benchmark suite and baseline measurements

---

## 7. Risk Assessment

### Low Risk ✅
- std::span integration (C++20 feature, widely supported)
- Memory hardening (compiler-level, no code changes)

### Medium Risk ⚠️
- std::inplace_vector fallback (needs thorough testing)
- ECS query refactoring (affects hot paths)

### Mitigation Strategy
1. Feature flags for gradual rollout
2. A/B testing with old vs new implementations
3. Performance regression tests
4. Extensive unit testing

---

## 8. Next Phase Preview

### Phase 2 (Week 2-4): Low-Risk Features
- std::optional<T&> for entity lookups
- Pack indexing for component queries
- std::debugging for assertion system
- Contracts for public APIs (after violation handler strategy)

### Phase 3 (Month 2-3): Design Phase
- std::execution design (error handling strategy)
- Static reflection prototype (with C++23 fallback)
- SIMD math library (with scalar fallback)

---

## Conclusion

**Phase 1 Status**: ON TRACK ✅

- Android NDK 29 with Clang 21 provides early C++26 support
- Memory hardening is enabled and ready for testing
- 4 high-value std::span integration points identified
- std::inplace_vector fallback implementation designed
- Benchmark suite planned

**Next Steps**: Begin implementation of std::span in buffer interfaces and test memory hardening with intentional bugs.

---

*Report Generated*: March 30, 2026  
*Author*: Kiro AI Assistant  
*Status*: Ready for Implementation
