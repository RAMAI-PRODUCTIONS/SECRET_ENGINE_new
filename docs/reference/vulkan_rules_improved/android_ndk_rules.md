# Android NDK r29 Rules for Vulkan Game Development

> **Target**: NDK r29 · Clang 19 · API 35 (Android 15) · arm64-v8a · GameActivity

---

## Build System Requirements

- **MANDATORY**: `ndkVersion "29.0.x"` in `build.gradle`; pin the exact patch version to ensure reproducible builds
- **REQUIRED**: `APP_PLATFORM := android-24` minimum (Vulkan 1.0 baseline); `compileSdk 35`, `targetSdk 35`
- **MANDATORY**: `APP_ABI := arm64-v8a` only; drop `armeabi-v7a` and `x86` / `x86_64` for production builds
- **ENFORCED**: `cmake_minimum_required(VERSION 3.22)` with the NDK r29 CMake toolchain (`android.toolchain.cmake`)
- **REQUIRED**: Use the Ninja generator: `-G Ninja` for fast incremental builds
- **MANDATORY**: Compiler flags: `-std=c++26 -march=armv8.2-a+dotprod+fp16 -fno-exceptions -fno-rtti -Wall -Wextra -Werror`
- **ENFORCED**: Release flags: `-O2 -flto=thin -Wl,--strip-all -Wl,--gc-sections`
- **FORBIDDEN**: Targeting API < 24 (security issues; Vulkan requires API 24+)
- **FORBIDDEN**: Makefiles; use CMake + Ninja exclusively

## Native Entry Point — GameActivity

- **MANDATORY**: Use **GameActivity** (Jetpack Games SDK), not the deprecated `NativeActivity` — GameActivity provides `GameActivityMotionEvent`, `GameActivityKeyEvent`, and improved lifecycle handling
- **REQUIRED**: Entry point signature: `void android_main(struct android_app* app)`; the `android_app` struct is provided by the Games SDK glue code
- **ENFORCED**: Process all input events via `android_app_swap_input_buffers()` / `GameActivityPointerAxes` — never poll `AInputQueue` directly on GameActivity
- **MANDATORY**: Set in `AndroidManifest.xml`:
  ```xml
  <activity
      android:name="com.google.androidgamesdk.GameActivity"
      android:hardwareAccelerated="true"
      android:configChanges="orientation|screenSize|keyboardHidden|screenLayout|uiMode"
      android:exported="true">
  ```
- **REQUIRED**: Declare Vulkan feature requirements:
  ```xml
  <uses-feature android:name="android.hardware.vulkan.level"
                android:required="true" />
  <uses-feature android:name="android.hardware.vulkan.version"
                android:required="true"
                android:version="0x00401000" />
  ```
- **ENFORCED**: `targetSdk 35` (mandatory for new Play Store submissions from August 2025)

## ANativeWindow Lifecycle

- **MANDATORY**: Obtain `ANativeWindow*` exclusively via `app->window` from the `android_app` struct (GameActivity sets this); never cache it across `APP_CMD_SURFACE_DESTROYED` / `APP_CMD_SURFACE_CREATED` events
- **REQUIRED**: Handle the full command lifecycle:
  - `APP_CMD_INIT_WINDOW` → create `VkSurfaceKHR`, create swapchain
  - `APP_CMD_TERM_WINDOW` → `vkDeviceWaitIdle` → destroy swapchain → destroy `VkSurfaceKHR`
  - `APP_CMD_GAINED_FOCUS` / `APP_CMD_LOST_FOCUS` → pause/resume rendering
- **ENFORCED**: Call `ANativeWindow_acquire(app->window)` if storing the pointer; call `ANativeWindow_release()` when done
- **MANDATORY**: Query window dimensions from `VkSurfaceCapabilitiesKHR::currentExtent` (set by Android), not from `ANativeWindow_getWidth/Height`

## Asset Management

- **MANDATORY**: Use `AAssetManager` (from `app->activity->assetManager`) for all assets packaged in the APK
- **REQUIRED**: `AAsset* a = AAssetManager_open(mgr, path, AASSET_MODE_BUFFER)` — check for `nullptr` before use; call `AAsset_close(a)` immediately after reading
- **ENFORCED**: Use `#embed "shader.spv"` (C++26) for SPIR-V shaders that don't need hot-reload; eliminates `AAssetManager` overhead for shader loading
- **MANDATORY**: Write mutable data (saves, config, pipeline cache) to `app->activity->internalDataPath` (internal storage) — never to APK assets
- **REQUIRED**: Validate asset checksums (CRC32 or xxHash64) for any bundle containing game-critical data
- **ENFORCED**: Preload shaders, pipeline cache, and core texture atlases during the splash/loading screen before entering the main loop

## Thread Management

- **MANDATORY**: `std::jthread` for all game threads — `std::thread` is **banned** (no cooperative cancellation)
- **REQUIRED**: Honor `std::stop_token` in all thread functions; check `stop_token.stop_requested()` in loop conditions
- **ENFORCED**: Dedicated thread pools: render, physics, audio, asset-loading, network — each a `std::jthread` pulling from a `std::deque<std::function<void()>>` protected by `std::mutex` + `std::condition_variable`
- **MANDATORY**: `std::atomic<bool>` / `std::atomic_flag` for all inter-thread signals — plain `bool` across threads is **undefined behaviour**
- **REQUIRED**: `std::chrono::steady_clock` for all elapsed-time and timeout measurements — never `time()`, `clock()`, or `gettimeofday()`
- **ENFORCED**: Never touch `ANativeWindow` or any JNI object from a non-main thread unless you have attached the thread with `JavaVM::AttachCurrentThread`
- **MANDATORY**: Install `SIGSEGV` and `SIGABRT` signal handlers in debug builds using `sigaction`; log a native stacktrace via `<unwind.h>` + `dladdr` before allowing the crash to proceed

