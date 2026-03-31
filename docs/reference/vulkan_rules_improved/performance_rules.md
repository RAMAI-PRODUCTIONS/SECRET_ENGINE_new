# Performance Rules for Vulkan Android Game Development

> **Stack**: Vulkan 1.4 · C++26 · NDK r29 · API 35 · arm64-v8a

---

## Profiling First

- **MANDATORY**: Profile on real Android hardware before making any optimisation — emulator GPU results are invalid
- **REQUIRED**: Establish a baseline frame time measurement (`VK_QUERY_TYPE_TIMESTAMP`, Perfetto) before any change; confirm improvement after
- **ENFORCED**: Identify whether the bottleneck is CPU-bound or GPU-bound before optimising; tools diverge completely
- **MANDATORY**: Optimise only what profiling data identifies as a bottleneck — no speculative micro-optimisation

### Profiling Tools

| Tool | What it measures | How to invoke |
|---|---|---|
| `VK_QUERY_TYPE_TIMESTAMP` | GPU pass durations | `vkCmdWriteTimestamp2` at pass boundaries |
| Perfetto | CPU traces, scheduling, I/O | `adb shell perfetto --txt -c cfg.pbtx -o /data/misc/perfetto-traces/t.pb` |
| RenderDoc | GPU draw calls, resources, shaders | ADB layer injection; view captures on desktop |
| AGI (Android GPU Inspector) | Hardware perf counters (Adreno) | Frame capture via USB; shows ALU/cache stats |
| Arm Mobile Studio (Streamline) | Hardware perf counters (Mali) | Daemon on device; timeline + counter charts |
| `adb shell dumpsys gfxinfo` | Frame pacing, jank count | `adb shell dumpsys gfxinfo <pkg> framestats` |
| `ATrace_beginSection` | Custom CPU spans in Perfetto | `<android/trace.h>` in hot paths |

- **MANDATORY**: Use `VkQueryPool` timestamp pairs around every render pass and major compute dispatch; read results back with `vkGetQueryPoolResults` after the frame fence signals; reset pool with `vkResetQueryPool` (core 1.2) outside a render pass
- **REQUIRED**: Use `VK_EXT_calibrated_timestamps` (extension, check availability) for correlated CPU–GPU timestamps when diagnosing frame pacing issues
- **ENFORCED**: Frame time targets:
  - 60 Hz: < 16.67 ms total (< 12 ms GPU, < 10 ms CPU logic)
  - 90 Hz: < 11.11 ms total
  - 120 Hz: < 8.33 ms total
  - Minimum spec (30 Hz): < 33.33 ms total

---

## Memory Management

- **MANDATORY**: All GPU memory via VMA ≥ 3.1 with `VMA_MEMORY_USAGE_AUTO`; zero raw `vkAllocateMemory` calls
- **REQUIRED**: Query `vmaGetHeapBudgets()` at the start of each scene load; abort streaming or lower texture quality if usage would exceed 75% of the reported device-local heap
- **ENFORCED**: `VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT` for any resource > 16 MB
- **MANDATORY**: Never allocate on the heap in the per-frame hot path — pre-allocate all buffers (staging, uniform, query) at init time; reuse via ring buffers or suballocated VMA pools
- **REQUIRED**: Use `std::pmr::monotonic_buffer_resource` for CPU-side per-frame allocations; reset at frame start
- **ENFORCED**: Track VMA stats per frame with `vmaCalculateStatistics`; trigger a warning log if the per-frame alloc count exceeds 50
- **MANDATORY**: Cache `VkSampler`, `VkDescriptorSetLayout`, `VkPipelineLayout` objects — they are immutable; never recreate identical ones mid-session

## CPU-Side Optimisation

