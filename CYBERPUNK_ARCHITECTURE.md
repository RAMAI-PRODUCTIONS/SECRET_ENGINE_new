# 🏗️ Cyberpunk City - Architecture Diagram

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        ANDROID DEVICE                            │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                    SecretEngine Core                       │  │
│  │  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐  │  │
│  │  │   ICore     │  │   IWorld     │  │  IInputSystem   │  │  │
│  │  │  (Manager)  │  │   (ECS)      │  │  (Fast Proto)   │  │  │
│  │  └─────────────┘  └──────────────┘  └─────────────────┘  │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│         ┌────────────────────┼────────────────────┐             │
│         │                    │                    │             │
│  ┌──────▼──────┐      ┌──────▼──────┐     ┌──────▼──────┐     │
│  │  Rendering  │      │  Gameplay   │     │   Input     │     │
│  │   Plugins   │      │   Plugins   │     │   Plugin    │     │
│  └─────────────┘      └─────────────┘     └─────────────┘     │
│         │                    │                    │             │
│  ┌──────▼──────┐      ┌──────▼──────┐     ┌──────▼──────┐     │
│  │   Vulkan    │      │ Procedural  │     │   Android   │     │
│  │  Renderer   │      │    City     │     │    Input    │     │
│  │             │      │   Plugin    │     │             │     │
│  │ • Meshes    │      │             │     │ • Joystick  │     │
│  │ • Shaders   │      │ • Buildings │     │ • Touch     │     │
│  │ • Particles │      │ • Neon      │     │ • Gestures  │     │
│  └─────────────┘      │ • Beacons   │     └─────────────┘     │
│         │             └─────────────┘            │             │
│         │                    │                   │             │
│  ┌──────▼──────┐      ┌──────▼──────┐           │             │
│  │  Lighting   │      │  Particle   │           │             │
│  │   System    │      │   System    │           │             │
│  │             │      │             │           │             │
│  │ • Ambient   │      │ • Rain      │           │             │
│  │ • Dynamic   │      │ • Sparks    │           │             │
│  │ • Fog       │      │ • Dust      │           │             │
│  └─────────────┘      └─────────────┘           │             │
│         │                    │                   │             │
│         └────────────────────┼───────────────────┘             │
│                              │                                 │
│                       ┌──────▼──────┐                          │
│                       │   Camera    │                          │
│                       │   Plugin    │                          │
│                       │             │                          │
│                       │ • Position  │                          │
│                       │ • Rotation  │                          │
│                       │ • ViewProj  │                          │
│                       └─────────────┘                          │
│                              │                                 │
│                       ┌──────▼──────┐                          │
│                       │   Vulkan    │                          │
│                       │   Surface   │                          │
│                       └─────────────┘                          │
│                              │                                 │
│                       ┌──────▼──────┐                          │
│                       │   Display   │                          │
│                       │   (Screen)  │                          │
│                       └─────────────┘                          │
└─────────────────────────────────────────────────────────────────┘
```

## Data Flow

### 1. Initialization Flow
```
App Launch
    │
    ├─→ Load Core
    │       │
    │       ├─→ Initialize Vulkan
    │       ├─→ Create Window/Surface
    │       └─→ Setup ECS (World)
    │
    ├─→ Load Plugins
    │       │
    │       ├─→ VulkanRenderer
    │       ├─→ CameraPlugin
    │       ├─→ AndroidInput
    │       ├─→ ProceduralCityPlugin ← NEW
    │       └─→ ParticleSystemPlugin ← NEW
    │
    ├─→ Load Level (cyberpunk_city.json)
    │       │
    │       ├─→ Parse fog settings
    │       ├─→ Parse lights
    │       └─→ Set player spawn
    │
    └─→ Generate City
            │
            ├─→ Create ground plane
            ├─→ Generate 100+ buildings
            ├─→ Add neon strips
            ├─→ Place beacons
            └─→ Spawn particles
```

### 2. Frame Update Flow
```
Frame Start (60 FPS target)
    │
    ├─→ Input Processing
    │       │
    │       ├─→ Read touch events
    │       ├─→ Update joystick
    │       ├─→ Calculate look delta
    │       └─→ Send to Fast Protocol
    │
    ├─→ Camera Update
    │       │
    │       ├─→ Process input packets
    │       ├─→ Update position
    │       ├─→ Update rotation
    │       └─→ Calculate ViewProj matrix
    │
    ├─→ Particle Update
    │       │
    │       ├─→ Update rain positions
    │       ├─→ Update spark positions
    │       ├─→ Update dust positions
    │       └─→ Recycle off-screen particles
    │
    ├─→ Rendering
    │       │
    │       ├─→ Begin frame
    │       ├─→ Frustum culling
    │       ├─→ Render buildings (MegaGeometry)
    │       ├─→ Render particles (Point sprites)
    │       ├─→ Apply fog
    │       ├─→ Render UI
    │       └─→ Present frame
    │
    └─→ Frame End