## Vulkan NDK Integration

- **MANDATORY**: Surface creation order: `vkCreateAndroidSurfaceKHR` → `vkGetPhysicalDeviceSurfaceSupportKHR` → `vkGetPhysicalDeviceSurfaceCapabilitiesKHR` — never skip the support check
- **REQUIRED**: Load Vulkan function pointers via `vkGetInstanceProcAddr` / `vkGetDeviceProcAddr`; link against `libvulkan.so` dynamically (not statically)
- **ENFORCED**: Use `VK_ANDROID_external_memory_android_hardware_buffer` for zero-copy `AHardwareBuffer` ↔ `VkImage` sharing (camera, MediaCodec)
- **MANDATORY**: Use `AHardwareBuffer_allocate` / `AHardwareBuffer_release` for buffer lifecycle; never use raw `AHardwareBuffer` with Vulkan without first calling `vkGetAndroidHardwareBufferPropertiesANDROID`
- **REQUIRED**: Verify `VK_KHR_driver_properties` conformance version; log a warning if the driver is non-conformant; consider graceful degradation

## Performance Optimisation Flags

- **MANDATORY**: `-march=armv8.2-a+dotprod+fp16` unlocks Arm v8.2 dot-product and native fp16 arithmetic on Cortex-A76+ (Pixel 4+, Snapdragon 855+, Mali-G77+)
- **REQUIRED**: `-O2` for the default release profile; use `-O3` only for specific TUs proven to benefit by profiling
- **ENFORCED**: `-Oz` for the JNI bridge, asset management glue, and any code executed < once per second
- **REQUIRED**: `-flto=thin` (ThinLTO) for link-time cross-TU inlining and dead-code elimination
- **MANDATORY**: `-Wl,--gc-sections` + `-ffunction-sections -fdata-sections` to eliminate unreferenced code and data
- **ENFORCED**: `-Wl,--strip-all` in release to remove all symbol and debug info from the final `.so`
- **REQUIRED**: Use `__attribute__((always_inline))` or `[[gnu::always_inline]]` sparingly for measured hot-path functions only

## Build and Deployment

- **MANDATORY**: Configure Gradle with `externalNativeBuild { cmake { path "CMakeLists.txt" } }` and set `ndkVersion "29.0.x"` explicitly
- **REQUIRED**: `android.useAndroidX = true` in `gradle.properties`
- **ENFORCED**: `android.enableAapt2 = true` in `gradle.properties`
- **MANDATORY**: `compileSdk 35`, `targetSdk 35`, `minSdk 24` in `build.gradle`
- **REQUIRED**: Sign release APKs with a v3 signature scheme (API 28+); include v2 for compatibility
- **ENFORCED**: Run `adb shell am start -W -n <package>/<activity>` and confirm launch time < 3 s on a Pixel 6a-class device before release
- **MANDATORY**: Test on at least three physical GPU families before release: Adreno (Qualcomm), Mali (Arm), Xclipse (Samsung / AMD)

## Memory and Resource Rules

- **MANDATORY**: Pre-allocate all per-frame resources (command buffers, staging buffers, descriptor sets, query pools) at init time — no heap allocation in the main loop
- **REQUIRED**: Respond to `onTrimMemory(TRIM_MEMORY_RUNNING_CRITICAL)` by releasing optional caches (mipmap data, audio buffers, non-visible textures)
- **ENFORCED**: Use `std::pmr::monotonic_buffer_resource` backed by a per-frame 4 MB stack arena for frame-local allocations; reset the resource at frame start
- **MANDATORY**: Limit active textures to 4096×4096 max; prefer 2048×2048 or smaller on devices with < 4 GB RAM (check `VK_EXT_memory_budget`)
- **REQUIRED**: Release `AAsset` handles immediately after reading — never hold open handles across frames
- **ENFORCED**: Use `std::string_view` for all hot-path string comparisons; no `std::string` construction in the game loop

## Debugging and Profiling

- **MANDATORY**: Filter Vulkan validation output with `adb logcat -s VulkanAPI:* VALIDATION:* *:S` during development
- **REQUIRED**: Enable `VK_LAYER_KHRONOS_validation` in debug APKs; validate both on Adreno and Mali before each release
- **ENFORCED**: Use `perfetto` for full system traces: `adb shell perfetto --txt -c config.pbtx -o /data/misc/perfetto-traces/trace.pb`; view in [ui.perfetto.dev](https://ui.perfetto.dev)
- **MANDATORY**: Use RenderDoc (via `adb forward tcp:1234 tcp:1234` + RenderDoc Android layer) for GPU draw call, resource, and shader analysis
- **REQUIRED**: Instrument critical code paths with `ATrace_beginSection` / `ATrace_endSection` (`<android/trace.h>`) so they appear in Perfetto system traces
- **ENFORCED**: Use `adb shell dumpsys gfxinfo <package> framestats` to verify frame pacing and detect jank
- **MANDATORY**: Profile on minimum-spec device (Pixel 4a class, Snapdragon 730G / 4 GB RAM) as the performance gate; mid-range (Pixel 6a class) as the target
- **REQUIRED**: Profile across all three GPU families: Adreno (Snapdragon 8 Gen 2+), Mali (Dimensity 9200+), Xclipse (Exynos 2400) — behaviour differs significantly
- **ENFORCED**: Use Arm Mobile Studio (Streamline) on Mali targets for hardware performance counter analysis (cache misses, ALU utilisation, bandwidth)
