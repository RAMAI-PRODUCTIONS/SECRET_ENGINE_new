# Vulkan 1.4 Rules for Android Game Development

> **Target stack**: Vulkan 1.4 · Android NDK r29 · API 35 · arm64-v8a

---

## Core vs. Extension — Reference Table

With `apiVersion = VK_MAKE_API_VERSION(0,1,4,0)` set, everything below is
**core** — no `vkEnumerateDeviceExtensionProperties` guard, no explicit enable.
Do **not** add extension-check boilerplate for these.

| Feature | Core since |
|---|---|
| `vkGetPhysicalDeviceProperties2` family | 1.1 |
| `VkPhysicalDeviceFeatures2` pNext chaining | 1.1 |
| Multiview (`VkPhysicalDeviceMultiviewFeatures`) | 1.1 |
| Timeline semaphores | 1.2 |
| Buffer device addresses | 1.2 |
| Descriptor indexing (partial binding, update-after-bind) | 1.2 |
| `float16` / `int8` shader types | 1.2 |
| Host query reset (`vkResetQueryPool`) | 1.2 |
| Draw indirect count | 1.2 |
| SPIR-V 1.5 minimum | 1.2 |
| `synchronization2` — barriers2, `vkCmdPipelineBarrier2` | 1.3 |
| Dynamic rendering — `vkCmdBeginRendering` | 1.3 |
| Extended dynamic state (1 & 2) | 1.3 |
| `maintenance4` | 1.3 |
| Format feature flags 2 | 1.3 |
| Inline uniform blocks | 1.3 |
| **`push_descriptor`** — `vkCmdPushDescriptorSet` | **1.4** |
| **`host_image_copy`** — CPU → VkImage, no staging buffer | **1.4** |
| **`uint8` index type** — `VK_INDEX_TYPE_UINT8` | **1.4** |
| **`maintenance5`, `maintenance6`** | **1.4** |
| **`map_memory2`** — `vkMapMemory2` | **1.4** |
| **`dynamic_rendering_local_read`** | **1.4** |
| **`pipeline_robustness`** | **1.4** |
| **`shader_expect_assume`** — `OpAssumeTrueKHR` | **1.4** |
| **`shader_subgroup_rotate`** | **1.4** |
| **`vertex_attribute_divisor`** | **1.4** |
| **`global_priority`** queue priorities | **1.4** |
| **`load_store_op_none`** | **1.4** |

Still **extensions** (check availability before use):
`VK_EXT_memory_budget`, `VK_GOOGLE_display_timing`, `VK_EXT_swapchain_colorspace`,
`VK_KHR_android_surface`, `VK_KHR_surface`, `VK_KHR_swapchain`,
`VK_EXT_debug_utils`, `VK_KHR_driver_properties`,
`VK_ANDROID_external_memory_android_hardware_buffer`,
`VK_EXT_subgroup_size_control`, `VK_EXT_calibrated_timestamps`,
`VK_KHR_pipeline_binary`.

---

## Instance and Device Creation

- **MANDATORY**: Set `VkApplicationInfo::apiVersion = VK_MAKE_API_VERSION(0, 1, 4, 0)`
- **MANDATORY**: Enable instance extensions `VK_KHR_surface` + `VK_KHR_android_surface` (still extensions)
- **ENFORCED**: Enable `VK_EXT_debug_utils` **in debug builds only**; strip it from release
- **MANDATORY**: Chain feature enablement through `VkPhysicalDeviceVulkan12Features`, `VkPhysicalDeviceVulkan13Features`, and `VkPhysicalDeviceVulkan14Features` structs — never use `VkDeviceCreateInfo::pEnabledFeatures` (Vulkan 1.0 path) for promoted features
- **REQUIRED**: Enable `VK_KHR_driver_properties` device extension; log `VkPhysicalDeviceDriverPropertiesKHR::driverName` and `conformanceVersion` at startup
- **ENFORCED**: Query and guard **only** truly optional, still-extension features (see list above) with `vkEnumerateDeviceExtensionProperties`

## Validation and Debugging

- **MANDATORY**: Enable `VK_LAYER_KHRONOS_validation` in debug APKs; do not ship it in release
- **REQUIRED**: Register `VkDebugUtilsMessengerEXT` with severity ≥ `WARNING`; route output to `__android_log_print(ANDROID_LOG_WARN/ERROR, ...)`
- **ENFORCED**: Call `vkSetDebugUtilsObjectNameEXT` for every long-lived object (pipelines, buffers, images, pools); this is the only way to get readable names in RenderDoc and AGI captures
- **MANDATORY**: Wrap every Vulkan call with a `VK_CHECK(result)` macro: `__android_log_assert` + stacktrace in debug, `std::terminate` in release
- **REQUIRED**: Annotate render passes and compute dispatches with `vkCmdBeginDebugUtilsLabelEXT` / `vkCmdEndDebugUtilsLabelEXT`

