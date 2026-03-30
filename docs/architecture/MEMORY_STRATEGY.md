SecretEngine – Memory Strategy (FROZEN)

Purpose

This document defines how memory is allocated, owned, and freed in SecretEngine.
These rules are absolute and apply to ALL code (human and LLM).

## 0. Memory Philosophy

Memory is:
- Explicitly managed
- Never hidden
- Predictable
- Measurable

If allocation cost is unknown → it's wrong.

## 1. Forbidden Patterns (Absolute)

❌ `new` / `delete` in hot paths
❌ `std::shared_ptr` in runtime
❌ `std::vector` growing during gameplay
❌ malloc/free directly
❌ Hidden allocations (string copies, exception handling)
❌ Thread-local storage with allocations
❌ RTTI with dynamic memory

Violation = immediate rejection.

## 2. Allowed Allocators

SecretEngine provides exactly 3 allocator types:

### 2.1 System Allocator
**When to use**: Startup, plugin loading, infrequent operations
**Characteristics**:
- Backed by OS allocator
- Thread-safe
- Slow
- Unbounded

**Example usage**:
```cpp
void* ptr = SystemAllocator::Allocate(size, alignment);
SystemAllocator::Free(ptr);
```

### 2.2 Linear Allocator (Arena)
**When to use**: Per-frame data, temporary data, command buffers
**Characteristics**:
- Bump-pointer allocation
- No individual free
- Reset entire arena
- Fast
- Cache-friendly

**Example usage**:
```cpp
LinearAllocator frame_arena(1MB);

// During frame
void* temp = frame_arena.Allocate(1024);

// End of frame
frame_arena.Reset(); // Free everything
```

### 2.3 Pool Allocator
**When to use**: Fixed-size objects (entities, components, handles)
**Characteristics**:
- Fixed block size
- O(1) allocate/free
- No fragmentation
- Predictable memory usage

**Example usage**:
```cpp
PoolAllocator<Entity> entity_pool(10000);

Entity* e = entity_pool.Allocate();
entity_pool.Free(e);
```

## 3. Allocator Ownership Rules

### Core Layer
- Owns system allocator
- Provides allocator interface to plugins
- Does NOT allocate on plugin's behalf

### Plugins
- Receive allocator interface from core
- Manage their own memory budgets
- Return memory on shutdown

### Game Code
- Uses frame arenas for temporary data
- Uses pools for game objects
- Never calls system allocator directly

## 4. Memory Budgets (Mobile-First)

### Budget Per System (Example - Android Mid-Range)
```
Total Available: ~2GB (after OS)

Core Engine:     50MB
  - Plugin Manager: 5MB
  - Entity Storage: 20MB
  - Command Buffers: 10MB
  - Job System: 5MB
  - Misc: 10MB

Renderer:        200MB
  - Vulkan Objects: 50MB
  - Frame Buffers: 100MB
  - Staging: 50MB

Assets:          500MB
  - Textures: 300MB
  - Meshes: 100MB
  - Audio: 50MB
  - Misc: 50MB

Game Logic:      100MB
  - Entities: 50MB
  - Gameplay: 50MB

Reserved:        150MB
  - OS Thrashing Buffer
```

Budgets are enforced at runtime in debug builds.

## 5. Allocation Tracking (Debug Only)

Debug builds track:
- Total bytes allocated
- Peak memory usage
- Allocation call stacks
- Leaked memory

Release builds: zero tracking overhead.

## 6. Handle System (Critical)

Instead of raw pointers, use handles:

```cpp
struct MeshHandle {
    uint32_t index;
    uint32_t generation;
};
```

Benefits:
- No dangling pointers
- Serializable
- Cache-friendly
- Generation catches use-after-free

All long-lived references use handles, not pointers.

## 7. Lifetime Rules

### Explicit Ownership
Every allocation must answer:
- Who owns this?
- When is it freed?
- What allocator was used?

### Common Patterns

**Pattern 1: Core Owns, Plugin Uses**
```cpp
// Core creates
Entity* e = core->CreateEntity();

// Plugin queries
for (Entity* e : core->QueryEntities(...)) {
    // use e, but don't store pointer
}

// Core destroys
core->DestroyEntity(e);
```

**Pattern 2: Plugin Owns Internally**
```cpp
class RendererPlugin {
    PoolAllocator<RenderCommand> cmd_pool;
    
    RenderCommand* AllocateCommand() {
        return cmd_pool.Allocate();
    }
};
```

**Pattern 3: Frame-Scoped Temporary**
```cpp
void Update(LinearAllocator& frame_arena) {
    void* temp = frame_arena.Allocate(1024);
    // use temp
    // automatically freed at frame end
}
```

## 8. Asset Memory Management

Assets are reference-counted handles:

```cpp
MeshHandle mesh = asset_system->Load("mesh.bin");
// refcount = 1

MeshHandle copy = mesh;
// refcount = 2

asset_system->Release(mesh);
// refcount = 1

asset_system->Release(copy);
// refcount = 0 → unload
```

