# VSync/Frame Pacing Investigation

## Problem
- App runs at 13-14 FPS consistently
- Frame time: ~72ms
- CPU time: 0.1ms
- GPU time: 0.0ms
- **Total work: 0.1ms, but frame takes 72ms = 71.9ms of idle time!**

## Root Cause Analysis

### The Bottleneck is NOT:
- ❌ ECS system (already optimized to O(1) access)
- ❌ Rendering pipeline (GPU is idle at 0.0ms)
- ❌ CPU processing (only 0.1ms per frame)
- ❌ C++26 conversion (type safety doesn't affect performance)

### The Bottleneck IS:
- ✅ **VSync/Present Mode locking the frame rate**
- The app is waiting ~72ms in `vkAcquireNextImageKHR` for the next swapchain image
- This suggests `VK_PRESENT_MODE_FIFO_KHR` (vsync) is active and locked to 14 FPS

## Changes Made

### 1. Swapchain Present Mode Selection (Swapchain.cpp)
```cpp
// OLD: Always used FIFO (vsync locked)
createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

// NEW: Query available modes and prefer IMMEDIATE > MAILBOX > FIFO
- Query all available present modes from device
- Prefer VK_PRESENT_MODE_IMMEDIATE_KHR (no vsync, maximum FPS)
- Fallback to VK_PRESENT_MODE_MAILBOX_KHR (triple buffering)
- Last resort: VK_PRESENT_MODE_FIFO_KHR (vsync locked)
- Added logging to show which mode was selected
```

### 2. Enhanced Logging
- Added direct Android logging (`__android_log_print`) to Swapchain.cpp
- Logs should show available present modes and which one was selected
- **Issue**: Logs are not appearing, suggesting either:
  - Swapchain is not being recreated (using cached swapchain)
  - Logger is not initialized when swapchain is created
  - Logs are being filtered by Android

## Device Information
- Device: moto g34 5G
- Android: 15
- Display: 720x1600, 120Hz capable
- GPU: Adreno (Qualcomm)
- Vulkan: Supported

## Why 14 FPS Specifically?
- 72ms per frame = 1000ms / 72ms ≈ 13.9 FPS
- This is NOT a standard refresh rate (60Hz = 16.7ms, 120Hz = 8.3ms)
- Possible causes:
  1. Display is in power-saving mode (14Hz refresh rate)
  2. Android is throttling the app due to thermal/battery concerns
  3. VSync is locking to a non-standard refresh rate
  4. The swapchain present mode change didn't take effect

## Next Steps to Fix

### Option 1: Verify Present Mode Selection
```bash
# Check if swapchain logs appear
adb logcat -d | grep -i "swapchain\|present mode\|immediate\|mailbox"
```

### Option 2: Force Swapchain Recreation
- Trigger a swapchain recreation event (rotate device, minimize/restore app)
- This will force the new present mode selection code to run

### Option 3: Check Android Display Settings
```bash
# Check current display refresh rate
adb shell dumpsys display | grep -i "refresh\|fps\|mode"

# Check if power saving mode is active
adb shell dumpsys battery

# Check if app is being throttled
adb shell dumpsys activity | grep -i "secret"
```

### Option 4: Add Frame Rate Hint to Android
Add to AndroidManifest.xml:
```xml
<activity android:name=".MainActivity"
    ...
    android:preferredDisplayModeId="1">  <!-- Request highest refresh rate -->
```

Or programmatically in MainActivity.java:
```java
Window window = getWindow();
WindowManager.LayoutParams params = window.getAttributes();
params.preferredDisplayModeId = 1; // Highest refresh rate mode
window.setAttributes(params);
```

### Option 5: Bypass VSync Entirely
If the device doesn't support IMMEDIATE mode, we can:
1. Use `VK_PRESENT_MODE_FIFO_RELAXED_KHR` (allows tearing but no vsync wait)
2. Implement our own frame pacing with a separate thread
3. Use Android's Choreographer API for frame timing

## Files Modified
- `plugins/VulkanRenderer/src/Swapchain.cpp` - Present mode selection
- `core/src/World.cpp` - ECS optimization (completed earlier)

## Performance Metrics
- Before: 14 FPS, 72ms frame time
- After present mode change: **Still 14 FPS, 72ms frame time**
- **Conclusion**: Present mode change either didn't take effect or device doesn't support IMMEDIATE/MAILBOX modes

## Recommendation
The most likely issue is that the Android display is in a low-power mode or the device doesn't support non-vsync present modes. We need to:
1. Check device display settings
2. Add display mode hints to Android manifest
3. Verify which present modes are actually available on this device
4. Consider using Android's Choreographer API for frame pacing instead of relying on Vulkan present modes