## Memory Management

- **MANDATORY**: Use Vulkan Memory Allocator ≥ 3.1 — **zero** raw `vkAllocateMemory` / `vkFreeMemory` calls anywhere in the codebase
- **REQUIRED**: Create `VmaAllocator` with:
  - `VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT` (always — BDA is core 1.2)
  - `VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT` when `VK_EXT_memory_budget` is present
- **MANDATORY**: Use `VMA_MEMORY_USAGE_AUTO` with `VMA_ALLOCATION_CREATE_*` access flags — the legacy `VMA_MEMORY_USAGE_GPU_ONLY` / `CPU_ONLY` etc. are **banned** (deprecated in VMA 3.x)
- **ENFORCED**: `VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT` for staging and upload buffers
- **ENFORCED**: `VMA_ALLOCATION_CREATE_MAPPED_BIT` only for memory that needs a persistent CPU pointer
- **REQUIRED**: `VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT` for any single resource > 16 MB
- **MANDATORY**: Call `vmaGetHeapBudgets()` before large batch allocations; abort or degrade quality if usage would exceed 75% of the reported GPU heap budget
- **ENFORCED**: Use `vkMapMemory2` (core 1.4) for all new mapping code; retire `vkMapMemory`

## Command Buffers and Synchronization

- **MANDATORY**: One `VkCommandPool` per thread; reset pool with `vkResetCommandPool` at the start of the frame — never recreate pools per frame
- **MANDATORY**: Use `vkCmdPipelineBarrier2` (`VkDependencyInfo` + `VkMemoryBarrier2` / `VkImageMemoryBarrier2`) — the Vulkan 1.0 `vkCmdPipelineBarrier` is **banned** for new code
- **REQUIRED**: Specify exact `srcStageMask2` / `dstStageMask2` / `srcAccessMask2` / `dstAccessMask2` for every barrier — never `VK_PIPELINE_STAGE_ALL_COMMANDS_BIT` except during init or teardown
- **MANDATORY**: Use timeline semaphores (core 1.2) for all inter-frame and inter-queue dependencies; binary semaphores only for present wait / signal
- **ENFORCED**: Use separate command pools per thread when recording on multiple threads; share queues only with external synchronization
- **REQUIRED**: Use `VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT` for upload and transition command buffers

## Render Passes — Dynamic Rendering

- **MANDATORY**: Use `vkCmdBeginRendering` / `vkCmdEndRendering` (core 1.3) for **all** render passes — no `VkRenderPass` / `VkFramebuffer` objects for new code
- **REQUIRED**: Set `VkRenderingAttachmentInfo::loadOp` and `storeOp` per attachment; use `VK_ATTACHMENT_LOAD_OP_NONE` / `VK_ATTACHMENT_STORE_OP_NONE` (core 1.4) for attachments that are fully overwritten or whose result is unused
- **ENFORCED**: Use `VkRenderingInfo::viewMask` for multiview / stereo rendering (Vulkan 1.1 multiview is core)
- **MANDATORY**: Use `dynamic_rendering_local_read` (core 1.4) for subpass-like input attachment patterns without explicit subpasses

## Pipeline and Shader Management

- **MANDATORY**: Serialize and reload `VkPipelineCache` data to `getFilesDir()` on Android — load before any `vkCreate*Pipeline` call, save on `onStop`
- **REQUIRED**: Use `VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT` in debug builds to detect cache misses
- **ENFORCED**: Evaluate `VK_KHR_pipeline_binary` (extension, check availability) for portable, driver-agnostic binary caching; prefer over raw `VkPipelineCache` blob distribution
- **MANDATORY**: Compile shaders with `glslc --target-env=vulkan1.4 -O`; validate with `spirv-val --target-env vulkan1.4`; optimise release SPIR-V with `spirv-opt -O`
- **REQUIRED**: Use specialization constants (`VkSpecializationInfo`) for per-variant shader constants — one SPIR-V blob per stage, not per variant
- **ENFORCED**: Sort draw calls: framebuffer attachments → pipeline → descriptor set → vertex input

## Descriptor Sets and Push Descriptors

- **MANDATORY**: Use `vkCmdPushDescriptorSet` (core 1.4) for small, high-churn descriptor updates (per-object transforms, material parameters) — replaces dynamic UBO offsets for this use case
- **REQUIRED**: Use `vkCmdPushDescriptorSetWithTemplate` for repeated push-descriptor patterns where the update template is stable
- **ENFORCED**: Use descriptor indexing (core 1.2) for all texture arrays and bindless resources; set `VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT` + `VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT`
- **MANDATORY**: Pre-create all `VkDescriptorSetLayout` and `VkDescriptorPool` at init time — no per-frame pool creation
- **REQUIRED**: Allocate pools with 20% headroom; use `VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT` when any binding has `UPDATE_AFTER_BIND`