Reference counting happens in asset system, not per asset.

## 9. GPU Memory Strategy

### Vulkan Memory Types
SecretEngine uses:
- Device-local (GPU only): textures, vertex buffers
- Host-visible (CPU→GPU): staging, uniforms
- Host-cached (GPU→CPU): readback

### Allocation Strategy
- Large heaps (256MB+) allocated at startup
- Sub-allocate from heaps
- No per-object VkDeviceMemory
- Use VMA (Vulkan Memory Allocator) or equivalent

### Streaming
- Textures stream in LODs
- Meshes loaded on-demand
- Shaders pre-compiled and loaded at startup

## 10. String Memory Rules

### Strings Are Expensive
Avoid:
- std::string in runtime
- String concatenation
- Dynamic string formatting

### Allowed String Usage
```cpp
// Compile-time strings (zero cost)
constexpr const char* NAME = "MySystem";

// String views (no allocation)
std::string_view name = entity->GetName();

// String interning (startup only)
StringID id = StringTable::Intern("ComponentType");
```

Runtime uses StringIDs, not strings.

## 11. Container Rules

### Forbidden in Hot Paths
❌ `std::vector` growing
❌ `std::map` (heap per node)
❌ `std::unordered_map` (heap per bucket)

### Allowed
✅ Fixed-size arrays
✅ Pre-allocated std::vector (reserved capacity)
✅ Flat hash maps (single allocation)
✅ Ring buffers

### Container Initialization
```cpp
// Startup
std::vector<Entity> entities;
entities.reserve(10000); // ONE allocation

// Runtime
entities.push_back(e); // No allocation if within capacity
```

## 12. Alignment Requirements

All allocations must respect alignment:

```cpp
struct Allocator {
    void* Allocate(size_t size, size_t alignment);
};
```

Default alignment: 16 bytes (SIMD-safe)

Components with SIMD data:
```cpp
struct alignas(16) TransformComponent {
    Vec4 position; // naturally aligned
};
```

## 13. Memory Barriers & Thread Safety

### Lock-Free Allocators
- Per-thread arenas (no locks)
- Pool allocators use atomic freelist

### Shared Allocators
- System allocator is thread-safe
- Plugins may use locks for their own allocators

Core does not enforce threading model on plugins.

## 14. Memory Leak Detection

Debug mode:
```cpp
MemoryTracker::EnableTracking();

// ... engine runs ...

MemoryTracker::ReportLeaks(); // On shutdown
```

Release mode: disabled.

## 15. Platform Differences

### Android
- Smaller budgets
- More aggressive pooling
- Streaming required

### Windows
- Larger budgets
- Can be less careful
- Still must follow rules (for mobile parity)

Code written for mobile works everywhere.

## 16. Allocator Interface (Core)

Core provides this to plugins:

```cpp
struct IAllocator {
    virtual void* Allocate(size_t size, size_t alignment) = 0;
    virtual void Free(void* ptr) = 0;
};

struct ILinearAllocator : IAllocator {
    virtual void Reset() = 0;
};

struct IPoolAllocator : IAllocator {
    virtual size_t GetBlockSize() const = 0;
};
```

Plugins receive allocators, not create them.

## 17. Initialization Memory

Startup memory is separate from runtime memory:

```
Startup Phase:
  - Can use system allocator freely
  - Can allocate large temporary buffers
  - Can parse files, build tables

Runtime Phase:
  - System allocator forbidden in hot paths
  - All temporary data from frame arenas
  - All persistent data from pools
```

Transition happens after first scene loads.

## 18. Out-of-Memory Handling

If allocation fails:
1. Log error
2. Attempt graceful degradation (drop LODs, unload assets)
3. If critical allocation fails → assert in debug, crash in release

No silent failures. No exceptions.

## 19. Memory Profiling

Tools:
- Tracy (frame profiler)
- Custom allocator wrapper
- Vulkan memory stats

Metrics:
- Bytes allocated per frame
- Peak memory usage
- Fragmentation
- Allocation hot spots

## 20. Common Mistakes to Avoid

❌ "I'll optimize memory later" → No. Design for it now.
❌ Using std::string in component → Use StringID or char[N]
❌ Growing vectors in game loop → Pre-allocate
❌ Shared_ptr for performance → Use handles
❌ "Just 1 allocation won't hurt" → Death by 1000 cuts

## 21. LLM-Specific Rules

When LLM generates code:

1. Must explicitly state which allocator is used
2. Must show deallocation
3. Must annotate lifetime
4. No hidden allocations via STL

Example LLM output (correct):
```cpp
// Using frame arena for temporary data
void ProcessEntities(ILinearAllocator& frame_arena) {
    void* temp = frame_arena.Allocate(1024, 16);
    // ... use temp ...
    // (freed automatically at frame end)
}
```

Status

✅ FROZEN
All code must follow these rules. No exceptions.
