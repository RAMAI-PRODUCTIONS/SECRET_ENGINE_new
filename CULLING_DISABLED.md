# Camera Culling Disabled

## Changes Made

All frustum culling has been removed from the camera system. Every object in the scene will now render regardless of whether it's visible to the camera.

### Modified Files

**plugins/VulkanRenderer/shaders/cull.comp**
- Modified `SphereInFrustum()` function to always return `true`
- Original culling code commented out for reference
- All instances now pass the frustum test

### Technical Details

The GPU culling compute shader (`cull.comp`) runs every frame and checks each instance against the camera frustum. Previously, only visible instances were copied to the visible instance buffer. Now:

- **Before:** Only objects in camera view were rendered (optimized)
- **After:** ALL objects render every frame (no culling)

### Performance Impact

Disabling culling means:
- All instances are always rendered
- GPU processes all geometry every frame
- May see performance decrease with large scenes
- Useful for debugging visibility issues

### Original Culling Logic (Now Disabled)

The original frustum culling performed:
1. Transform instance position to clip space
2. Early Z rejection (behind camera or beyond far plane)
3. XY frustum test (left/right/top/bottom bounds)
4. Only visible instances copied to render buffer

### To Re-enable Culling

If you want to restore culling, edit `plugins/VulkanRenderer/shaders/cull.comp`:

```glsl
bool SphereInFrustum(vec3 pos, float radius) {
    // Change this line from:
    return true;
    
    // To: (uncomment the original code below)
    // ... original culling logic ...
}
```

Then recompile shaders:
```bash
cd tools
./compile_3d_shaders.bat
```

And rebuild:
```bash
cd android
./gradlew.bat assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## Deployment Status

✅ Shader recompiled with culling disabled
✅ APK rebuilt
✅ Installed on device: ZD222JHZ2N
✅ App launched

## Testing

The app is now running with no culling. All objects in the scene will render regardless of camera direction. You should see:
- Consistent triangle count regardless of where you look
- All geometry visible at all times
- Potentially lower FPS due to rendering everything

## Notes

This change only affects the GPU frustum culling. The rendering pipeline still works the same way, but now processes all instances instead of only visible ones.
