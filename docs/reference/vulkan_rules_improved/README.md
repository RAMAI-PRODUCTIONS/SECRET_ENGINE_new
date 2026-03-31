# Vulkan Android Game Development Rules

> **Stack**: Vulkan 1.4 · C++26 (Clang 19) · NDK r29 · API 35 · GameActivity · arm64-v8a

This project enforces strict coding standards to ensure quality, performance, and platform compliance for a Vulkan 1.4–based Android game. All rules are documented in the following files:

| File | Scope |
|---|---|
| `cpp26_rules.md` | C++26 language features, compilation flags, forbidden patterns |
| `vulkan_rules.md` | Vulkan 1.4 API usage, core-vs-extension reference table |
| `android_ndk_rules.md` | NDK r29, GameActivity, build system, profiling |
| `game_architecture_rules.md` | ECS, game loop, threading, audio, UI, save system |
| `performance_rules.md` | GPU/CPU profiling, draw call budget, thermal management |

---

## Quick Reference — What Changed in This Stack

### Vulkan 1.4 Features Now Used Unconditionally (no extension check)
- `vkCmdPushDescriptorSet` — per-object descriptor updates without pool management
- `vkCmdBeginRendering` / `vkCmdEndRendering` — no `VkRenderPass` / `VkFramebuffer`
- `vkCmdPipelineBarrier2` — synchronization2 barriers (replaces 1.0 barrier API)
- `VK_INDEX_TYPE_UINT8` — 8-bit indices for small meshes
- `vkCopyMemoryToImageEXT` — CPU → VkImage upload without staging buffers
- `vkMapMemory2` — replaces `vkMapMemory`
- `VK_ATTACHMENT_LOAD_OP_NONE` / `VK_ATTACHMENT_STORE_OP_NONE`
- `subgroupRotate` / `assumeEXT` in GLSL shaders
- Timeline semaphores, buffer device addresses, descriptor indexing (core since 1.2/1.3)

### C++26 Features Now Mandatory (Clang 19 / NDK r29)
- `std::inplace_vector<T, N>` — fixed-capacity, stack-resident dynamic array
- `#embed "shader.spv"` — compile-time binary embedding (replaces AAssetManager for shaders)
- Pack indexing `Args...[I]` — variadic argument access without recursion
- `_` placeholder in structured bindings
- `= delete("reason")` — self-documenting deleted functions
- `std::expected<T, E>` (C++23, mandatory) — all error propagation
- `std::flat_map` / `std::flat_set` (C++23, mandatory) — replace `std::map` / `std::set`
- `std::mdspan` (C++23, mandatory) — multi-dimensional buffer views
- `std::jthread` + `std::stop_token` (C++20, mandatory) — `std::thread` is banned

### NDK r29 / GameActivity Changes
- **GameActivity** replaces deprecated `NativeActivity`
- `android_app_swap_input_buffers()` for input polling
- `AThermalManager_getThermalHeadroom()` for thermal monitoring (API 30+)
- ThinLTO (`-flto=thin`) confirmed working in NDK r29
- `-march=armv8.2-a+dotprod+fp16` for hardware dot-product and fp16

---

## Compliance Checklist

- [x] `VkApplicationInfo::apiVersion = VK_MAKE_API_VERSION(0, 1, 4, 0)`
- [x] All feature structs chained through `VkPhysicalDeviceVulkan12/13/14Features`
- [x] Zero raw `vkAllocateMemory` calls — all memory via VMA ≥ 3.1 with `VMA_MEMORY_USAGE_AUTO`
- [x] `-std=c++26 -fno-exceptions -fno-rtti -Werror` enforced in CMakeLists.txt
- [x] `vkCmdPipelineBarrier2` used everywhere — old barrier API banned
- [x] `vkCmdBeginRendering` used everywhere — no `VkRenderPass` objects
- [x] `std::jthread` everywhere — `std::thread` banned
- [x] `std::expected` everywhere — no exceptions
- [ ] `#embed` adopted for all SPIR-V that does not need hot-reload
- [ ] `VK_INDEX_TYPE_UINT8` applied to all eligible meshes
- [ ] `vkCopyMemoryToImageEXT` evaluated for all write-once static textures
- [ ] Thermal headroom monitoring integrated (`AThermalManager`)
- [ ] GPU timestamp queries active on every render pass
- [x] GameActivity (not NativeActivity) configured in `AndroidManifest.xml`
- [x] `targetSdk 35`, `compileSdk 35`, `minSdk 24`
- [x] Validation layers enabled in debug, stripped in release

---

## Enforcement

- **Static analysis**: `cppcheck --enable=all --error-exitcode=1 --std=c++26 --suppress=missingInclude`
- **Shader validation**: `spirv-val --target-env vulkan1.4` on every `.spv` in CI
- **Vulkan validation**: `VK_LAYER_KHRONOS_validation` on debug APK; fail build if any `ERROR` is logged during the integration test run
- **Code review**: `vulkan_rules.md` (extension-vs-core table) and `cpp26_rules.md` (forbidden patterns table) are **mandatory review items** for every PR touching Vulkan or C++ code

---

## Violation Protocol

1. Run `git bisect` to find the introducing commit
2. File a JIRA ticket:
   - `[VULKAN]` for Vulkan API violations
   - `[ANDROID]` for NDK / manifest / build violations
   - `[CPP26]` for language rule violations
3. Revert the commit; do not merge a fix on top of a violation
4. Re-run `spirv-val`, Vulkan validation layer smoke test, and cppcheck before re-landing

---

## Example Violations

```cpp
// BANNED: Old-style barrier (use vkCmdPipelineBarrier2 instead)
vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, ...);

// BANNED: Raw Vulkan allocation (use VMA)
VkMemoryAllocateInfo info{};
vkAllocateMemory(device, &info, nullptr, &memory);

// BANNED: C-style cast (use static_cast / std::bit_cast)
int32_t x = (int32_t)offset;

// BANNED: Ignored VkResult
vkQueueSubmit(queue, 1, &submit, fence);  // must VK_CHECK()

// BANNED: std::thread (use std::jthread)
std::thread t([]{ work(); });

// BANNED: Old VMA usage (use VMA_MEMORY_USAGE_AUTO)
VmaAllocationCreateInfo ai{ .usage = VMA_MEMORY_USAGE_GPU_ONLY };

// BANNED: VkRenderPass + VkFramebuffer (use vkCmdBeginRendering)
vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass);

// BANNED: AAssetManager for SPIR-V (use #embed for static shaders)
AAsset* a = AAssetManager_open(mgr, "shaders/vert.spv", AASSET_MODE_BUFFER);
```

---

## Key Tool Chain

| Tool | Version | Purpose |
|---|---|---|
| Android NDK | r29 | Native build toolchain (Clang 19) |
| Vulkan SDK | latest | `glslc`, `spirv-val`, `spirv-opt`, `vulkaninfo` |
| VMA | ≥ 3.1 | GPU memory allocation |
| GameActivity | Jetpack Games SDK | Native activity + input |
| Oboe | latest | Cross-API audio (AAudio + OpenSL ES) |
| entt | ≥ 3.13 | ECS |
| simdjson | ≥ 3.x | Fast JSON parsing |
| Dear ImGui | latest | Debug UI |
| Perfetto | on-device | CPU / system profiling |
| RenderDoc | latest | GPU frame capture |
| AGI | latest | Adreno hardware counters |
| Arm Mobile Studio | latest | Mali hardware counters |
