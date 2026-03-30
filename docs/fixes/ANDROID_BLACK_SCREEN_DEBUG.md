# Android Black Screen Debugging Guide (RESOLVED)

## Status: ✅ FIXED (2026-02-02)

### Summary of Resolution:
The black screen and crash issues were resolved by:
1. **Synchronized Initialization**: Ensuring Vulkan resources are only accessed after the Android `NativeWindow` is fully valid and initialized.
2. **Proper Resource Destruction**: Fixing a SIGSEGV during swapchain recreation by correctly cleaning up old image views and framebuffers.
3. **2D Pipeline Fix**: Correcting the `Create2DPipeline()` call sequence to ensure all dependencies (device, renderpass) are valid before creation.
4. **Shader Paths**: Fixing incorrect asset paths for shaders on Android.

## Historical Context (Pre-Fix)
... (keeping the rest of the file as reference) ...
