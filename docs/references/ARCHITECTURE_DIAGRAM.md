# SecretEngine New ECS Architecture Diagram

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         SecretEngine                             │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                         ICore                               │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │ │
│  │  │   ILogger    │  │  IRenderer   │  │  IAllocator  │    │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘    │ │
│  │                                                             │ │
│  │  ┌──────────────────────────────────────────────────────┐ │ │
│  │  │                    Scene (NEW)                        │ │ │
│  │  │  ┌────────────────────────────────────────────────┐  │ │ │
│  │  │  │           EntityObject (Actor)                  │  │ │ │
│  │  │  │  ┌──────────────────────────────────────────┐  │  │ │ │
│  │  │  │  │         IComponent (Base)                 │  │  │ │ │
│  │  │  │  │  ┌────────────────────────────────────┐  │  │  │ │ │
│  │  │  │  │  │  TransformComponentNew             │  │  │  │ │ │
│  │  │  │  │  │  MeshComponentNew                  │  │  │  │ │ │
│  │  │  │  │  │  CameraComponentNew                │  │  │  │ │ │
│  │  │  │  │  │  LightComponentNew                 │  │  │  │ │ │
│  │  │  │  │  │  ScriptComponentNew                │  │  │  │ │ │
│  │  │  │  │  │  CustomComponent...                │  │  │  │ │ │
│  │  │  │  │  └────────────────────────────────────┘  │  │  │ │ │
│  │  │  │  └──────────────────────────────────────────┘  │  │ │ │
│  │  │  └────────────────────────────────────────────────┘  │ │ │
│  │  │                                                        │ │ │
│  │  │  ┌────────────────────────────────────────────────┐  │ │ │
│  │  │  │         FastAccess (Optimization)              │  │ │ │
│  │  │  │  • meshes[]                                    │  │ │ │
│  │  │  │  • cameras[]                                   │  │ │ │
│  │  │  │  • lights[]                                    │  │ │ │
│  │  │  │  • transforms[]                                │  │ │ │
│  │  │  └────────────────────────────────────────────────┘  │ │ │
│  │  └──────────────────────────────────────────────────────┘ │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                      Plugins                                │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │ │
│  │  │   Renderer   │  │ Level System │  │   Physics    │    │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘    │ │
│  └────────────────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────────────────────┘
```

## Component Lifecycle Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                      Scene.Play()                                │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                  EntityObject.OnAwake()                          │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Component.OnAwake()  - Initialize component                │ │
│  └────────────────────────────────────────────────────────────┘ │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                 EntityObject.OnEnable()                          │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Component.OnEnable() - Activate component                  │ │
│  └────────────────────────────────────────────────────────────┘ │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                  EntityObject.OnStart()                          │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Component.OnStart()  - First frame logic                   │ │
│  └────────────────────────────────────────────────────────────┘ │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Game Loop                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Scene.Update(deltaTime)                                    │ │
│  │    └─> EntityObject.OnUpdate(deltaTime)                    │ │
│  │          └─> Component.OnUpdate(deltaTime)                 │ │
│  │                                                              │ │
│  │  Scene.FixedUpdate(fixedDeltaTime)                         │ │
│  │    └─> EntityObject.OnFixedUpdate(fixedDeltaTime)         │ │
│  │          └─> Component.OnFixedUpdate(fixedDeltaTime)       │ │
│  │                                                              │ │
│  │  Scene.LateUpdate(deltaTime)                               │ │
│  │    └─> EntityObject.OnLateUpdate(deltaTime)               │ │
│  │          └─> Component.OnLateUpdate(deltaTime)             │ │
│  └────────────────────────────────────────────────────────────┘ │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│              EntityObject.SetActive(false)                       │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Component.OnDisable() - Deactivate component               │ │
│  └────────────────────────────────────────────────────────────┘ │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│           EntityObject.MarkForDestruction()                      │
│           Scene.CollectGarbage()                                 │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Component.OnDestroy() - Cleanup component                  │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## Entity Hierarchy

```
Scene
  │
  ├─ EntityObject "Player" (Root)
  │    ├─ TransformComponentNew (0, 0, 0)
  │    ├─ MeshComponentNew
  │    └─ ScriptComponentNew
  │    │
  │    └─ EntityObject "Weapon" (Child)
  │         ├─ TransformComponentNew (1, 0, 0) [Local]
  │         │   └─ World Position: (1, 0, 0) [Parent + Local]
  │         └─ MeshComponentNew
  │
  ├─ EntityObject "Enemy" (Root)
  │    ├─ TransformComponentNew (10, 0, 0)
  │    ├─ MeshComponentNew
  │    └─ LightComponentNew
  │
  └─ EntityObject "Camera" (Root)
       ├─ TransformComponentNew (0, 5, -10)
       └─ CameraComponentNew (Main)