- **MANDATORY**: `-march=armv8.2-a+dotprod+fp16` to unlock hardware dot-product and native fp16 on Cortex-A76+ devices
- **REQUIRED**: `-flto=thin` for link-time cross-TU inlining; confirm binary size doesn't regress beyond budget
- **ENFORCED**: `[[likely]]` / `[[unlikely]]` on branches only after profiling confirms > 90% bias; applied to < 5% of all branches
- **MANDATORY**: `std::flat_map` / `std::flat_set` for hot-path lookups (pipeline cache, resource lookup by ID) — sorted, contiguous, cache-friendly
- **REQUIRED**: `std::inplace_vector<T, N>` (C++26) for small per-frame lists (visible entity IDs, descriptor writes, barrier batches) — no heap allocation
- **ENFORCED**: `std::pmr::unsynchronized_pool_resource` for thread-local scratch inside job system workers
- **MANDATORY**: `std::vector::reserve()` at construction for any container whose final size is estimable
- **REQUIRED**: Use `std::unreachable()` in provably-unreachable `default:` cases to enable compiler optimisation of surrounding code

## GPU Draw Call Optimisation

- **MANDATORY**: Sort draws each frame: render attachment layout → pipeline → descriptor set → vertex input — enforce with a `RenderKey` sortable integer
- **REQUIRED**: Target < 500 unique pipeline state changes per frame on mid-range devices (Snapdragon 7s Gen 2 / Mali-G710)
- **ENFORCED**: Target < 2 000 draw calls per frame on mid-range; < 500 on minimum spec (Snapdragon 730G)
- **MANDATORY**: Use `vkCmdDrawIndexedIndirect` / `vkCmdDrawIndexedIndirectCount` (core 1.2) for GPU-driven rendering of large batches — eliminates CPU loop over draw calls
- **REQUIRED**: Instancing (`VK_VERTEX_INPUT_RATE_INSTANCE`) for any geometry drawn > 4 times per frame with the same mesh
- **ENFORCED**: Bindless textures via descriptor indexing (core 1.2) — eliminate per-draw descriptor set switches for textures
- **MANDATORY**: Use `vkCmdPushDescriptorSet` (core 1.4) for per-object data (model matrix, material index) — no `VkDescriptorPool` churn

## Command Buffer and Synchronisation

- **MANDATORY**: `vkCmdPipelineBarrier2` with `VkDependencyInfo` (synchronization2, core 1.3) — the Vulkan 1.0 barrier API is **banned** for new code
- **REQUIRED**: Specify exact `srcStageMask2` / `dstStageMask2` — never `VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT` in the hot path (causes full pipeline flush)
- **ENFORCED**: Batch multiple `VkImageMemoryBarrier2` into a single `vkCmdPipelineBarrier2` call — do not issue one barrier per image
- **MANDATORY**: Record command buffers once per frame; avoid re-recording mid-frame for static or semi-static geometry
- **REQUIRED**: Use secondary command buffers for stable, repeated draw batches (static geometry, UI) to enable parallel recording
- **ENFORCED**: Timeline semaphores (core 1.2) for all inter-queue and inter-frame dependencies; binary semaphores only for `vkQueuePresentKHR`

## Dynamic Rendering

- **MANDATORY**: Use `vkCmdBeginRendering` / `vkCmdEndRendering` (core 1.3) — no `VkRenderPass` / `VkFramebuffer` objects for new code
- **REQUIRED**: Use `VK_ATTACHMENT_LOAD_OP_NONE` (core 1.4) for depth/stencil attachments that are fully overwritten by the first draw
- **ENFORCED**: On tile-based GPUs (Mali, Adreno): ensure colour attachments use `VK_ATTACHMENT_STORE_OP_DONT_CARE` for transient targets (e.g. resolved MSAA source) — averts costly tile writeback
- **MANDATORY**: Use `VK_ATTACHMENT_LOAD_OP_CLEAR` for colour and depth at the start of each frame; never leave initial state as `UNDEFINED` without an explicit clear or full-screen overdraw

## Vertex and Index Data

- **MANDATORY**: `VK_INDEX_TYPE_UINT8` (core 1.4) for meshes ≤ 255 unique vertices — halves index bandwidth on mobile
- **REQUIRED**: `VK_INDEX_TYPE_UINT16` for meshes ≤ 65 535 unique vertices — default for most game geometry
- **ENFORCED**: `VK_INDEX_TYPE_UINT32` only when necessary; document in code
- **MANDATORY**: Interleaved vertex layout (`POSITION|NORMAL|UV` packed per-vertex) for better L1 cache locality during vertex fetch
- **REQUIRED**: Compress vertex attributes: 16-bit half-float UVs, 10/10/10/2 packed normals/tangents, half-float positions for small geometry
- **ENFORCED**: Align vertex stride to 16 bytes for Mali; 4-byte component alignment for Adreno

