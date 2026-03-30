# Test C++26 Features on Android Device

**Prompt for Kiro AI Assistant**

---

## TASK

Test all C++26 features on Android device using logcat to verify:
1. Memory sanitizers are working (catching bugs)
2. std::span is functioning correctly
3. std::inplace_vector is working
4. Feature detection is accurate
5. No performance regressions

---

## STEPS

### 1. Build Debug APK
```bash
cd android
./gradlew assembleDebug
```

### 2. Install on Device
```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

### 3. Start Logcat Monitoring
```bash
adb logcat -c  # Clear logs
adb logcat | grep -E "SecretEngine|sanitizer|ASAN|UBSAN|CPP26|inplace_vector|span"
```

### 4. Launch App
```bash
adb shell am start -n com.secretengine.game/.MainActivity
```

---

## WHAT TO VERIFY

### A. Sanitizer Initialization
**Look for**: AddressSanitizer and UndefinedBehaviorSanitizer startup messages

**Expected**:
```
==12345==AddressSanitizer: detected memory leaks
==12345==UndefinedBehaviorSanitizer initialized
```

### B. Feature Detection
**Look for**: CPP26Support logging

**Expected**:
```
SecretEngine: C++26 Features:
SecretEngine:   std::span: Available
SecretEngine:   std::inplace_vector: Fallback
SecretEngine:   Memory hardening: Enabled
```

### C. std::span Usage
**Look for**: Buffer access logs from LightingSystem, MaterialSystem

**Expected**:
```
LightingSystem: Buffer size: X lights (std::span)
MaterialSystem: Buffer size: Y materials (std::span)
```

### D. Physics System
**Look for**: Raycast and collision logs

**Expected**:
```
PhysicsPlugin: Raycast origin=[0,0,0] direction=[0,-1,0] (std::span)
PhysicsPlugin: Collision detected (std::span parameters)
```

### E. Performance
**Look for**: Frame time logs

**Expected**:
```
Renderer: Frame time: 16ms (no regression)
```

---

## TEST SCENARIOS

### Test 1: Normal Operation
- Launch app
- Move camera
- Verify no crashes
- Check frame rate

### Test 2: Trigger Sanitizer (Intentional Bug)
Add temporary test code:
```cpp
void TestSanitizer() {
    int arr[10];
    arr[15] = 42;  // Out of bounds - should be caught!
}
```

**Expected**: ASan catches and logs the error

### Test 3: std::span Bounds Check
Add temporary test code:
```cpp
void TestSpanBounds() {
    std::span<float, 3> pos = GetPosition();
    float x = pos[5];  // Out of bounds - should assert in debug!
}
```

**Expected**: Debug assertion or sanitizer error

### Test 4: std::inplace_vector Capacity
Add temporary test code:
```cpp
void TestInplaceVector() {
    std::inplace_vector<int, 10> vec;
    for (int i = 0; i < 15; ++i) {
        vec.push_back(i);  // Exceeds capacity - should assert!
    }
}
```

**Expected**: Assertion failure logged

---

## SUCCESS CRITERIA

✅ App launches without crashes
✅ Sanitizers initialize correctly
✅ Feature detection reports correct status
✅ std::span functions work correctly
✅ Physics system uses std::span parameters
✅ Frame rate is stable (16ms target)
✅ No memory leaks detected
✅ Bounds violations are caught in debug

---

## FAILURE INDICATORS

❌ App crashes on launch
❌ Sanitizers not initialized
❌ Segmentation faults
❌ Frame rate drops significantly
❌ Memory leaks reported
❌ Bounds violations not caught

---

## LOGCAT COMMANDS

### Full Log
```bash
adb logcat
```

### Filtered for SecretEngine
```bash
adb logcat | grep SecretEngine
```

### Filtered for Sanitizers
```bash
adb logcat | grep -E "sanitizer|ASAN|UBSAN"
```

### Filtered for C++26
```bash
adb logcat | grep -E "CPP26|span|inplace_vector"
```

### Save to File
```bash
adb logcat > cpp26_test_log.txt
```

---

## REPORT FORMAT

After testing, create report with:
1. Device info (model, Android version)
2. Build info (NDK version, Clang version)
3. Test results (pass/fail for each scenario)
4. Logcat excerpts showing key events
5. Performance metrics (frame times)
6. Any issues found

---

**Run this test and report results.**