```

## Component Type System

```
┌─────────────────────────────────────────────────────────────────┐
│                        IComponent                                │
│  • virtual void OnAwake()                                        │
│  • virtual void OnStart()                                        │
│  • virtual void OnUpdate(float deltaTime)                        │
│  • virtual void OnFixedUpdate(float deltaTime)                   │
│  • virtual void OnLateUpdate(float deltaTime)                    │
│  • virtual void OnDisable()                                      │
│  • virtual void OnDestroy()                                      │
│  • virtual const char* GetTypeName() = 0                         │
│  • virtual uint32_t GetTypeID() = 0                              │
│  • EntityObject& m_owner                                         │
└────────────────────────┬────────────────────────────────────────┘
                         │
         ┌───────────────┼───────────────┬───────────────┐
         │               │               │               │
         ▼               ▼               ▼               ▼
┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│ Transform   │ │    Mesh     │ │   Camera    │ │    Light    │
│ Component   │ │  Component  │ │  Component  │ │  Component  │
│             │ │             │ │             │ │             │
│ • Position  │ │ • MeshPath  │ │ • FOV       │ │ • Type      │
│ • Rotation  │ │ • Texture   │ │ • NearPlane │ │ • Color     │
│ • Scale     │ │ • Color     │ │ • FarPlane  │ │ • Intensity │
│ • Matrices  │ │ • Visible   │ │ • IsMain    │ │ • Range     │
└─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘
```

## Fast Access System

```
┌─────────────────────────────────────────────────────────────────┐
│                          Scene                                   │
│                                                                   │
│  Entities (All)                    FastAccess (Optimized)        │
│  ┌──────────────────┐             ┌──────────────────┐          │
│  │ Entity 1         │             │ meshes[]         │          │
│  │  • Transform     │────────────>│  • Mesh 1 ───────┼──┐       │
│  │  • Mesh          │             │  • Mesh 2 ───────┼──┼──┐    │
│  │  • Camera        │             │  • Mesh 3 ───────┼──┼──┼──┐ │
│  └──────────────────┘             └──────────────────┘  │  │  │ │
│                                                           │  │  │ │
│  ┌──────────────────┐             ┌──────────────────┐  │  │  │ │
│  │ Entity 2         │             │ cameras[]        │  │  │  │ │
│  │  • Transform     │────────────>│  • Camera 1 ─────┼──┘  │  │ │
│  │  • Mesh          │             └──────────────────┘     │  │ │
│  │  • Light         │                                      │  │ │
│  └──────────────────┘             ┌──────────────────┐     │  │ │
│                                    │ lights[]         │     │  │ │
│  ┌──────────────────┐             │  • Light 1 ──────┼─────┘  │ │
│  │ Entity 3         │────────────>│  • Light 2 ──────┼────────┘ │
│  │  • Transform     │             └──────────────────┘          │
│  │  • Mesh          │                                            │
│  └──────────────────┘             ┌──────────────────┐          │
│                                    │ transforms[]     │          │
│                                    │  • Transform 1   │          │
│                                    │  • Transform 2   │          │
│                                    │  • Transform 3   │          │
│                                    └──────────────────┘          │
│                                                                   │
│  Rendering Loop:                                                 │
│  for (mesh : fastAccess.meshes) {  // O(n) where n = mesh count │
│      transform = mesh->GetOwner().GetComponent<Transform>();    │
│      Render(mesh, transform);                                    │
│  }                                                                │
└───────────────────────────────────────────────────────────────────┘
```

## Event System

```
┌─────────────────────────────────────────────────────────────────┐
│                    Event<int, std::string>                       │
│                                                                   │
│  Listeners:                                                       │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ ID: 1 → Lambda: [](int score, std::string player) { ... }  │ │
│  │ ID: 2 → Lambda: [](int score, std::string player) { ... }  │ │
│  │ ID: 3 → Lambda: [](int score, std::string player) { ... }  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                   │
│  Invoke(100, "Player1"):                                         │
│    ├─> Call Listener 1 with (100, "Player1")                    │
│    ├─> Call Listener 2 with (100, "Player1")                    │
│    └─> Call Listener 3 with (100, "Player1")                    │
└───────────────────────────────────────────────────────────────────┘

