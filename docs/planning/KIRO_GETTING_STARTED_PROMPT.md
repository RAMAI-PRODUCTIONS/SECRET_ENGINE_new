# Kiro AI Assistant - C++26 Integration Getting Started Prompt

## Copy-Paste This Prompt to Start Working with Kiro

---

```
I'm working on SECRET_ENGINE, a game engine project with Android as the primary platform. I want to start adopting C++26 features incrementally based on the comprehensive plan in the cpp26-experimental branch.

CONTEXT:
- Project: SECRET_ENGINE (C++ game engine)
- Primary Platform: Android (NDK)
- Current Standard: C++20/23
- Branch: cpp26-experimental (already created with documentation)
- Build System: CMake
- Key Systems: ECS, Vulkan renderer, job system, physics, mega-geometry

WHAT'S ALREADY DONE:
✅ cpp26-experimental branch created
✅ Memory safety hardening enabled in core/CMakeLists.txt (-fhardened, sanitizers)
✅ CPP26Features.h created with feature detection and fallbacks
✅ 8 comprehensive planning documents created:
   - CPP26_INTEGRATION_PLAN.md (strategic overview)
   - CPP26_CRITICAL_ACTIONS.md (immediate actions)
   - CPP26_QUICK_START.md (developer guide)
   - CPP26_REFLECTION_PRACTICAL_GUIDE.md (CppCon 2025 examples)
   - CPP26_SUMMARY.md (executive summary)
   - CLAUDE_4.6_ANALYSIS_RESPONSE.md (expert review)
   - FINAL_RECOMMENDATION.md (decision matrix)

CURRENT PRIORITY: Phase 1 - Immediate Actions (Week 1)

TASKS FOR THIS SESSION:
1. Check Android NDK version and C++26 support
2. Verify memory hardening is working (test debug build)
3. Start using std::span in renderer buffer access
4. Plan std::inplace_vector prototype for ECS component queries

SPECIFIC QUESTIONS:
- What's my current Android NDK version and Clang version?
- Is C++26 support available in my NDK?
- Are the sanitizers working correctly in debug builds?
- Where should I start using std::span in the renderer?
- How should I prototype std::inplace_vector for ECS queries?

CONSTRAINTS:
- Android NDK lags Clang by 12-18 months (must use fallbacks)
- Every C++26 feature needs #if __cpp_feature guards
- No breaking changes to existing code
- Incremental adoption only
- Performance validation required

EXPECTED OUTCOMES:
- Know my Android NDK C++26 support status
- Verify hardening is catching bugs
- Identify 2-3 places to use std::span immediately
- Create prototype for std::inplace_vector in ECS
- Benchmark performance improvement

DOCUMENTATION AVAILABLE:
All planning docs are in docs/planning/ and docs/guides/
CPP26Features.h is in core/include/SecretEngine/

Please help me:
1. Check my Android NDK version and C++26 support
2. Verify the memory hardening is working
3. Identify where to use std::span in the renderer
4. Create a prototype for std::inplace_vector in ECS component queries
5. Set up benchmarking for performance validation

Let's start with checking the Android NDK version and C++26 support status.
```

---

## Alternative Prompts for Different Phases

### Phase 2: Reflection Prototype (Month 2)

```
I'm ready to start prototyping C++26 reflection for SECRET_ENGINE based on the CppCon 2025 examples in CPP26_REFLECTION_PRACTICAL_GUIDE.md.

CONTEXT:
- Android NDK version: [YOUR_VERSION]
- C++26 support status: [AVAILABLE/PARTIAL/NOT_AVAILABLE]
- Branch: cpp26-experimental
- Target: Implement reflection logger and simple serialization

TASKS:
1. Implement reflection logger from CppCon 2025 example
2. Test with TransformComponent
3. Create automatic JSON serialization for components
4. Implement dual-path with macro fallback for Android
5. Benchmark compile-time impact

FILES TO MODIFY:
- plugins/DebugPlugin/src/DebugPlugin.cpp (reflection logger)
- core/src/World.cpp (serialization)
- core/include/SecretEngine/Components.h (component definitions)

Please help me implement the reflection logger first, then move to serialization.
```

---

### Phase 3: std::inplace_vector Integration (Month 1-2)

```
I want to integrate std::inplace_vector into SECRET_ENGINE's ECS system to eliminate per-frame heap allocations.

CONTEXT:
- Current approach: std::vector with reserve() for component queries
- Target: Replace with std::inplace_vector for 20-50% performance win
- Fallback: std::vector for Android NDK (already in CPP26Features.h)

TASKS:
1. Identify all per-frame std::vector allocations in ECS
2. Replace with std::inplace_vector<T, N> where N is known
3. Benchmark performance before/after
4. Test on Android with fallback
5. Document performance improvements

FILES TO ANALYZE:
- core/src/World.cpp (component queries)
- plugins/VulkanRenderer/src/MegaGeometryRenderer.cpp (culling)
- plugins/PhysicsPlugin/src/PhysicsPlugin.cpp (collision detection)

Please help me:
1. Find all per-frame vector allocations
2. Determine appropriate capacity N for each use case
3. Implement std::inplace_vector with fallback
4. Set up performance benchmarks
```

