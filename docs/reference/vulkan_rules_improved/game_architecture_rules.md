# Game Architecture Rules for Vulkan Android Game

> **Stack**: C++26 · Vulkan 1.4 · NDK r29 · GameActivity · API 35

---

## Core Architecture — ECS

- **MANDATORY**: Use ECS (Entity-Component-System) as the primary architecture; no deep OOP inheritance hierarchies in game logic
- **REQUIRED**: Use `entt` (header-only, mature, cache-friendly archetypes) or a custom ECS with compile-time component type registration
- **ENFORCED**: No `virtual` methods in ECS systems — use `std::variant` + `std::visit` or concept-constrained templates for dispatch
- **MANDATORY**: Use `std::pmr::monotonic_buffer_resource` for ECS archetype chunk allocation; pre-allocate a 64 MB arena at startup
- **REQUIRED**: Use `std::span<ComponentType>` for component storage views — no raw pointer + size pairs
- **ENFORCED**: Strictly separate `TransformComponent`, `RenderComponent`, `PhysicsComponent`, `AudioComponent` — no monolithic `GameObject`
- **MANDATORY**: Keep component structs POD or aggregate-initializable; no constructors with side effects in component types

## Game Loop

- **MANDATORY**: Fixed-timestep accumulator loop for deterministic physics and gameplay simulation
  ```cpp
  constexpr double kFixedDt = 1.0 / 60.0;
  double accumulator = 0.0;
  while (running) {
      double frameTime = measure(steadyClock);
      accumulator += std::min(frameTime, 0.25); // clamp spiral-of-death
      while (accumulator >= kFixedDt) {
          update(kFixedDt);
          accumulator -= kFixedDt;
      }
      render(accumulator / kFixedDt); // interpolation alpha
  }
  ```
- **REQUIRED**: `render(alpha)` interpolates between previous and current transform states; never renders stale state
- **ENFORCED**: `std::chrono::steady_clock` for all time measurements — `time()`, `clock()`, `gettimeofday()` are **banned**
- **MANDATORY**: Cap `frameTime` at 250 ms to prevent the spiral-of-death on stall (debugger breakpoint, system load spike)
- **ENFORCED**: No `sleep()` / `usleep()` in the main loop — rely on `vkQueuePresentKHR` with `VK_PRESENT_MODE_FIFO_KHR` for pacing
- **MANDATORY**: Implement Android lifecycle pause/resume: on `APP_CMD_LOST_FOCUS` halt the accumulator and suspend rendering; on `APP_CMD_GAINED_FOCUS` reset the clock before resuming to avoid a time spike

## Render Thread Architecture

- **MANDATORY**: Separate render thread from logic/physics thread using a triple-buffered game state ring
- **REQUIRED**: Logic thread writes to `states[writeIndex]`; render thread reads from `states[readIndex]`; `std::atomic<uint32_t>` index handshake
- **ENFORCED**: Render thread only reads committed state — never accesses logic thread's in-progress write buffer
- **MANDATORY**: Render thread lifecycle: wait for `APP_CMD_INIT_WINDOW` before creating Vulkan objects; destroy them on `APP_CMD_TERM_WINDOW`
- **REQUIRED**: Use `std::jthread` for the render thread; stop it via `std::stop_token` before `onDestroy`

## Resource Management

- **MANDATORY**: Central `ResourceManager` with LRU eviction for all runtime assets (textures, meshes, audio clips, pipeline objects)
- **REQUIRED**: Expose resources via `ResourceHandle<T>` — a lightweight wrapper over a 32-bit index into a typed slot map; zero heap allocation per access
- **ENFORCED**: Reference-count within the slot map; evict when ref-count drops to zero and the LRU age exceeds a configurable threshold
- **MANDATORY**: Load assets asynchronously on a dedicated `std::jthread` pool; return `std::future<ResourceHandle<T>>` to the requesting system
- **REQUIRED**: Preload critical assets (shaders via `#embed`, pipeline cache, UI atlas, font) before the first rendered frame
- **ENFORCED**: Use `AAssetManager` for APK-packaged assets; write mutable data (saves, config, pipeline cache blobs) to `internalDataPath`
- **MANDATORY**: Hot-reload SPIR-V shaders in debug builds by watching `internalDataPath` for `.spv` file changes via `inotify`

