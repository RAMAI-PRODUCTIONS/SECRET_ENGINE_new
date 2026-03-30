# C++26 Week 1 Quick Start Guide

## Overview
This guide walks you through Phase 1 of C++26 adoption in SECRET_ENGINE.

## Your Environment
- **NDK**: 29.0.14206865 (Clang 21.0.0)
- **C++26 Support**: Partial (early adopter status)
- **Memory Hardening**: ✅ Enabled
- **Fallback Strategy**: ✅ Dual-path ready

## Day 1: Verification ✅ COMPLETE

### What We Discovered
1. Clang 21 in Android NDK 29 provides early C++26 support
2. Memory hardening is enabled in debug builds
3. 4 high-priority areas for std::span integration
4. std::inplace_vector needs fallback implementation (now complete)

## Day 2: Test Memory Hardening

### Build and Test
```bash
cd android
./gradlew assembleDebug

# Install on device/emulator
adb install -r app/build/outputs/apk/debug/app-debug.apk

# Monitor for sanitizer output
adb logcat | grep -i "sanitizer\|asan\|ubsan"
```

### Expected Output
Look for sanitizer initialization messages:
```
==12345==AddressSanitizer: detected memory leaks
==12345==UndefinedBehaviorSanitizer: undefined-behavior
```

## Day 3-4: Implement std::span

### Priority 1: Lighting System

See `core/include/SecretEngine/ILightingSystem.h`

### Priority 2: Material System
See `core/include/SecretEngine/IMaterialSystem.h`

### Priority 3: MegaGeometryRenderer
See `plugins/VulkanRenderer/src/MegaGeometryRenderer.cpp`

## Day 5-6: Test std::inplace_vector

### Run Feature Tests
```bash
# Build test suite
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make cpp26_feature_test

# Run tests
./tests/cpp26_feature_test
```

### Expected Output
```
=== C++26 Feature Test Suite ===
Running test: feature_detection...
  ✓ PASSED
Running test: inplace_vector_basic...
  ✓ PASSED
...
=== All Tests Passed ✓ ===
```

## Day 7: Benchmark

### Run Performance Tests
```bash
make cpp26_benchmarks
./tests/cpp26_benchmarks
```

## Next Steps
- Week 2: Implement std::span in all buffer interfaces
- Week 3: Integrate std::inplace_vector into ECS queries
- Week 4: Performance validation and optimization

## Resources
- Status Report: `docs/planning/CPP26_PHASE1_STATUS_REPORT.md`
- Integration Plan: `docs/planning/CPP26_INTEGRATION_PLAN.md`
- Feature Header: `core/include/SecretEngine/CPP26Features.h`
