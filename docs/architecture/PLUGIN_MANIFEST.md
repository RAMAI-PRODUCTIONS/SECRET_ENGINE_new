SecretEngine – Plugin Manifest & Contracts (FROZEN)

Purpose

This document defines how plugins are discovered, loaded, and interact with core.
These contracts are binding.

## 1. Plugin Definition

A plugin is a shared library (.dll, .so) that implements the IPlugin interface.

```cpp
class IPlugin {
public:
    virtual const char* GetName() const = 0;
    virtual uint32_t GetVersion() const = 0;
    virtual void OnLoad(ICore* core) = 0;
    virtual void OnActivate() = 0;
    virtual void OnDeactivate() = 0;
    virtual void OnUnload() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void* GetInterface(uint32_t id) = 0;
};
```

## 2. Plugin Manifest (JSON)

Every plugin has a manifest file:

**File**: `plugin_manifest.json` (inside plugin folder)

```json
{
  "name": "VulkanRenderer",
  "version": "1.0.0",
  "type": "renderer",
  "library": "VulkanRenderer.dll",
  "dependencies": [],
  "capabilities": ["rendering", "swapchain"],
  "requirements": {
    "vulkan_version": "1.2",
    "min_engine_version": "0.1.0"
  },
  "config": {
    "default_resolution": [1920, 1080],
    "enable_validation": false
  }
}
```

## 3. Plugin Types (Canonical)

Only these plugin types exist:

| Type | Purpose | Singleton? |
|------|---------|-----------|
| renderer | Graphics rendering | Yes |
| input | Input handling | Yes |
| physics | Physics simulation | Yes |
| navigation | Pathfinding | Yes |
| audio | Sound playback | Yes |
| networking | Multiplayer | Yes |
| ui | User interface | Yes |
| animation | Animation system | Yes |
| debug | Debug tools (Stats, Overlay) | No |
| game | Game logic module | No |

Singleton plugins: only one active at a time.
Non-singleton: multiple allowed.

## 4. Plugin Discovery

### Discovery Process
1. Core scans `plugins/` folder
2. Reads all `plugin_manifest.json` files
3. Validates manifest schema
4. Builds plugin registry
5. Resolves dependencies

### Plugin Folder Structure
```
plugins/
├── VulkanRenderer/
│   ├── plugin_manifest.json
│   ├── VulkanRenderer.dll
│   └── shaders/
├── GameLogic/
│   ├── src/
│   │   ├── LogicPlugin.cpp
│   │   └── LogicPlugin.h
│   └── CMakeLists.txt
├── AndroidInput/
│   ├── plugin_manifest.json
│   └── AndroidInput.so
├── DebugPlugin/
│   ├── src/
│   │   ├── DebugPlugin.cpp
│   │   └── DebugPlugin.h
│   └── CMakeLists.txt
└── PhysX/
    ├── plugin_manifest.json
    └── PhysX.dll
```

## 5. Plugin Loading (Detailed)

### Phase 1: Scan
- Enumerate plugin folders
- Parse manifests
- Validate versions

### Phase 2: Load Libraries
- Load shared libraries
- Resolve `CreatePlugin()` symbol
- Call `CreatePlugin()` → returns `IPlugin*`

### Phase 3: Register
- Call `IPlugin::OnLoad(core)`
- Plugin registers capabilities
- Plugin receives allocator, logger

### Phase 4: Activate
- Call `IPlugin::OnActivate()`
- Plugin initializes systems
- Plugin becomes active

### Phase 5: Runtime
- Plugin processes queries
- Plugin responds to events

### Phase 6: Deactivate
- Call `IPlugin::OnDeactivate()`
- Plugin stops processing
- Resources remain allocated

### Phase 7: Unload
- Call `IPlugin::OnUnload()`
- Plugin frees all resources
- Library is unloaded

## 6. Plugin Entry Point

Every plugin DLL exports:

```cpp
extern "C" {
    PLUGIN_API IPlugin* CreatePlugin();
    PLUGIN_API void DestroyPlugin(IPlugin* plugin);
}
```

Core calls `CreatePlugin()` to instantiate plugin.

## 7. Capability System

Plugins declare what they provide:

```cpp
void RendererPlugin::OnLoad(ICore* core) {
    core->RegisterCapability("rendering", this);
    core->RegisterCapability("swapchain", this);
}
```