## Scene Graph and Culling

- **MANDATORY**: Scene spatial structure: BVH (bounding volume hierarchy) for dynamic objects; static geometry in a pre-built octree
- **REQUIRED**: Frustum cull on the CPU against the BVH before uploading any transforms to the GPU each frame
- **ENFORCED**: Output of culling: a `std::span<EntityID>` of visible entities passed directly to the render system — no per-entity heap allocation
- **MANDATORY**: LOD selection: compute screen-space projected size; select mip/mesh LOD in the culling pass, not per draw call
- **REQUIRED**: Use `std::variant<TransformNode, MeshNode, CameraNode, LightNode, DecalNode>` for typed scene node dispatch

## Threading and Job System

- **MANDATORY**: `std::jthread` for all named threads; `std::thread` is **banned**
- **REQUIRED**: Job system: fixed-size worker pool (`std::jthread` × hardware_concurrency − 2); jobs are `std::move_only_function<void()>` in a lock-free MPMC queue
- **ENFORCED**: `std::atomic<bool>` / `std::atomic_flag` for all cross-thread signals — plain `bool` is UB
- **MANDATORY**: `std::condition_variable_any` with `std::stop_token` for worker thread sleep/wake to integrate cleanly with `std::jthread` cooperative cancellation
- **REQUIRED**: Per-thread `std::pmr::unsynchronized_pool_resource` for scratch allocations inside jobs — no shared allocator contention
- **ENFORCED**: Physics, animation, culling, and particle updates run as parallel jobs; no system may depend on another system's output within the same job batch
- **MANDATORY**: `std::latch` to synchronise the end of a job batch before the render submission step

## Input System

- **MANDATORY**: Use `GameActivityPointerAxes` / `GameActivityMotionEvent` from GameActivity — not `AInputQueue`
- **REQUIRED**: Consume input events via `android_app_swap_input_buffers(app)` each frame; process the returned event list before logic update
- **ENFORCED**: Input abstraction layer: unify touch, gamepad (`AGamepad`, Paddleboat SDK), and keyboard into a single `InputEvent` variant
  ```cpp
  using InputEvent = std::variant<TouchEvent, GamepadAxisEvent, GamepadButtonEvent, KeyEvent>;
  ```
- **MANDATORY**: Deadzone normalisation and radial deadzone for all analogue gamepad axes before any game logic reads them
- **REQUIRED**: Gesture recogniser (tap, long-press, swipe, pinch-to-zoom) running on touch events; output high-level `GestureEvent` to the game logic
- **ENFORCED**: All touch targets: minimum 48 dp hit area (enforced at the UI layout level)

## Audio System

- **MANDATORY**: Use **AAudio** (API 26+) as the primary backend; fall back to **Oboe** (wraps both AAudio and OpenSL ES) for API 24–25 compatibility
- **REQUIRED**: AAudio stream configuration: `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY`, `AAUDIO_SHARING_MODE_EXCLUSIVE` for the game SFX stream
- **ENFORCED**: Audio mixing on a dedicated high-priority `std::jthread` (`SCHED_FIFO` if available, else `THREAD_PRIORITY_AUDIO` via `setpriority`)
- **MANDATORY**: Music: stream from `AAssetManager` in 256 KB chunks decoded to PCM on the audio thread; never load an entire music file into RAM
- **REQUIRED**: `std::atomic<float>` for master volume, SFX volume, and music volume — written from UI thread, read from audio thread
- **ENFORCED**: 3D audio: distance attenuation and panning computed per-source per-mix callback; listener state updated from logic thread via a lock-free single-producer single-consumer queue