## Texture and Sampler Optimisation

- **MANDATORY**: **ASTC** (4×4 or 6×6 block) for devices that report `VkPhysicalDeviceFeatures::textureCompressionASTC_LDR`; fall back to **ETC2** (universal on API 24+) — never uncompressed for world textures
- **REQUIRED**: Generate mipmaps offline (via offline tool or `vkCmdBlitImage` at load time); never generate per-frame
- **ENFORCED**: Texture atlases for all UI and frequently-used sprite sheets — reduce descriptor set changes to zero for 2D draws
- **MANDATORY**: Cache `VkSampler` objects keyed on `VkSamplerCreateInfo` hash — they are immutable; never create per-frame
- **REQUIRED**: `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE` for UI; `VK_SAMPLER_ADDRESS_MODE_REPEAT` for world tiling textures
- **ENFORCED**: Anisotropic filtering only on world geometry where tested profiling shows perceptible quality improvement; cap at 4× on mobile

## Resource Uploads — Host Image Copy

- **MANDATORY**: Evaluate `vkCopyMemoryToImageEXT` (host_image_copy, core 1.4) before allocating a staging buffer for write-once static textures — eliminates staging buffer alloc + transfer queue submission
- **REQUIRED**: Use the traditional staging path (`vkCmdCopyBufferToImage2`) for streaming textures, video frames, and any texture updated > once per scene
- **ENFORCED**: On upload: transition image layout with `vkTransitionImageLayoutEXT` (host_image_copy path) or explicit barrier (staging path) before first sample

## Shader Optimisation

- **MANDATORY**: Compile with `glslc --target-env=vulkan1.4 -O`; run `spirv-opt -O` on all release SPIR-V
- **REQUIRED**: `float16_t` / `f16vec*` (requires `VkPhysicalDeviceVulkan12Features::shaderFloat16`) for all intermediate colour, lighting, and effect computations where fp32 precision is unnecessary
- **ENFORCED**: Use specialization constants (`VkSpecializationInfo`) for quality-level knobs (shadow cascades, particle count, MSAA samples) — driver can eliminate dead code at pipeline creation
- **MANDATORY**: Use `OpAssumeTrueKHR` / `assumeEXT` (core 1.4 `shader_expect_assume`) in GLSL to annotate branch conditions the driver can assume true — enables better wavefront scheduling
- **REQUIRED**: Minimise divergent branches; prefer arithmetic selection (`mix(a, b, bvec)`, `clamp`, `step`) over `if`/`else` in fragment and compute shaders
- **ENFORCED**: Cache texture sample results in local variables if a texel is read > once in a shader invocation

## Compute Shaders

- **MANDATORY**: Align workgroup X size to the GPU's subgroup size: 32 on Adreno (Snapdragon 8 Gen 2+), 64 on Mali (Dimensity 9200+)
- **REQUIRED**: Use `subgroupRotate` / `subgroupClusteredRotate` (core 1.4 `shader_subgroup_rotate`) for efficient wavefront-level data exchange
- **ENFORCED**: Prefer subgroup reductions (`subgroupAdd`, `subgroupMin`, `subgroupBallot`) over shared-memory ping-pong for inter-thread communication
- **MANDATORY**: Use `shared` memory for data reused within a workgroup; size it to fit in the GPU's L1 scratchpad (32–64 KB on current mobile GPUs)
- **REQUIRED**: Dispatch compute on a dedicated async compute queue when overlapping with graphics work; synchronise via timeline semaphore

## Frame Pacing and Presentation