Other systems query capabilities:

```cpp
IPlugin* renderer = core->GetCapability("rendering");
```

## 8. Dependency Resolution

### Dependency Rules
- Plugins may depend on capabilities, not plugins
- Dependencies are loaded before dependents
- Circular dependencies are forbidden
- Missing dependencies = load failure

### Example Manifest with Dependencies
```json
{
  "name": "PhysicsDebugRenderer",
  "dependencies": ["rendering", "physics"]
}
```

Core guarantees:
- Renderer and Physics are loaded before PhysicsDebugRenderer
- If either is missing, PhysicsDebugRenderer is not loaded

## 9. Plugin Configuration

### Engine Config (JSON)
Defines which plugins to load:

**File**: `engine_config.json`

```json
{
  "plugins": {
    "renderer": "VulkanRenderer",
    "input": "AndroidInput",
    "physics": "PhysX",
    "audio": "FMODAudio"
  },
  "plugin_config": {
    "VulkanRenderer": {
      "enable_validation": true,
      "prefer_integrated_gpu": false
    }
  }
}
```

### Plugin-Specific Config
Each plugin receives its config section:

```cpp
void RendererPlugin::OnLoad(ICore* core) {
    const ConfigNode* config = core->GetPluginConfig("VulkanRenderer");
    bool validation = config->GetBool("enable_validation", false);
}
```

## 10. Hot Reload Rules

### Allowed to Reload
- Debug plugins
- Game plugins
- Audio plugins

### NOT Allowed to Reload
- Renderer (requires swapchain recreation)
- Physics (simulation state)
- Core plugins

Hot reload process:
1. Call `OnDeactivate()`
2. Call `OnUnload()`
3. Unload library
4. Reload library
5. Call `OnLoad()`
6. Call `OnActivate()`

## 11. Plugin Communication

### Rule: NO DIRECT PLUGIN-TO-PLUGIN CALLS

Communication happens via:

#### Method 1: Core-Mediated Queries
```cpp
// Physics plugin wants render debug data
void PhysicsPlugin::Update() {
    IDebugRenderer* debug = core->GetCapability<IDebugRenderer>("debug_render");
    if (debug) {
        debug->DrawLine(start, end, color);
    }
}
```

#### Method 2: Event System
```cpp
// Input plugin fires event
core->PostEvent("input.key_pressed", key_data);

// Other plugins subscribe
core->SubscribeEvent("input.key_pressed", this, &MyPlugin::OnKeyPressed);
```

#### Method 3: Shared Data (Read-Only)
```cpp
// Renderer publishes camera
core->SetSharedData("camera", &camera_data);

// Physics reads camera
const CameraData* cam = core->GetSharedData<CameraData>("camera");
```

#### Method 4: Fast Data Architecture (FDA) Stream
For high-frequency cross-thread communication:
```cpp
// 1. Get the 1024-slot command stream from the renderer
auto& stream = renderer->GetCommandStream();

// 2. Push 8-byte UltraPacket (sub-100ns)
UltraPacket p;
p.Set(PacketType::RenderCommand, CMD_ID, data, 0);
stream.Push(p);
```
**Mandatory for Input, Render Commands, and Physics Sync.**

## 12. Plugin Lifecycle Hooks (Detailed)

### OnLoad(ICore* core)
**When**: Plugin library is loaded
**Purpose**: Register capabilities, query dependencies
**Rules**:
- Must not allocate large buffers
- Must not create GPU resources
- Must not assume other plugins exist

### OnActivate()
**When**: Plugin is selected as active (per config)
**Purpose**: Initialize systems, allocate resources
**Rules**:
- Can create GPU resources
- Can allocate memory
- Can assume dependencies are loaded

### OnDeactivate()
**When**: Plugin is swapped out (hot reload, config change)
**Purpose**: Stop processing, but don't free resources
**Rules**:
- Must stop all threads
- Must flush pending work
- Must not destroy resources (OnUnload does this)

### OnUnload()
**When**: Plugin is being removed
**Purpose**: Free all resources, cleanup
**Rules**:
- Must free all allocations
- Must destroy GPU resources
- Must leave no global state

## 13. Plugin Versioning

### Semantic Versioning
```
MAJOR.MINOR.PATCH

1.0.0 → Initial
1.1.0 → New feature (backward compatible)
1.1.1 → Bug fix
2.0.0 → Breaking change
```