```

### 3. Particle System Flow
```
Particle System (10,600 particles)
    │
    ├─→ Rain Particles (8,000)
    │       │
    │       ├─→ Spawn at Y=55
    │       ├─→ Fall at 18 units/sec
    │       ├─→ Check if Y < 0
    │       └─→ Respawn at top
    │
    ├─→ Spark Particles (600)
    │       │
    │       ├─→ Spawn at Y=12
    │       ├─→ Fall at 1.2 units/sec
    │       ├─→ Check if Y < 0
    │       └─→ Respawn at top
    │
    └─→ Dust Particles (2,000)
            │
            ├─→ Spawn at Y=0
            ├─→ Rise at 0.4 units/sec
            ├─→ Check if Y > 24
            └─→ Respawn at bottom
```

### 4. Building Generation Flow
```
ProceduralCityPlugin::GenerateCity()
    │
    ├─→ Create Ground Plane
    │       └─→ 200x200 dark metallic plane
    │
    ├─→ Generate City Grid
    │       │
    │       ├─→ For X = -42 to 42 (spacing 6.5)
    │       │   └─→ For Z = -42 to 42 (spacing 6.5)
    │       │       │
    │       │       ├─→ Skip if center area
    │       │       ├─→ Random skip (8% chance)
    │       │       └─→ CreateBuilding(x, z)
    │       │           │
    │       │           ├─→ Random height (8-30)
    │       │           ├─→ Random width (2.8-6.3)
    │       │           ├─→ Create main cube
    │       │           ├─→ Add neon window strips
    │       │           ├─→ Add rooftop beacon
    │       │           └─→ Add billboard (50% chance)
    │       │
    │       └─→ Result: ~100 buildings
    │
    └─→ Generate Landmarks
            │
            └─→ For each of 8 positions
                │
                ├─→ Height: 28-40 units
                ├─→ Width: 4.2 units
                ├─→ Add glowing stripes
                └─→ Add neon cap
```

## Component Architecture

### Entity Component System (ECS)

```
Entity (Building)
    │
    ├─→ TransformComponent
    │       ├─→ position: [x, y, z]
    │       ├─→ rotation: [pitch, yaw, roll]
    │       └─→ scale: [width, height, depth]
    │
    └─→ MeshComponent
            ├─→ meshPath: "meshes/cube.meshbin"
            ├─→ color: [r, g, b, a]
            ├─→ texturePath: (optional)
            └─→ normalMapPath: (optional)

Entity (Particle)
    │
    ├─→ TransformComponent
    │       ├─→ position: [x, y, z]
    │       └─→ scale: [0.05, 0.05, 0.3]
    │
    └─→ MeshComponent
            ├─→ meshPath: "meshes/cube.meshbin"
            └─→ color: [r, g, b, alpha]
```

## Shader Pipeline

### Particle Rendering Pipeline

```
Vertex Shader (particle_vert.glsl)
    │
    ├─→ Input: vec3 position, vec4 color
    ├─→ Uniform: mat4 viewProj
    ├─→ Transform: gl_Position = viewProj * vec4(pos, 1.0)
    ├─→ Set: gl_PointSize = 3.0
    └─→ Output: vec4 fragColor
            │
            ▼
Fragment Shader (particle_frag.glsl)
    │
    ├─→ Input: vec4 fragColor, vec2 gl_PointCoord
    ├─→ Calculate: distance from center
    ├─→ Discard: if distance > 0.5
    ├─→ Calculate: soft alpha (smoothstep)
    └─→ Output: vec4(color.rgb, alpha)
            │
            ▼
Blending (Additive)
    │
    ├─→ srcFactor: ONE
    ├─→ dstFactor: ONE
    └─→ Result: Glowing particles
```

### Building Rendering Pipeline

```
Vertex Shader (mega_geometry_vert.glsl)
    │
    ├─→ Input: Instanced transform data
    ├─→ Uniform: ViewProj matrix
    ├─→ Transform: World → View → Clip space
    └─→ Output: Position, Normal, Color
            │
            ▼