## Swapchain and Presentation

- **MANDATORY**: Pass `VkSwapchainCreateInfoKHR::oldSwapchain` when recreating; destroy old swapchain after new one is valid
- **REQUIRED**: Always match swapchain extent to `VkSurfaceCapabilitiesKHR::currentExtent` on Android (fixed by the OS)
- **ENFORCED**: Prefer `VK_FORMAT_R8G8B8A8_SRGB` with `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`; check `VK_EXT_swapchain_colorspace` for `VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT` on HDR panels
- **MANDATORY**: Handle both `VK_ERROR_OUT_OF_DATE_KHR` and `VK_SUBOPTIMAL_KHR` by triggering swapchain recreation before the next `vkQueuePresentKHR`
- **REQUIRED**: Request `VK_GOOGLE_display_timing` (extension) if available for sub-millisecond frame delivery scheduling

## Resource Uploads

- **MANDATORY**: Evaluate `vkCopyMemoryToImageEXT` (host_image_copy, core 1.4) before allocating a staging buffer — for write-once static textures it eliminates the staging buffer entirely
- **REQUIRED**: Use staging buffers (host-visible → device-local via `vkCmdCopyBufferToImage2`) for streamed or dynamically-updated textures
- **ENFORCED**: Set `VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT` on images uploaded via host_image_copy
- **MANDATORY**: Transition image layout with `vkTransitionImageLayoutEXT` (core 1.4 host_image_copy) when using CPU upload path

## Index Buffers

- **MANDATORY**: Use `VK_INDEX_TYPE_UINT8` (core 1.4) for meshes with ≤ 255 unique vertices — significant bandwidth savings on mobile tile GPUs
- **REQUIRED**: Use `VK_INDEX_TYPE_UINT16` for meshes with ≤ 65 535 unique vertices
- **ENFORCED**: Use `VK_INDEX_TYPE_UINT32` only when genuinely needed; document the reason in code

## Compute and Subgroup Operations

- **MANDATORY**: Use `subgroupRotate` / `subgroupClusteredRotate` (core 1.4 `shader_subgroup_rotate`) for efficient intra-wave data movement
- **REQUIRED**: Align compute workgroup X size to `VkPhysicalDeviceSubgroupProperties::subgroupSize` (32 on Adreno, 64 on Mali G78+)
- **ENFORCED**: Prefer subgroup reductions (`subgroupAdd`, `subgroupBallot`, `subgroupShuffle`) over shared-memory ping-pong
- **MANDATORY**: Use `OpAssumeTrueKHR` / `assumeEXT` (core 1.4 `shader_expect_assume`) in GLSL compute hot paths to guide driver optimisation

## Android-Specific Vulkan

- **MANDATORY**: Obtain `ANativeWindow*` via `ANativeWindow_fromSurface(env, surface)` inside `onSurfaceCreated`; never cache it across Activity restarts
- **REQUIRED**: `onPause` → `vkDeviceWaitIdle` → destroy swapchain → destroy `VkSurfaceKHR`. `onResume` → create surface → create swapchain
- **MANDATORY**: Use `VK_ANDROID_external_memory_android_hardware_buffer` extension for zero-copy camera / MediaCodec → Vulkan pipelines
- **ENFORCED**: Enforce the 75% memory budget rule using `VK_EXT_memory_budget` (extension); query each frame on constrained devices (< 4 GB RAM)
- **REQUIRED**: Check `VK_KHR_driver_properties::conformanceVersion`; log a warning if non-conformant
- **MANDATORY**: Profile frame timing with `VK_QUERY_TYPE_TIMESTAMP`; reset query pool with `vkResetQueryPool` (core 1.2) outside render pass

## Performance

- **ENFORCED**: Keep draw calls < 2 000 per frame on mid-range devices (Snapdragon 7s Gen 2 / Mali-G710 class)
- **MANDATORY**: Use `vkCmdDrawIndexedIndirect` or `vkCmdDrawIndexedIndirectCount` (core 1.2) for GPU-driven indirect rendering
- **REQUIRED**: Use `VK_QUERY_TYPE_OCCLUSION` for GPU-side occlusion culling where the object count justifies the overhead
- **ENFORCED**: Use `VK_EXT_subgroup_size_control` (extension, check availability) to pin compute subgroup size for predictable wavefront packing
- **MANDATORY**: Use `VK_EXT_calibrated_timestamps` (extension, check availability) for correlated CPU–GPU timestamp pairs when debugging frame pacing
