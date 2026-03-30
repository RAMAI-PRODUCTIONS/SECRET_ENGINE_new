---
inclusion: auto
priority: 100
---

# C++26 STRICT CODING RULES FOR SECRET_ENGINE

**MANDATORY**: All code written for SECRET_ENGINE MUST follow these C++26 rules.

---

## 🚫 FORBIDDEN PATTERNS

### 1. Raw Pointers for Buffers - BANNED
```cpp
// ❌ FORBIDDEN - Never use raw pointers for buffers
const void* GetBuffer() const;
size_t GetBufferSize() const;

void ProcessData(const void* data, size_t size);
void UploadTexture(const uint8_t* pixels, int width, int height);

// ✅ REQUIRED - Always use std::span
std::span<const std::byte> GetBuffer() const;
void ProcessData(std::span<const std::byte> data);
void UploadTexture(std::span<const std::byte> pixels, int width, int height);
```

### 2. Array Parameters - BANNED
```cpp
// ❌ FORBIDDEN - Never use C-style array parameters
void SetPosition(const float pos[3]);
void SetColor(float rgba[4]);
RaycastHit Raycast(const float origin[3], const float direction[3]);

// ✅ REQUIRED - Always use std::span with compile-time size
void SetPosition(std::span<const float, 3> pos);
void SetColor(std::span<float, 4> rgba);
RaycastHit Raycast(std::span<const float, 3> origin, std::span<const float, 3> direction);
```

### 3. Separate Size Parameters - BANNED
```cpp
// ❌ FORBIDDEN - Never pass size separately
void CopyBuffer(const void* src, size_t srcSize, void* dst, size_t dstSize);
void LoadAsset(const char* path, void* buffer, size_t bufferSize);

// ✅ REQUIRED - Size is part of span
void CopyBuffer(std::span<const std::byte> src, std::span<std::byte> dst);
void LoadAsset(const char* path, std::span<std::byte> buffer);
```

### 4. Heap Allocations in Hot Paths - BANNED
```cpp
// ❌ FORBIDDEN - Never allocate in per-frame code
void Update() {
    std::vector<Entity*> visible;  // HEAP ALLOCATION!
    visible.reserve(1024);
    // ... query entities
}

// ✅ REQUIRED - Use std::inplace_vector for fixed-size collections
void Update() {
    std::inplace_vector<Entity*, 1024> visible;  // STACK ONLY!
    // ... query entities
}
```

### 5. Nullable Pointers for Queries - DISCOURAGED
```cpp
// ⚠️ DISCOURAGED - Pointers for optional returns
Entity* FindEntity(uint32_t id);
const Material* GetMaterial(MaterialHandle handle);

// ✅ PREFERRED - Use std::optional<T&> (when available)
std::optional<Entity&> FindEntity(uint32_t id);
std::optional<const Material&> GetMaterial(MaterialHandle handle);

// ✅ ACCEPTABLE - Pointer with clear nullptr semantics
Entity* FindEntity(uint32_t id);  // Returns nullptr if not found
```

---

## ✅ REQUIRED PATTERNS

### 1. Buffer Interfaces - std::span MANDATORY
```cpp
// Interface definition
class ILightingSystem {
public:
    // C++26: Type-safe buffer access
    virtual std::span<const LightData> GetLightBuffer() const = 0;
    
    // Legacy API (deprecated, for backward compatibility only)
    virtual const void* GetLightBufferRaw() const = 0;
    virtual size_t GetLightBufferSize() const = 0;
};

// Implementation
class LightManager {
    std::vector<LightData> m_lights;
    
public:
    std::span<const LightData> GetLightBuffer() const {
        return m_lights;  // Automatic conversion!
    }
    
    const void* GetLightBufferRaw() const {
        return m_lights.data();
    }
    
    size_t GetLightBufferSize() const {
        return m_lights.size() * sizeof(LightData);
    }
};
```

### 2. Array Parameters - std::span<T, N> MANDATORY
```cpp
// Physics API
class PhysicsSystem {
public:
    // Fixed-size arrays use std::span<T, N>
    RaycastHit Raycast(
        std::span<const float, 3> origin,
        std::span<const float, 3> direction,
        float maxDistance
    );
    
    void SetGravity(std::span<const float, 3> gravity);
    void AddForce(Entity entity, std::span<const float, 3> force);
};

// Implementation
RaycastHit PhysicsSystem::Raycast(
    std::span<const float, 3> origin,
    std::span<const float, 3> direction,
    float maxDistance
) {
    // Access with .data() when calling legacy APIs
    return RaycastInternal(origin.data(), direction.data(), maxDistance);
    
    // Or access elements directly
    float x = origin[0];
    float y = origin[1];
    float z = origin[2];
}
```

