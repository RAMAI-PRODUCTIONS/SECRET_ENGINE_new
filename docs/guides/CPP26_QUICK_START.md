# C++26 Quick Start Guide for SECRET_ENGINE Developers

## Compiler Setup

### GCC 14+ (Recommended for Android NDK)
```bash
# Enable C++26 mode
g++ -std=c++2c -fconcepts-diagnostics-depth=2 source.cpp

# With hardening
g++ -std=c++2c -fhardened source.cpp
```

### Clang 19+
```bash
# Enable C++26 mode
clang++ -std=c++2c source.cpp

# With sanitizers
clang++ -std=c++2c -fsanitize=address,undefined source.cpp
```

### CMake Configuration
```cmake
# Add to CMakeLists.txt
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Feature detection
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-std=c++2c)
endif()
```

---

## Feature Quick Reference

### 1. Static Reflection - Component Auto-Registration

**Before (C++20)**:
```cpp
// Manual registration required
class TransformComponent {
    Vec3 position;
    Quat rotation;
    Vec3 scale;
};

REGISTER_COMPONENT(TransformComponent)
REGISTER_FIELD(TransformComponent, position)
REGISTER_FIELD(TransformComponent, rotation)
REGISTER_FIELD(TransformComponent, scale)
```

**After (C++26)**:
```cpp
// Automatic reflection
class TransformComponent {
    Vec3 position;
    Quat rotation;
    Vec3 scale;
    
    // Auto-generated serialization
    static constexpr auto reflect() {
        return std::meta::members_of(^TransformComponent);
    }
};

// Usage
template<typename T>
void SerializeComponent(const T& component, JsonWriter& writer) {
    constexpr auto members = T::reflect();
    for (auto member : members) {
        writer.Write(member.name(), member.get(component));
    }
}
```

---

### 2. Contracts - Safe APIs

**Before (C++20)**:
```cpp
void SetVelocity(const Vec3& v) {
    assert(v.Length() < MAX_VELOCITY && "Velocity too high!");
    velocity = v;
}
```

**After (C++26)**:
```cpp
void SetVelocity(const Vec3& v)
    pre(v.Length() < MAX_VELOCITY)
    post(velocity == v)
{
    velocity = v;
}

// Compile with different levels:
// -fcontracts=off      - No checking (shipping)
// -fcontracts=audit    - Important checks only
// -fcontracts=all      - Full checking (debug)
```

---

### 3. std::execution - Modern Async

**Before (C++20)**:
```cpp
// Custom job system
jobSystem->Submit([this]() {
    LoadAsset("model.glb");
});

jobSystem->Submit([this]() {
    LoadAsset("texture.png");
});

jobSystem->WaitAll();
```

**After (C++26)**:
```cpp
// Structured concurrency
auto load_model = std::execution::schedule(io_scheduler)
    | std::execution::then([]{ return LoadAsset("model.glb"); });

auto load_texture = std::execution::schedule(io_scheduler)
    | std::execution::then([]{ return LoadAsset("texture.png"); });

auto both = std::execution::when_all(load_model, load_texture);
std::this_thread::sync_wait(both);
```

---

### 4. SIMD Math Operations

**Before (C++20)**:
```cpp
// Scalar operations
void TransformVertices(Vec3* vertices, size_t count, const Mat4& matrix) {
    for (size_t i = 0; i < count; ++i) {
        vertices[i] = matrix * vertices[i];
    }
}
```

**After (C++26)**:
```cpp
// SIMD operations
void TransformVertices(Vec3* vertices, size_t count, const Mat4& matrix) {
    using simd_float4 = std::simd<float, 4>;
    
    for (size_t i = 0; i < count; i += 4) {
        simd_float4 x(&vertices[i].x, std::simd_flag_aligned);
        simd_float4 y(&vertices[i].y, std::simd_flag_aligned);
        simd_float4 z(&vertices[i].z, std::simd_flag_aligned);
        
        // SIMD matrix multiply
        auto rx = matrix[0][0] * x + matrix[1][0] * y + matrix[2][0] * z + matrix[3][0];
        auto ry = matrix[0][1] * x + matrix[1][1] * y + matrix[2][1] * z + matrix[3][1];
        auto rz = matrix[0][2] * x + matrix[1][2] * y + matrix[2][2] * z + matrix[3][2];
        
        rx.copy_to(&vertices[i].x);
        ry.copy_to(&vertices[i].y);
        rz.copy_to(&vertices[i].z);
    }
}
```

---

### 5. std::optional<T&> - Cleaner Lookups

**Before (C++20)**:
```cpp
Entity* FindEntity(EntityID id) {
    auto it = entities.find(id);
    return it != entities.end() ? &it->second : nullptr;
}

// Usage
if (auto* entity = FindEntity(id)) {
    entity->Update();
}
```

**After (C++26)**:
```cpp
std::optional<Entity&> FindEntity(EntityID id) {
    auto it = entities.find(id);
    return it != entities.end() 
        ? std::optional<Entity&>{it->second} 
        : std::nullopt;
}

// Usage - cleaner semantics
if (auto entity = FindEntity(id)) {
    entity->Update();
}

// Or with ranges
for (auto& entity : FindEntity(id)) {
    entity.Update();
}
```

---

## Common Patterns

### Pattern 1: Async Asset Loading with std::execution

