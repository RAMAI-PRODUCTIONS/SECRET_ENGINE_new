# SecretEngine

A high-performance, cross-platform 3D game engine built from scratch in C++ using Vulkan.

## Features
- **Ultra-Low Latency Architecture:** 8-byte packets and lock-free SPSC queues for sub-microsecond input-to-game communication.
- **GPU-Driven Mega Geometry:** Render **6,000,000+ triangles** per frame at **43 FPS** (Moto G34 / Adreno 619). This is a **12x-20x density lead** over Call of Duty: Mobile.
- **Real-Time Debug System:** Dedicated `DebugPlugin` for tracking FPS, Triangle counts, and Instancing stats.
- **Vulkan 3D Renderer:** Modern rendering pipeline with push constants and MVP transformations.
- **Plugin Architecture:** Decoupled systems (Renderer, Input, Logic) using RTTI-free interface discovery.
- **Mirrored Mobile UI:** Ergonomic right-side joystick and alphanumeric HUD optimized for CODM-parity mobile ergonomics.
- **ECS (Entity Component System):** Centralized `World` with `Transform` and `Mesh` components.
- **Multi-Platform:** Support for Windows (Visual Studio) and Android (NativeActivity).
- **Scene System:** Binary scene loading support.

## Project Structure
- `core/`: Main engine logic, memory management, and interfaces.
- `plugins/`: Pluggable modules (VulkanRenderer, AndroidInput, GameLogic).
- `platform/`: Platform-specific entry points (Android NativeActivity).
- `Assets/`: Shaders and textures.

## Build Instructions

### Android
1. Open have Android NDK r25+ installed and `ANDROID_NDK` env var set.
2. Run `.\gradlew assembleDebug` in the `android/` folder.
3. Install the APK: `adb install android/app/build/outputs/apk/debug/app-debug.apk`

### Windows
1. Open the project in Visual Studio 2022.
2. Configure with CMake: `cmake -B build -S .`
3. Build the `SecretEngine` and `VulkanRenderer` targets.
4. Run `package_windows.bat` to create a distribution folder.

## Controls
- **Windows:** WASD to move (if GameLogic supports it), ESC to exit.
- **Android:** Touch to toggle cube colors (currently hardcoded in Input/Renderer bridge).

## Documentation
- [AI-Driven Development Guide](docs/architecture/AI_DRIVEN_DEVELOPMENT.md) - How to use Claude/Gemini to develop plugins independently.
- [Asset Workflow](docs/ASSET_WORKFLOW.md) - How to cook and import meshes.
- [Plugin Manifest](docs/architecture/PLUGIN_MANIFEST.md) - The strict contract for building engine modules.
- [LLM Coding Rules](docs/implementation/LLM_CODING_RULES.md) - Essential discipline for AI-assisted coding.

## Author
Ramai Productions