### 3. Fixed-Size Collections - std::inplace_vector MANDATORY
```cpp
// ECS component queries
std::inplace_vector<Entity*, 1024> QueryVisibleEntities() {
    std::inplace_vector<Entity*, 1024> results;
    
    for (auto& entity : m_entities) {
        if (IsVisible(entity)) {
            results.push_back(&entity);
        }
    }
    
    return results;  // No heap allocation!
}

// Physics collision detection
void DetectCollisions() {
    std::inplace_vector<CollisionPair, 512> pairs;
    
    // Broadphase
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            if (BroadphaseTest(bodies[i], bodies[j])) {
                pairs.push_back({i, j});
            }
        }
    }
    
    // Narrowphase
    for (auto& pair : pairs) {
        ResolveCollision(pair);
    }
}

// Renderer draw calls
void BuildDrawCalls() {
    std::inplace_vector<DrawCall, 256> drawCalls;
    
    for (auto& mesh : m_meshes) {
        drawCalls.emplace_back(mesh.vertexOffset, mesh.indexCount);
    }
    
    SubmitDrawCalls(drawCalls);
}
```

### 4. Feature Detection - ALWAYS USE GUARDS
```cpp
#include <SecretEngine/CPP26Features.h>

// Check feature availability
#if SE_HAS_INPLACE_VECTOR
    // Use native std::inplace_vector
    std::inplace_vector<int, 100> data;
#else
    // Use fallback implementation
    std::inplace_vector<int, 100> data;  // Falls back to custom impl
#endif

// Contracts
void SetVelocity(const Vec3& v)
    SE_PRECONDITION(v.Length() < MAX_VELOCITY)  // Becomes assert() on Android
{
    velocity = v;
}

// Debugging
void CustomAssert(bool condition) {
    if (!condition) {
        if (SE_IS_DEBUGGER_PRESENT()) {
            SE_BREAKPOINT();
        }
    }
}
```

---

## 📋 CONVERSION CHECKLIST

When writing or modifying code, verify:

- [ ] No `void*` for buffers (use `std::span<const std::byte>`)
- [ ] No `const T*` + `size_t` pairs (use `std::span<const T>`)
- [ ] No array parameters `T[]` or `T[N]` (use `std::span<T, N>`)
- [ ] No `std::vector` in hot paths (use `std::inplace_vector<T, N>`)
- [ ] All buffer interfaces return `std::span`
- [ ] Legacy APIs marked deprecated
- [ ] Feature guards for C++26 features
- [ ] Backward compatibility maintained

---

## 🎯 PERFORMANCE RULES

### 1. Zero-Cost Abstractions ONLY
```cpp
// ✅ GOOD - std::span is zero-cost
void ProcessBuffer(std::span<const float> data) {
    for (float value : data) {  // Same as raw pointer loop
        Process(value);
    }
}

// ✅ GOOD - std::inplace_vector is stack-only
std::inplace_vector<int, 100> GetResults() {
    std::inplace_vector<int, 100> results;
    // ... fill results
    return results;  // No heap, no copy (RVO)
}
```

### 2. Hot Path Optimization
```cpp
// Per-frame code (60 FPS = 16ms budget)
void Update(float dt) {
    // ✅ GOOD - Stack allocation
    std::inplace_vector<Entity*, 1024> visible;
    
    // ✅ GOOD - Span for buffer access
    auto lights = m_lightingSystem->GetLightBuffer();
    
    // ❌ BAD - Heap allocation
    // std::vector<Entity*> visible;  // FORBIDDEN!
}
```

### 3. Compile-Time Size Verification
```cpp
// ✅ GOOD - Compile-time size checking
void SetPosition(std::span<const float, 3> pos) {
    // Compiler verifies size is exactly 3
    m_position[0] = pos[0];
    m_position[1] = pos[1];
    m_position[2] = pos[2];
}

// Calling code
float pos[3] = {1, 2, 3};
SetPosition(pos);  // OK

float pos2[4] = {1, 2, 3, 4};
SetPosition(pos2);  // COMPILE ERROR - size mismatch!
```

---