Usage Example:
┌─────────────────────────────────────────────────────────────────┐
│  // Create event                                                 │
│  Event<int, std::string> onScoreChanged;                         │
│                                                                   │
│  // Add listener                                                 │
│  auto id = onScoreChanged.AddListener([](int s, std::string p) {│
│      std::cout << p << " scored " << s << std::endl;            │
│  });                                                              │
│                                                                   │
│  // Invoke event                                                 │
│  onScoreChanged.Invoke(100, "Player1");                          │
│  // Output: "Player1 scored 100"                                 │
│                                                                   │
│  // Remove listener                                              │
│  onScoreChanged.RemoveListener(id);                              │
└───────────────────────────────────────────────────────────────────┘
```

## Component Management

```
EntityObject
  │
  ├─ m_components (vector<shared_ptr<IComponent>>)
  │    ├─ [0] TransformComponentNew
  │    ├─ [1] MeshComponentNew
  │    └─ [2] ScriptComponentNew
  │
  └─ m_componentMap (unordered_map<TypeID, IComponent*>)
       ├─ [TypeID(Transform)] → TransformComponentNew*
       ├─ [TypeID(Mesh)]      → MeshComponentNew*
       └─ [TypeID(Script)]    → ScriptComponentNew*

AddComponent<T>():
  1. Check if component exists in m_componentMap
  2. If exists, return existing component
  3. Create new component with shared_ptr
  4. Add to m_components vector
  5. Add to m_componentMap for O(1) lookup
  6. Call lifecycle methods if entity is initialized
  7. Return component pointer

GetComponent<T>():
  1. Get TypeID for T
  2. Lookup in m_componentMap (O(1))
  3. Return component pointer or nullptr

RemoveComponent<T>():
  1. Get TypeID for T
  2. Find in m_componentMap
  3. Call OnDisable() and OnDestroy()
  4. Remove from m_componentMap
  5. Remove from m_components vector
```

## Memory Layout

```
Scene (1 KB)
  │
  ├─ m_entities: vector<unique_ptr<EntityObject>>
  │    │
  │    ├─ EntityObject (200 bytes)
  │    │    ├─ m_handle: Entity (8 bytes)
  │    │    ├─ m_name: string (32 bytes)
  │    │    ├─ m_components: vector<shared_ptr> (24 bytes)
  │    │    ├─ m_componentMap: unordered_map (56 bytes)
  │    │    ├─ m_parent: EntityObject* (8 bytes)
  │    │    ├─ m_children: vector<EntityObject*> (24 bytes)
  │    │    └─ flags: bool (8 bytes)
  │    │
  │    └─ Components (shared_ptr)
  │         ├─ TransformComponentNew (100 bytes)
  │         ├─ MeshComponentNew (600 bytes)
  │         └─ ScriptComponentNew (50 bytes)
  │
  └─ m_fastAccess: FastAccess (100 bytes)
       ├─ meshes: vector<MeshComponent*> (24 bytes)
       ├─ cameras: vector<CameraComponent*> (24 bytes)
       ├─ lights: vector<LightComponent*> (24 bytes)
       └─ transforms: vector<TransformComponent*> (24 bytes)

Total for 1000 entities with 3 components each:
  • Entities: 200 KB
  • Components: 750 KB
  • Fast Access: 24 KB
  • Total: ~1 MB
```

## Performance Characteristics

```
Operation                    Old System    New System    Improvement
─────────────────────────────────────────────────────────────────────
Component Lookup             O(log n)      O(1)          ✅ Faster
Find All Meshes              O(n)          O(m)          ✅ Much faster
                            (n=entities)  (m=meshes)
Entity Creation              O(1)          O(1)          ✅ Same
Component Addition           O(log n)      O(1)          ✅ Faster
Hierarchy Update             N/A           O(depth)      ✅ New feature
Active State Propagation     N/A           O(children)   ✅ New feature
Memory per Entity            ~50 bytes     ~200 bytes    ⚠️ More (worth it)
Memory per Component         ~100 bytes    ~100 bytes    ✅ Same

Where:
  n = total number of entities
  m = number of specific component type (usually << n)
  depth = hierarchy depth (usually < 10)
  children = number of children (usually < 5)
```

## Integration Points

```
┌─────────────────────────────────────────────────────────────────┐
│                         ICore                                    │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  GetScene() → Scene*                                        │ │
│  │  GetWorld() → IWorld* (OLD, deprecated)                    │ │
│  └────────────────────────────────────────────────────────────┘ │
└────────────────────────┬────────────────────────────────────────┘
                         │
         ┌───────────────┼───────────────┬───────────────┐
         │               │               │               │
         ▼               ▼               ▼               ▼
┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│  Renderer   │ │ Level System│ │   Physics   │ │   Audio     │
│   Plugin    │ │   Plugin    │ │   Plugin    │ │   Plugin    │
│             │ │             │ │             │ │             │
│ Uses:       │ │ Uses:       │ │ Uses:       │ │ Uses:       │
│ • FastAccess│ │ • Scene     │ │ • Scene     │ │ • FastAccess│
│ • Meshes    │ │ • Entities  │ │ • Entities  │ │ • Audio     │
│ • Cameras   │ │ • Components│ │ • Components│ │   Components│
│ • Lights    │ │             │ │             │ │             │
└─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘
```

---

These diagrams provide a visual understanding of the new ECS architecture and how all the pieces fit together!