## Serialisation and Save System

- **MANDATORY**: Use JSON for human-readable config and save data; use [simdjson](https://github.com/simdjson/simdjson) for parsing (SIMD-accelerated, zero-copy, ideal for mobile)
- **REQUIRED**: Save slot system: `save_slot_0.json`, `save_slot_1.json`, `save_slot_2.json` + `autosave.json` in `internalDataPath`
- **ENFORCED**: Every save file includes a CRC32 or xxHash64 checksum of the payload; validate on load; fall back to previous slot on corruption
- **MANDATORY**: `std::expected<SaveData, SaveError>` return type for all load functions — no exceptions
- **REQUIRED**: `std::filesystem` (C++17, NDK r21+) for save file operations on writable storage; `AAssetManager` for read-only packaged data
- **ENFORCED**: Integrate Google Play Games Services (GPGS) cloud save for Play Store releases; treat cloud save as an async mirror, not a blocking dependency

## UI System

- **MANDATORY**: Immediate-mode UI (**Dear ImGui**) for all in-game debug overlays and developer tooling
- **REQUIRED**: Retained-mode UI (custom or a lightweight library) for production menus and HUD; driven by a scene-description data model, not ad-hoc `vkCmdDraw` calls
- **ENFORCED**: All UI draws batched into a single `vkCmdPushDescriptorSet` + `vkCmdDrawIndexed` call per atlas texture at the end of the frame
- **MANDATORY**: Screen-space layout in dp (density-independent pixels); multiply by `displayDensity` (`AConfiguration_getDensity(config) / 160.0f`) to get pixels
- **REQUIRED**: Safe area insets: query via `WindowInsetsController` (JNI) or `GameActivity`'s `WindowInsets` callback; respect notch and navigation bar insets
- **ENFORCED**: Every interactive element: minimum 48 × 48 dp touch target

## Error Handling and Logging

- **MANDATORY**: `std::expected<T, ErrorCode>` everywhere — exceptions are **banned** (`-fno-exceptions`)
- **REQUIRED**: Structured logging with severity levels: `DEBUG`, `INFO`, `WARNING`, `ERROR`, `FATAL`; route to `__android_log_print` with appropriate `ANDROID_LOG_*` priority
- **ENFORCED**: Use `std::source_location::current()` in all log macros — no `__FILE__` / `__LINE__` macros
- **MANDATORY**: On `FATAL`: log a native stacktrace via `<unwind.h>` + `dladdr`, flush logcat, then call `std::terminate`
- **REQUIRED**: Integrate Firebase Crashlytics (or equivalent) for symbolicated production crash reports; upload `.so` symbol files as part of the CI pipeline
- **ENFORCED**: Never silently swallow a `std::unexpected` — either propagate up or log + terminate

## Debugging and Development Tools

- **MANDATORY**: In-game developer console (ImGui overlay): command dispatcher using `std::flat_map<std::string_view, ConsoleCommand>`; supports cvars, stat toggling, and scene inspection
- **REQUIRED**: Real-time frame time graph (last 128 frames) and GPU memory budget bar in the ImGui debug overlay
- **ENFORCED**: ECS entity inspector: list all components on a selected entity with live value editing via ImGui in debug builds
- **MANDATORY**: All debug tooling behind `#ifdef GAME_DEBUG` — zero overhead in release builds

## Testing and QA

- **MANDATORY**: Google Test (gtest) for unit tests of all utility, math, serialisation, and ECS logic; run on the host machine as well as on-device
- **REQUIRED**: Integration tests for Vulkan initialisation, swapchain creation, and pipeline compilation — run on CI using a real Android device farm
- **ENFORCED**: Performance regression gate: if mean frame time over a 60-second run exceeds 17 ms on the target device, the CI build fails
- **MANDATORY**: Fuzz-test all asset and save-file parsers with `libfuzzer` (NDK r29 ships it) to catch malformed-input crashes before release