Fragment Shader (mega_geometry_frag.glsl)
    │
    ├─→ Input: Position, Normal, Color
    ├─→ Calculate: Lighting (ambient + directional)
    ├─→ Apply: Fog (exponential)
    ├─→ Apply: Emissive (for neon)
    └─→ Output: Final color
```

## Memory Layout

### Particle Buffer (GPU)
```
┌─────────────────────────────────────┐
│  Particle Vertex Buffer (VBO)      │
│  ┌───────────────────────────────┐  │
│  │ Particle 0: [x, y, z, r, g, b, a] │
│  │ Particle 1: [x, y, z, r, g, b, a] │
│  │ Particle 2: [x, y, z, r, g, b, a] │
│  │ ...                            │  │
│  │ Particle 10599: [x, y, z, ...]│  │
│  └───────────────────────────────┘  │
│  Size: 10,600 × 28 bytes = 296 KB  │
└─────────────────────────────────────┘
```

### Building Instance Buffer (GPU)
```
┌─────────────────────────────────────┐
│  Instance Buffer (VBO)              │
│  ┌───────────────────────────────┐  │
│  │ Building 0: [transform matrix] │  │
│  │ Building 1: [transform matrix] │  │
│  │ ...                            │  │
│  │ Building 99: [transform matrix]│  │
│  └───────────────────────────────┘  │
│  Size: 100 × 64 bytes = 6.4 KB     │
└─────────────────────────────────────┘
```

## Performance Profile

### CPU Usage
```
Frame Time Budget: 16.67ms (60 FPS)
    │
    ├─→ Input Processing: 0.5ms
    ├─→ Camera Update: 0.3ms
    ├─→ Particle Update: 2.0ms
    ├─→ ECS Update: 1.0ms
    ├─→ Rendering: 10.0ms
    │       ├─→ Culling: 1.0ms
    │       ├─→ Draw calls: 5.0ms
    │       └─→ GPU wait: 4.0ms
    └─→ Overhead: 2.87ms
```

### GPU Usage
```
GPU Frame Time: ~10ms
    │
    ├─→ Building Rendering: 6ms
    │       ├─→ Vertex processing: 2ms
    │       ├─→ Fragment shading: 3ms
    │       └─→ Depth testing: 1ms
    │
    ├─→ Particle Rendering: 3ms
    │       ├─→ Vertex processing: 1ms
    │       ├─→ Fragment shading: 1.5ms
    │       └─→ Blending: 0.5ms
    │
    └─→ UI Rendering: 1ms
```

### Memory Usage
```
Total GPU Memory: ~50 MB
    │
    ├─→ Vertex Buffers: 10 MB
    ├─→ Index Buffers: 5 MB
    ├─→ Textures: 20 MB
    ├─→ Shaders: 2 MB
    ├─→ Uniform Buffers: 1 MB
    └─→ Framebuffers: 12 MB
```

## Plugin Communication

```
┌─────────────────────────────────────────────────────────┐
│                    Fast Protocol                         │
│  ┌───────────────────────────────────────────────────┐  │
│  │  UltraRingBuffer<512> (Lock-free queue)          │  │
│  │  ┌─────────────────────────────────────────────┐  │  │
│  │  │  Packet: InputAxis (joystick)               │  │  │
│  │  │  Packet: InputAxis (look)                   │  │  │
│  │  │  Packet: InputButton (jump)                 │  │  │
│  │  └─────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────┘  │
│                          │                              │
│         ┌────────────────┼────────────────┐             │
│         │                │                │             │
│  ┌──────▼──────┐  ┌──────▼──────┐  ┌──────▼──────┐    │
│  │   Android   │  │   Camera    │  │  Gameplay   │    │
│  │    Input    │  │   Plugin    │  │   Logic     │    │
│  │             │  │             │  │             │    │
│  │  Produces   │  │  Consumes   │  │  Consumes   │    │
│  │  packets    │  │  packets    │  │  packets    │    │
│  └─────────────┘  └─────────────┘  └─────────────┘    │
└─────────────────────────────────────────────────────────┘
```

## Coordinate System (Z-Up)

```
        Z (Up)
        │
        │
        │
        └────── Y
       ╱
      ╱
     X

Buildings:
  - X, Y: Horizontal plane (city grid)
  - Z: Vertical (height)

Camera:
  - Position: [X, Y, Z]
  - Yaw: Rotation around Z axis
  - Pitch: Up/down look

Particles:
  - Rain: Falls along -Z
  - Dust: Rises along +Z
```

---

This architecture provides a solid foundation for the cyberpunk city demo with room for future enhancements!