### Compatibility Rules
- Plugins with same MAJOR version are compatible
- Engine checks MAJOR version on load
- Mismatched MAJOR = load failure

## 14. Error Handling

Plugins must handle errors explicitly:

```cpp
enum class PluginResult {
    Success,
    NotInitialized,
    InvalidParameter,
    OutOfMemory,
    NotSupported
};

PluginResult RendererPlugin::Initialize(const Config& config) {
    if (!config.IsValid()) {
        return PluginResult::InvalidParameter;
    }
    
    if (!VulkanAvailable()) {
        return PluginResult::NotSupported;
    }
    
    return PluginResult::Success;
}
```

No exceptions. Return codes only.

## 15. Logging from Plugins

Plugins use core logger:

```cpp
void RendererPlugin::OnLoad(ICore* core) {
    logger_ = core->GetLogger();
}

void RendererPlugin::Render() {
    logger_->LogInfo("VulkanRenderer", "Frame rendered");
}
```

No direct stdout/stderr. No printf.

## 16. Memory from Core

Plugins receive allocators from core:

```cpp
void RendererPlugin::OnLoad(ICore* core) {
    allocator_ = core->GetAllocator("renderer");
    frame_arena_ = core->GetFrameArena();
}

void* RendererPlugin::Allocate(size_t size) {
    return allocator_->Allocate(size, 16);
}
```

Plugins never call `malloc` or `new` directly.

## 17. Thread Safety

### Core Guarantees
- `OnLoad`, `OnActivate`, `OnDeactivate`, `OnUnload` are single-threaded
- Event callbacks are single-threaded
- Queries may be multi-threaded (plugin decides)

### Plugin Responsibilities
- If plugin spawns threads, plugin manages them
- Plugin must synchronize internal state
- Plugin must not assume main thread context

## 18. Platform-Specific Plugins

Plugins may be platform-specific:

```
plugins/
├── VulkanRenderer/     (Windows, Android)
├── AndroidInput/       (Android only)
├── WindowsInput/       (Windows only)
└── FMODAudio/          (All platforms)
```

Engine config selects platform:

```json
{
  "plugins": {
    "input": "${platform}.Input"  // resolves to AndroidInput or WindowsInput
  }
}
```

## 19. Debug vs Release Plugins

Plugins may have debug/release variants:

```
plugins/VulkanRenderer/
├── VulkanRenderer_Debug.dll
├── VulkanRenderer_Release.dll
└── plugin_manifest.json
```

Manifest specifies:
```json
{
  "library": {
    "debug": "VulkanRenderer_Debug.dll",
    "release": "VulkanRenderer_Release.dll"
  }
}
```

## 20. Plugin Testing

Every plugin must provide:

```cpp
class IPlugin {
    virtual bool SelfTest() = 0;
};
```

Self-test:
- Verifies plugin can initialize
- Checks dependencies
- Validates configuration
- Returns true/false

Run at startup in debug builds.

## 21. Plugin Manifest Validation

Core validates manifests:

```cpp
struct ManifestValidator {
    bool ValidateName(const std::string& name);
    bool ValidateVersion(const std::string& version);
    bool ValidateType(const std::string& type);
    bool ValidateDependencies(const std::vector<std::string>& deps);
};
```

Invalid manifest = plugin not loaded.

## 22. Forbidden Plugin Behaviors

❌ Plugins calling other plugins directly
❌ Plugins modifying core state
❌ Plugins assuming load order
❌ Plugins writing to global variables
❌ Plugins creating threads without cleanup
❌ Plugins keeping state between load/unload
❌ Plugins using C++ exceptions
❌ Plugins using RTTI

## 23. Plugin Development Checklist

Before submitting a plugin:

- [ ] Manifest is valid JSON
- [ ] Entry points are exported
- [ ] Dependencies are declared
- [ ] Lifecycle hooks are implemented
- [ ] Resources are freed in OnUnload
- [ ] No direct plugin-to-plugin calls
- [ ] Logging uses core logger
- [ ] Memory uses core allocator
- [ ] SelfTest() implemented
- [ ] Version follows semver
- [ ] Platform requirements documented

Status

✅ FROZEN
This is the plugin contract. All plugins must comply.
