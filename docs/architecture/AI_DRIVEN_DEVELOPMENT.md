# SecretEngine: AI-Driven Development & Plugin Independence

This document outlines how SecretEngine's architecture is specifically optimized for **Independent AI-Assisted Development** using tools like Claude, Gemini, or ChatGPT.

---

## 1. The "Clean Boundary" Advantage

The defining feature of SecretEngine is its **strict plugin isolation**. Unlike monolithic engines where code is a tangled web of dependencies, SecretEngine enforces a "Capillary System" via interfaces.

### Why this is perfect for Web LLMs (Claude/Gemini):
1. **Low Token Overhead**: You don't need to upload the whole engine. You only need to provide the "Surface Area" (the headers).
2. **Context Stability**: The internal logic of the Vulkan Renderer can change entirely, and it won't break a Game Logic plugin, because they only talk through the immutable `ICore` interface.
3. **No "Hallucination" of Dependencies**: Since plugins are forbidden from calling each other directly, an LLM cannot accidentally create a circular dependency that would break your build.

---

## 2. The "Interface-First" Workflow

To develop a new feature using an external AI, follow this **3-Step Context Injection** method:

### Step 1: Inject the "Laws of the Engine"
Provide the LLM with the base interfaces. These tell the AI *how to live* within the engine:
- `core/include/SecretEngine/IPlugin.h` (The Lifecycle)
- `core/include/SecretEngine/ICore.h` (The "Main Hub" access)

### Step 2: Inject the "Domain Language"
Provide the specific headers relevant to the task:
- For logic: `core/include/SecretEngine/IWorld.h` & `Components.h`
- For graphics: `plugins/VulkanRenderer/include/IRenderer.h`

### Step 3: Define the Goal
Ask the AI to generate a **Self-Contained Plugin**.

**Example Prompt:**
> "I am building a plugin for SecretEngine. Using the provided `IPlugin` and `IWorld` interfaces, create a `GravitySystem` plugin. It should find all entities with a `TransformComponent` and apply a constant downward Y-velocity every frame in `OnUpdate`."

---

## 3. Implementation checklist for AI Outputs

When an LLM provides code, verify it against these engine standards:

1. **Self-Contained**: Does it live in its own `plugins/FolderName` directory?
2. **Manifest Presence**: Did it generate a `plugin_manifest.json`?
3. **No `new`/`malloc`**: Does it use the `ICore::GetAllocator()`?
4. **Interface Only**: Does it include headers from other plugins? (If yes, reject it. It must use `core->GetCapability("name")` instead).

---

## 4. Why "Independence" is a Feature

Because of this architecture, you (the Human) act as the **Orchestrator**. 

- You can have **Claude** write a complex Pathfinding plugin.
- You can have **Gemini** write a high-performance Particle system.
- You can have **Antigravity** (this assistant) integrate them into the Build system.

Each "Worker AI" stays within its sandbox, ensuring the core engine remains stable, fast, and maintainable.

---

## 5. Security & Stability

The Engine Core acts as a **Kernel**. Even if an LLM writes a "bad" plugin that crashes, you can simply remove that plugin's entry from `engine_config.json` and the core engine will still boot and run perfectly. This allows for **fearless experimentation**.
