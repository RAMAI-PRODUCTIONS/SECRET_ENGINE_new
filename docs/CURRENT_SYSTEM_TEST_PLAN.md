# Current System Testing Plan

## System Status
✅ v7.3 Level System implemented
✅ Per-instance colors working
✅ Shader compilation fixed
✅ FPS: 560+ average
✅ 11 instances rendering

## Testing Checklist

### 1. Visual Verification
- [ ] All 11 entities visible on screen
- [ ] Each mesh has unique color from JSON
- [ ] Ground plane visible (greenish tint)
- [ ] Trigger cube visible (bright yellow)
- [ ] Texture detail preserved
- [ ] No visual artifacts or glitches

**Expected Colors**:
- Ground: Greenish (0.3, 0.5, 0.3)
- Object 1 @ (15, 0, 10): Red (0.8, 0.2, 0.2)
- Object 2 @ (-20, 0, 15): Green (0.2, 0.8, 0.2)
- Object 3 @ (25, 0, -18): Blue (0.2, 0.2, 0.8)
- Object 4 @ (-15, 0, -25): Yellow (0.8, 0.8, 0.2)
- Object 5 @ (0, 0, 30): Magenta (0.8, 0.2, 0.8)
- Object 6 @ (35, 0, 5): Cyan (0.2, 0.8, 0.8)
- Trigger @ (40, 5, 0): Bright Yellow (1.0, 1.0, 0.0)

### 2. Performance Testing
- [ ] FPS consistently above 500
- [ ] GPU time under 2ms
- [ ] No frame drops or stuttering
- [ ] Smooth camera movement
- [ ] Joystick responsive

**Current Metrics**:
- FPS: 560+ average
- GPU: 1.3ms per frame
- CPU: 0.1ms per frame
- Instances: 11
- Triangles: 66,550
- Draw calls: 2

### 3. Player Movement
- [ ] Joystick controls working
- [ ] Player can move forward/backward
- [ ] Player can strafe left/right
- [ ] Camera rotation working
- [ ] No collision issues
- [ ] Movement speed feels good

### 4. Level System
- [ ] levelone loads on startup
- [ ] All entities from JSON spawned
- [ ] Transforms correct (position, rotation, scale)
- [ ] Colors from material_params applied
- [ ] No missing entities
- [ ] No duplicate entities

### 5. Trigger System
- [ ] Trigger cube visible at (40, 5, 0)
- [ ] Trigger has correct size (3x10x3)
- [ ] Trigger color is bright yellow
- [ ] Player can approach trigger
- [ ] (Future) Trigger activates on entry

### 6. Camera System
- [ ] Camera positioned correctly
- [ ] Can see all objects in scene
- [ ] FOV appropriate
- [ ] Near/far planes correct
- [ ] No clipping issues

### 7. Shader System
- [ ] Shaders compile correctly
- [ ] compile_shaders.bat works
- [ ] Shader changes take effect after recompile
- [ ] No shader errors in logs
- [ ] Colors render correctly

### 8. Build System
- [ ] Gradle build succeeds
- [ ] APK installs on device
- [ ] No compilation errors
- [ ] Assets copied correctly
- [ ] Shaders included in APK

### 9. Memory & Stability
- [ ] No memory leaks
- [ ] App doesn't crash
- [ ] Can run for extended period
- [ ] No GPU errors
- [ ] No validation layer errors

### 10. Multi-Level Support
- [ ] levelone loads correctly
- [ ] leveltwo defined in JSON
- [ ] Can switch between levels (future)
- [ ] Level data parsed correctly
- [ ] No level loading errors

## Known Issues
None currently!

## Performance Baseline

### Device: moto g34 5G (Android 15)
- **FPS**: 560+ average
- **GPU**: 1.3ms per frame
- **CPU**: 0.1ms per frame
- **Memory**: TBD
- **Battery**: TBD

### Rendering Stats
- **Instances**: 11
- **Triangles**: 66,550
- **Draw Calls**: 2
- **Vertices**: ~20,000
- **Texture Memory**: TBD

## Test Scenarios

### Scenario 1: Basic Rendering
1. Launch app
2. Verify all 11 entities visible
3. Check colors match JSON
4. Verify FPS > 500
5. Take screenshot

### Scenario 2: Player Movement
1. Use joystick to move forward
2. Move to each colored object
3. Verify no collision issues
4. Check FPS remains stable
5. Return to start position

### Scenario 3: Camera Control
1. Rotate camera 360 degrees
2. Look up and down
3. Verify all objects visible from different angles
4. Check for culling issues
5. Verify smooth rotation

### Scenario 4: Trigger Approach
1. Move player toward trigger (40, 5, 0)
2. Verify trigger visible from distance
3. Approach trigger cube
4. Note distance when trigger becomes clear
5. (Future) Test trigger activation

### Scenario 5: Extended Play
1. Run app for 5 minutes
2. Move around continuously
3. Monitor FPS stability
4. Check for memory leaks
5. Verify no crashes

### Scenario 6: Level Reload
1. Force stop app
2. Restart app
3. Verify level loads correctly
4. Check all entities present
5. Verify colors correct

## Success Criteria

✅ **Visual Quality**
- All colors visible and correct
- Texture detail preserved
- No visual glitches

✅ **Performance**
- FPS > 500 consistently
- GPU < 2ms per frame
- No stuttering

✅ **Functionality**
- All entities load from JSON
- Colors from material_params work
- Player movement responsive

✅ **Stability**
- No crashes
- No memory leaks
- Runs for extended periods

## Next Steps After Testing

1. **If all tests pass**:
   - Document current system
   - Create tutorial/guide
   - Plan next features

2. **If issues found**:
   - Document issues
   - Prioritize fixes
   - Create fix plan

3. **Future enhancements**:
   - Trigger activation
   - Level switching
   - More entities
   - Vertex lighting (planned)

## Testing Log

### Test Session 1: [Date]
- Tester: 
- Device: moto g34 5G
- Build: 
- Results:
  - Visual: 
  - Performance: 
  - Functionality: 
  - Stability: 
- Issues Found:
- Notes:

---

**Status**: Ready for Testing
**Priority**: High
**Estimated Time**: 30-60 minutes