## 🔧 MIGRATION PATTERNS

### Pattern 1: Buffer Interface Migration
```cpp
// BEFORE
class ISystem {
    virtual const void* GetBuffer() const = 0;
    virtual size_t GetBufferSize() const = 0;
};

// AFTER (maintain both for compatibility)
class ISystem {
    // New API (preferred)
    virtual std::span<const Data> GetBuffer() const = 0;
    
    // Legacy API (deprecated)
    virtual const void* GetBufferRaw() const = 0;
    virtual size_t GetBufferSize() const = 0;
};

// Implementation
class System : public ISystem {
    std::vector<Data> m_data;
    
public:
    std::span<const Data> GetBuffer() const override {
        return m_data;
    }
    
    const void* GetBufferRaw() const override {
        return m_data.data();
    }
    
    size_t GetBufferSize() const override {
        return m_data.size() * sizeof(Data);
    }
};
```

### Pattern 2: Array Parameter Migration
```cpp
// BEFORE
void SetTransform(const float pos[3], const float rot[3], const float scale[3]);

// AFTER
void SetTransform(
    std::span<const float, 3> pos,
    std::span<const float, 3> rot,
    std::span<const float, 3> scale
);

// Implementation
void SetTransform(
    std::span<const float, 3> pos,
    std::span<const float, 3> rot,
    std::span<const float, 3> scale
) {
    // Access with [] or .data()
    m_position[0] = pos[0];
    m_position[1] = pos[1];
    m_position[2] = pos[2];
    
    // Or pass to legacy APIs
    memcpy(m_rotation, rot.data(), sizeof(float) * 3);
}
```

### Pattern 3: Hot Path Allocation Migration
```cpp
// BEFORE
void CullObjects() {
    std::vector<Entity*> visible;
    visible.reserve(1024);
    
    for (auto* entity : m_entities) {
        if (IsVisible(entity)) {
            visible.push_back(entity);
        }
    }
    
    RenderObjects(visible);
}

// AFTER
void CullObjects() {
    std::inplace_vector<Entity*, 1024> visible;
    
    for (auto* entity : m_entities) {
        if (IsVisible(entity)) {
            visible.push_back(entity);
        }
    }
    
    RenderObjects(visible);  // Same API!
}
```

---

## 🚨 ENFORCEMENT

### Compile-Time Checks
- Use `-std=c++26` or `-std=c++2c`
- Enable all warnings: `-Wall -Wextra -Wpedantic`
- Treat warnings as errors: `-Werror`

### Code Review Checklist
1. No raw `void*` for buffers
2. No C-style array parameters
3. No heap allocations in hot paths
4. All spans have correct const-ness
5. Fixed-size spans use `std::span<T, N>`
6. Feature guards present for C++26 features

### Static Analysis
- Run clang-tidy with C++26 checks
- Check for buffer overruns (sanitizers)
- Verify zero-cost abstractions (assembly inspection)

---

## 📚 QUICK REFERENCE

### Include Headers
```cpp
#include <span>                              // std::span
#include <SecretEngine/CPP26Features.h>     // std::inplace_vector, feature detection
```

### Common Conversions
| Old Pattern | New Pattern |
|-------------|-------------|
| `const void* data, size_t size` | `std::span<const std::byte> data` |
| `const float pos[3]` | `std::span<const float, 3> pos` |
| `float* GetBuffer()` | `std::span<float> GetBuffer()` |
| `std::vector<T> temp` | `std::inplace_vector<T, N> temp` |
| `T* Find(...)` | `std::optional<T&> Find(...)` |

### Capacity Guidelines
- **ECS queries**: `std::inplace_vector<Entity*, 1024>`
- **Physics pairs**: `std::inplace_vector<CollisionPair, 512>`
- **Draw calls**: `std::inplace_vector<DrawCall, 256>`
- **UI vertices**: `std::inplace_vector<Vertex2D, 4096>`

---

## ✅ SUMMARY

**ALWAYS**:
- Use `std::span` for all buffer parameters
- Use `std::span<T, N>` for fixed-size arrays
- Use `std::inplace_vector<T, N>` for temporary collections
- Include feature guards for C++26 features
- Maintain legacy APIs for backward compatibility

**NEVER**:
- Use `void*` for buffers
- Use C-style array parameters
- Allocate `std::vector` in hot paths
- Forget const-correctness on spans
- Skip feature detection guards

---

**This is the law. Follow it strictly.**