---

### Phase 4: SIMD Math Library (Month 3)

```
I want to add SIMD parallelism to SECRET_ENGINE's math library using std::simd.

CONTEXT:
- Current: Scalar math operations in core/include/SecretEngine/Math.h
- Target: 2-8x speedup with SIMD
- Fallback: Scalar operations for platforms without SIMD

TASKS:
1. Identify hot math operations (profiling data available)
2. Implement SIMD versions with std::simd
3. Keep scalar fallbacks
4. Benchmark performance improvement
5. Test on all platforms (desktop, Android)

FILES TO MODIFY:
- core/include/SecretEngine/Math.h (SIMD math)
- plugins/VulkanRenderer/src/MegaGeometryRenderer.cpp (SIMD culling)
- plugins/PhysicsPlugin/src/CollisionDetection.h (SIMD broadphase)

Please help me:
1. Profile current math operations
2. Implement SIMD Vec3/Mat4 operations
3. Add SIMD frustum culling
4. Benchmark and validate performance
```

---

### Phase 5: Contracts Strategy (Month 2)

```
I need to define a contract violation handler strategy for SECRET_ENGINE before adding contracts to APIs.

CONTEXT:
- Target: Add contracts to public APIs for safety
- Risk: Medium-high (violation handler semantics matter)
- Need: Define strategy before implementation

TASKS:
1. Review contract violation handler options
2. Define strategy for debug/release/shipping builds
3. Create contract policy document
4. Prototype on a few APIs
5. Get team feedback

QUESTIONS TO ANSWER:
- What should happen on contract violation? (terminate/log/throw)
- Different behavior for debug vs release?
- How to handle contract violations in plugins?
- Performance impact acceptable?
- Team training needed?

Please help me:
1. Analyze contract violation handler options
2. Recommend strategy for game engine
3. Create policy document
4. Prototype on sample APIs
```

---

### Phase 6: std::execution Design (Month 4-6)

```
I need to design the error handling strategy for migrating SECRET_ENGINE's job system to std::execution.

CONTEXT:
- Current: Custom job system with simple error handling
- Target: std::execution with structured concurrency
- Complexity: Vulkan errors + CPU sync + timeouts must map to sender chains
- Timeline: 6-9 month design + shadow implementation

TASKS:
1. Map current error domains to sender error channels
2. Design cancellation strategy (frame timeout)
3. Design GPU sync integration (fence waits)
4. Plan backward compatibility during migration
5. Define testing strategy for behavioral parity

CRITICAL QUESTIONS:
- How do Vulkan errors map to sender error channels?
- How does frame timeout cancel in-flight work?
- How do fence waits integrate with sender chains?
- Can old job system API coexist during migration?
- How to prove behavioral parity?

Please help me:
1. Analyze current error handling patterns
2. Design error mapping strategy
3. Create migration plan
4. Define success criteria
```

---

## Quick Reference: Common Kiro Commands

### Check Status
```
Show me the current state of the cpp26-experimental branch
List all C++26 related files in docs/
What's the status of memory hardening in CMakeLists.txt?
```

### Code Analysis
```
Analyze core/src/World.cpp for std::vector allocations that could use std::inplace_vector
Find all places in the renderer that could benefit from std::span
Show me where contracts would add the most value in public APIs
```

### Implementation
```
Implement reflection logger for TransformComponent based on CppCon 2025 example
Add std::inplace_vector to ECS component queries with fallback
Create SIMD version of Vec3 math operations with scalar fallback
```

### Testing & Validation
```
Create benchmark for std::inplace_vector vs std::vector in ECS queries
Test memory hardening by introducing an uninitialized variable
Verify CPP26Features.h fallbacks work on Android NDK
```

### Documentation
```
Update CPP26_INTEGRATION_PLAN.md with current progress
Document the contract violation handler strategy we decided on
Create team training materials for C++26 reflection
```

---

## Tips for Working with Kiro

1. **Be Specific**: Reference exact files and line numbers when possible
2. **Provide Context**: Mention which phase/task you're working on
3. **Ask for Validation**: Request benchmarks and tests
4. **Incremental Changes**: Don't ask for everything at once
5. **Reference Documentation**: Point to the planning docs we created

---

## Example Session Flow

```
Session 1: Android NDK Check
→ Check NDK version
→ Test C++26 support
→ Verify hardening works
→ Document findings

Session 2: std::span Integration
→ Identify renderer buffer access points
→ Replace raw pointers with std::span
→ Test and validate
→ Commit changes

Session 3: std::inplace_vector Prototype
→ Find per-frame allocations
→ Implement with fallback
→ Benchmark performance
→ Document results

Session 4: Reflection Logger
→ Implement CppCon 2025 example
→ Test with components
→ Add to debug plugin
→ Verify on Android

... continue incrementally
```

---

*Copy the main prompt above and paste it to Kiro to get started!*
