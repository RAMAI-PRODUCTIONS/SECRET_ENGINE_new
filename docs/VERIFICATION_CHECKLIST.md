# Instance Cleanup Verification Checklist

## What to Verify

### 1. Build Success
- [  ] Clean build completes without errors
- [  ] All C++ files compile successfully
- [  ] APK installs on device

### 2. App Launch
- [  ] App launches without crashes
- [  ] Initial level loads correctly
- [  ] Renderer initializes properly

### 3. Instance Cleanup - Initial Load
Look for these log messages when the app first starts:

```
VulkanRenderer: ✓ Cleared all instances: X instances removed
V73LevelSystemPlugin: Clearing all renderer instances before loading new level...
V73LevelSystemPlugin: Level loaded successfully
```

### 4. Instance Cleanup - Level Change
When changing levels, verify:

```
V73LevelSystemPlugin: Clearing all renderer instances before loading new level...
MegaGeometryRenderer: ✓ Cleared all instances: X instances removed
V73LevelSystemPlugin: Loading level: [new_level_path]
```

### 5. Instance Count Verification
Check that instance counts reset properly:

**Before cleanup:**
```
MegaGeometryRenderer: totalInstances=4000 (or whatever was loaded)
```

**After cleanup:**
```
MegaGeometryRenderer: ✓ Cleared all instances: 4000 instances removed
MegaGeometryRenderer: totalInstances=0
```

**After new level loads:**
```
MegaGeometryRenderer: totalInstances=[new_count] (should be only from new level)
```

### 6. Visual Verification
- [  ] Screenshot shows app running
- [  ] No visual artifacts or crashes
- [  ] Level geometry renders correctly

### 7. Memory Verification
Check that memory doesn't accumulate:
- [  ] VRAM usage resets between level changes
- [  ] No memory leaks over multiple level changes

## Logcat Filters

Use these filters to find relevant messages:

```bash
# Instance cleanup messages
adb logcat | grep -i "cleared all instances"

# Level system messages  
adb logcat | grep -i "V73LevelSystemPlugin"

# Renderer messages
adb logcat | grep -i "VulkanRenderer\|MegaGeometryRenderer"

# Instance count messages
adb logcat | grep -i "totalInstances\|instanceCount"
```

## Expected Behavior

### Scenario 1: App Launch
1. App starts
2. Renderer initializes
3. **ClearAllInstances() called** (clears any stale data)
4. Initial level loads
5. Instances added for initial level only

### Scenario 2: Level Change
1. User triggers level change
2. **ClearAllInstances() called** (removes all old instances)
3. Old level unloads
4. New level loads
5. Instances added for new level only
6. **Total instance count = only new level instances**

### Scenario 3: Multiple Level Changes
1. Load Level A → 200 instances
2. Change to Level B → **Clear 200** → Load 300 instances
3. Change to Level C → **Clear 300** → Load 150 instances
4. **At no point should instances accumulate beyond current level**

## Common Issues to Watch For

### ❌ Issue: Instances Not Clearing
**Symptom:** Instance count keeps growing (4000 → 4200 → 4500...)
**Expected:** Instance count resets (4000 → 0 → 200)
**Fix:** Check that ClearAllInstances() is being called

### ❌ Issue: Cleanup Not Called
**Symptom:** No "Cleared all instances" messages in log
**Expected:** Message appears on every level load/unload
**Fix:** Verify level system is calling renderer->ClearAllInstances()

### ❌ Issue: Partial Cleanup
**Symptom:** Some instances remain after cleanup
**Expected:** All instances removed (count = 0)
**Fix:** Check MegaGeometryRenderer::ClearAllInstances() implementation

## Success Criteria

✅ **PASS** if:
- Instance count resets to 0 on level change
- New level starts with clean slate
- No accumulation over multiple level changes
- Log messages confirm cleanup is happening

❌ **FAIL** if:
- Instances accumulate across level changes
- No cleanup messages in logs
- Memory keeps growing
- Crashes on level change

## Test Procedure

1. **Launch app** → Capture initial state
2. **Wait 10 seconds** → Let level load completely
3. **Capture logcat** → Look for cleanup messages
4. **Take screenshot** → Verify visual state
5. **Change level** (if possible) → Verify cleanup happens
6. **Repeat step 5** multiple times → Verify no accumulation

## Files to Review

After running verification:
- `screenshot_initial.png` - Visual confirmation
- `logcat_output.txt` - Detailed logs
- Look for patterns matching the expected behavior above