- **MANDATORY**: Default present mode: `VK_PRESENT_MODE_FIFO_KHR` (vsync, power-efficient)
- **REQUIRED**: `VK_PRESENT_MODE_MAILBOX_KHR` only after measuring a latency reduction on the target device; it increases GPU power consumption
- **ENFORCED**: Triple-buffering (3 swapchain images) for 60/90/120 Hz targets; double-buffering for 30 Hz minimum-spec targets
- **MANDATORY**: Use `VK_GOOGLE_display_timing` (extension, check availability) for sub-millisecond presentation scheduling on supported Android devices
- **REQUIRED**: Monitor `VkPresentTimeGOOGLE` results; log a warning if actual present time deviates > 2 ms from desired
- **ENFORCED**: Use `VK_KHR_incremental_present` (extension, check availability) for partial swapchain updates (UI-only frames, overlay updates)

## Thermal and Power Management

- **MANDATORY**: Query `AThermalManager_getThermalHeadroom(thermalManager, 5.0f)` (NDK API 30+) every 5 seconds; begin quality reduction at headroom < 0.5
- **REQUIRED**: Adaptive quality ladder (triggered by thermal and frame time):
  1. Disable post-processing (bloom, DOF, SSAO)
  2. Halve particle emission rate
  3. Reduce shadow map resolution (2048 → 1024 → 512)
  4. Enable dynamic resolution scaling (down to 50% native)
- **ENFORCED**: Dynamic resolution scaling: reduce render target size when mean frame time > 18 ms for 3 consecutive frames; recover when mean < 14 ms for 10 consecutive frames
- **MANDATORY**: Request `SUSTAIN_PERFORMANCE` mode via `android.os.PowerManager` (JNI) at session start for consistent performance in performance-critical benchmarks / tournaments
- **REQUIRED**: Profile battery consumption: `adb shell dumpsys batterystats --reset` before test; `adb shell dumpsys batterystats` after; target < 5% battery per 10 minutes of gameplay on a Pixel 6a

## Android-Specific

- **MANDATORY**: `VK_EXT_memory_priority` (extension, check availability) — set `VkMemoryPriorityAllocateInfoEXT::priority = 1.0f` for frame-critical resources (swapchain images, depth buffer, shadow maps)
- **REQUIRED**: Use `AHardwareBuffer` + `VK_ANDROID_external_memory_android_hardware_buffer` for camera / MediaCodec zero-copy pipelines; query `vkGetAndroidHardwareBufferPropertiesANDROID` before import
- **ENFORCED**: `adb shell dumpsys SurfaceFlinger` to verify composition mode (GPU vs. HWC) — ensure game frames are composited by HWC, not falling back to GPU composition
- **MANDATORY**: Instrument critical paths with `ATrace_beginSection` / `ATrace_endSection` so they appear in Perfetto traces for correlating CPU and GPU activity

## Storage I/O

- **MANDATORY**: `#embed "asset.bin"` (C++26) for compile-time embedding of small, static binary assets (SPIR-V shaders, icon atlases, font bitmaps) — zero runtime I/O, zero `AAssetManager` overhead
- **REQUIRED**: Stream large assets (level geometry, audio) from the APK via `AAssetManager` on a dedicated I/O `std::jthread`; use `AASSET_MODE_STREAMING` for large sequential reads
- **ENFORCED**: Compress save files with LZ4 (fast) or Zstandard (better ratio); decompress on the I/O thread before parsing
- **MANDATORY**: Pipeline cache blob: load from `internalDataPath/pipeline_cache.bin` at startup; save on `APP_CMD_STOP`; validate with a version header before use

## Quality vs. Performance Presets

| Setting | Low (minimum spec) | Medium (target) | High (flagship) |
|---|---|---|---|
| Render scale | 50% | 75% | 100% |
| Shadow map | Off | 512 px | 2048 px |
| MSAA | Off | 2× | 4× |
| Texture quality | ETC2 | ASTC 6×6 | ASTC 4×4 |
| Post-processing | Off | Bloom only | Full |
| Particles | 25% | 50% | 100% |
| Shader precision | `float16` | `float16` | `float32` |
| Draw distance | 50% | 75% | 100% |

- **MANDATORY**: Expose all presets in the settings UI; allow per-setting override
- **REQUIRED**: Persist chosen preset to `internalDataPath/graphics_settings.json`; reload on startup
- **ENFORCED**: Auto-detect preset at first launch based on `VkPhysicalDeviceMemoryProperties` heap size and `VkPhysicalDeviceLimits::maxImageDimension2D`