```cpp
class AssetLoader {
    std::execution::static_thread_pool io_pool{4};
    
public:
    auto LoadMeshAsync(const std::string& path) {
        return std::execution::schedule(io_pool.get_scheduler())
            | std::execution::then([path]{ 
                return ParseGLTF(path); 
            })
            | std::execution::then([this](MeshData data) {
                return UploadToGPU(data);
            });
    }
    
    void LoadLevel(const std::string& levelPath) {
        auto meshes = LoadMeshAsync("level/meshes.glb");
        auto textures = LoadMeshAsync("level/textures.ktx");
        auto audio = LoadMeshAsync("level/audio.ogg");
        
        // Wait for all
        auto all = std::execution::when_all(meshes, textures, audio);
        auto [m, t, a] = std::this_thread::sync_wait(all).value();
    }
};
```

### Pattern 2: Component Reflection for Serialization

```cpp
template<typename Component>
void SaveComponent(const Component& comp, JsonWriter& json) {
    json.StartObject();
    
    // Reflect over all members
    constexpr auto members = std::meta::members_of(^Component);
    
    for (auto member : members) {
        const char* name = std::meta::name_of(member);
        auto value = member.get(comp);
        json.Write(name, value);
    }
    
    json.EndObject();
}

// Works for any component automatically!
SaveComponent(transformComp, writer);
SaveComponent(meshComp, writer);
```

### Pattern 3: SIMD Frustum Culling

```cpp
struct FrustumCuller {
    using simd_float = std::simd<float, 8>;
    
    void CullObjects(const Frustum& frustum, 
                     const BoundingSphere* spheres, 
                     bool* visible, 
                     size_t count) {
        for (size_t i = 0; i < count; i += 8) {
            simd_float x(&spheres[i].center.x);
            simd_float y(&spheres[i].center.y);
            simd_float z(&spheres[i].center.z);
            simd_float r(&spheres[i].radius);
            
            // Test against all 6 planes simultaneously
            auto inside = TestFrustumSIMD(frustum, x, y, z, r);
            inside.copy_to(&visible[i]);
        }
    }
};
```

---

## Migration Checklist

### Phase 1: Immediate (No Code Changes)
- [ ] Update compiler to GCC 14+ or Clang 19+
- [ ] Add `-std=c++2c` to build flags
- [ ] Enable `-fhardened` for debug builds
- [ ] Test existing code compiles

### Phase 2: Low-Hanging Fruit
- [ ] Replace `Entity*` returns with `std::optional<Entity&>`
- [ ] Add contracts to public APIs
- [ ] Use range-based for with optionals
- [ ] Enable memory safety warnings

### Phase 3: Performance Wins
- [ ] Add SIMD to math library (`Math.h`)
- [ ] Vectorize culling code (`MegaGeometryRenderer.cpp`)
- [ ] SIMD physics broadphase (`PhysicsPlugin.cpp`)

### Phase 4: Architecture Improvements
- [ ] Migrate job system to `std::execution`
- [ ] Add static reflection to components
- [ ] Auto-generate serialization code
- [ ] Implement reflection-based editor

---

## Debugging Tips

### Contract Violations
```bash
# Build with contract checking
g++ -std=c++2c -fcontracts=all source.cpp

# Output shows which contract failed:
# contract violation in function 'SetVelocity'
# precondition 'v.Length() < MAX_VELOCITY' failed
```

### SIMD Issues
```cpp
// Check SIMD width at compile time
static_assert(std::simd<float>::size() >= 4, "Need at least SSE");

// Runtime detection
if constexpr (std::simd<float, std::simd_abi::native>::size() == 8) {
    // AVX path
} else {
    // SSE path
}
```

### Reflection Debugging
```cpp
// Print all members of a type
template<typename T>
void DebugPrintType() {
    constexpr auto members = std::meta::members_of(^T);
    for (auto member : members) {
        std::cout << std::meta::name_of(member) << "\n";
    }
}
```

---

## Performance Profiling

### Measure SIMD Impact
```cpp
#include <chrono>

// Benchmark scalar vs SIMD
auto start = std::chrono::high_resolution_clock::now();
TransformVerticesScalar(vertices, count, matrix);
auto scalar_time = std::chrono::high_resolution_clock::now() - start;

start = std::chrono::high_resolution_clock::now();
TransformVerticesSIMD(vertices, count, matrix);
auto simd_time = std::chrono::high_resolution_clock::now() - start;

std::cout << "Speedup: " << scalar_time / simd_time << "x\n";
```

### Profile std::execution
```cpp
// Use execution policies with profiling
auto work = std::execution::schedule(scheduler)
    | std::execution::then([](){ 
        PROFILE_SCOPE("AsyncWork");
        return DoWork(); 
    });
```

---

## Common Pitfalls

### ❌ Don't: Mix old and new optional
```cpp
// Bad - inconsistent API
Entity* FindEntityOld(EntityID id);
std::optional<Entity&> FindEntityNew(EntityID id);
```

### ✅ Do: Consistent optional usage
```cpp
// Good - uniform API
std::optional<Entity&> FindEntity(EntityID id);
std::optional<Component&> GetComponent(EntityID id);
```

### ❌ Don't: Overuse contracts in hot paths
```cpp
// Bad - contract check every frame
void Update(float dt) 
    pre(dt > 0.0f && dt < 1.0f)  // Too expensive!
{
    // ...
}
```

### ✅ Do: Use contracts for API boundaries
```cpp
// Good - check at API entry
void SetPhysicsTimestep(float dt)
    pre(dt > 0.0f && dt < 1.0f)
{
    timestep = dt;
}
```

---

## Resources

- [C++26 Compiler Support](https://en.cppreference.com/w/cpp/compiler_support/26)
- [Modern C++ Course](https://github.com/federico-busato/Modern-CPP-Programming)
- [std::execution Reference](https://github.com/NVIDIA/stdexec)
- [beman.optional Examples](https://github.com/bemanproject/optional/tree/main/examples)

---

*Last Updated: March 30, 2026*
