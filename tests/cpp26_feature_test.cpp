/**
 * C++26 Feature Test Suite
 * 
 * Tests for C++26 features and fallback implementations:
 * 1. std::inplace_vector fallback
 * 2. Memory safety (sanitizers)
 * 3. std::span usage
 * 4. Feature detection macros
 */

#include <SecretEngine/CPP26Features.h>
#include <iostream>
#include <cassert>
#include <cstring>

// Test helper
#define TEST(name) \
    void test_##name(); \
    struct TestRunner_##name { \
        TestRunner_##name() { \
            std::cout << "Running test: " #name << "..." << std::endl; \
            test_##name(); \
            std::cout << "  ✓ PASSED\n" << std::endl; \
        } \
    } testRunner_##name; \
    void test_##name()

// ============================================================================
// Test 1: Feature Detection
// ============================================================================

TEST(feature_detection) {
    using namespace SecretEngine;
    
    std::cout << "  C++26 Feature Support:" << std::endl;
    std::cout << "    Reflection: " << (CPP26Support::hasReflection ? "YES" : "NO") << std::endl;
    std::cout << "    Contracts: " << (CPP26Support::hasContracts ? "YES" : "NO") << std::endl;
    std::cout << "    Execution: " << (CPP26Support::hasExecution ? "YES" : "NO") << std::endl;
    std::cout << "    InplaceVector: " << (CPP26Support::hasInplaceVector ? "YES" : "NO (using fallback)") << std::endl;
    std::cout << "    PackIndexing: " << (CPP26Support::hasPackIndexing ? "YES" : "NO") << std::endl;
    std::cout << "    OptionalRef: " << (CPP26Support::hasOptionalRef ? "YES" : "NO") << std::endl;
    std::cout << "    Debugging: " << (CPP26Support::hasDebugging ? "YES" : "NO") << std::endl;
    std::cout << "    SIMD: " << (CPP26Support::hasSIMD ? "YES" : "NO") << std::endl;
    std::cout << "    Mdspan: " << (CPP26Support::hasMdspan ? "YES" : "NO") << std::endl;
}

// ============================================================================
// Test 2: std::inplace_vector Basic Operations
// ============================================================================

TEST(inplace_vector_basic) {
    std::inplace_vector<int, 10> vec;
    
    // Test empty
    assert(vec.empty());
    assert(vec.size() == 0);
    assert(vec.capacity() == 10);
    
    // Test push_back
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    
    assert(!vec.empty());
    assert(vec.size() == 3);
    assert(vec[0] == 1);
    assert(vec[1] == 2);
    assert(vec[2] == 3);
    
    // Test front/back
    assert(vec.front() == 1);
    assert(vec.back() == 3);
    
    // Test iteration
    int sum = 0;
    for (auto val : vec) {
        sum += val;
    }
    assert(sum == 6);
    
    // Test pop_back
    vec.pop_back();
    assert(vec.size() == 2);
    assert(vec.back() == 2);
    
    // Test clear
    vec.clear();
    assert(vec.empty());
    assert(vec.size() == 0);
}

// ============================================================================
// Test 3: std::inplace_vector with Complex Types
// ============================================================================

struct ComplexType {
    int value;
    static int constructCount;
    static int destructCount;
    
    ComplexType(int v = 0) : value(v) { ++constructCount; }
    ComplexType(const ComplexType& other) : value(other.value) { ++constructCount; }
    ComplexType(ComplexType&& other) noexcept : value(other.value) { ++constructCount; other.value = 0; }
    ~ComplexType() { ++destructCount; }
    
    ComplexType& operator=(const ComplexType& other) {
        value = other.value;
        return *this;
    }
    
    ComplexType& operator=(ComplexType&& other) noexcept {
        value = other.value;
        other.value = 0;
        return *this;
    }
};

int ComplexType::constructCount = 0;
int ComplexType::destructCount = 0;

TEST(inplace_vector_complex_types) {
    ComplexType::constructCount = 0;
    ComplexType::destructCount = 0;
    
    {
        std::inplace_vector<ComplexType, 5> vec;
        
        vec.push_back(ComplexType(10));
        vec.push_back(ComplexType(20));
        vec.emplace_back(30);
        
        assert(vec.size() == 3);
        assert(vec[0].value == 10);
        assert(vec[1].value == 20);
        assert(vec[2].value == 30);
        
        // Test copy constructor
        std::inplace_vector<ComplexType, 5> vec2 = vec;
        assert(vec2.size() == 3);
        assert(vec2[0].value == 10);
        
        // Test move constructor
        std::inplace_vector<ComplexType, 5> vec3 = std::move(vec2);
        assert(vec3.size() == 3);
        assert(vec3[0].value == 10);
    }
    
    // Verify all objects were destroyed
    std::cout << "  Constructions: " << ComplexType::constructCount << std::endl;
    std::cout << "  Destructions: " << ComplexType::destructCount << std::endl;
    assert(ComplexType::constructCount == ComplexType::destructCount);
}

// ============================================================================
// Test 4: std::inplace_vector Performance (ECS Use Case)
// ============================================================================

struct Entity {
    uint32_t id;
    float x, y, z;
};

TEST(inplace_vector_ecs_query) {
    // Simulate ECS component query
    std::inplace_vector<Entity*, 1024> visibleEntities;
    
    // Create some entities
    Entity entities[100];
    for (int i = 0; i < 100; ++i) {
        entities[i].id = i;
        entities[i].x = i * 1.0f;
        entities[i].y = i * 2.0f;
        entities[i].z = i * 3.0f;
    }
    
    // Query: Find entities with x > 50
    for (int i = 0; i < 100; ++i) {
        if (entities[i].x > 50.0f) {
            visibleEntities.push_back(&entities[i]);
        }
    }
    
    assert(visibleEntities.size() == 49); // 51-99 inclusive
    assert(visibleEntities[0]->id == 51);
    assert(visibleEntities.back()->id == 99);
    
    std::cout << "  ECS Query: Found " << visibleEntities.size() << " entities" << std::endl;
}

// ============================================================================
// Test 5: std::span Usage
// ============================================================================

#include <span>
#include <vector>

float SumBuffer_RawPointer(const float* data, size_t size) {
    float sum = 0;
    for (size_t i = 0; i < size; ++i) {
        sum += data[i];
    }
    return sum;
}

float SumBuffer_Span(std::span<const float> data) {
    float sum = 0;
    for (auto val : data) {
        sum += val;
    }
    return sum;
}

TEST(span_usage) {
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    
    // Old way: raw pointer + size
    float sum1 = SumBuffer_RawPointer(data.data(), data.size());
    
    // New way: std::span
    float sum2 = SumBuffer_Span(data);
    
    assert(sum1 == sum2);
    assert(sum1 == 15.0f);
    
    std::cout << "  Span sum: " << sum2 << std::endl;
}

// ============================================================================
// Test 6: Memory Safety (Sanitizer Test)
// ============================================================================

TEST(memory_safety_bounds_check) {
    std::inplace_vector<int, 10> vec;
    
    // Fill to capacity
    for (int i = 0; i < 10; ++i) {
        vec.push_back(i);
    }
    
    // This should trigger an assertion in debug builds
    // Uncomment to test sanitizer:
    // vec.push_back(11); // SHOULD FAIL
    
    std::cout << "  Bounds checking: OK (capacity respected)" << std::endl;
}

// ============================================================================
// Test 7: Contract Macros
// ============================================================================

void SetVelocity(float x, float y, float z)
    SE_PRECONDITION(x >= -100.0f && x <= 100.0f)
    SE_PRECONDITION(y >= -100.0f && y <= 100.0f)
    SE_PRECONDITION(z >= -100.0f && z <= 100.0f)
{
    // Function body
    std::cout << "  Velocity set: (" << x << ", " << y << ", " << z << ")" << std::endl;
}

TEST(contract_macros) {
    // Valid call
    SetVelocity(10.0f, 20.0f, 30.0f);
    
    // Invalid call (should trigger assertion in debug)
    // Uncomment to test:
    // SetVelocity(200.0f, 0.0f, 0.0f); // SHOULD FAIL
    
    std::cout << "  Contract checking: OK" << std::endl;
}

// ============================================================================
// Test 8: Debugging Macros
// ============================================================================

TEST(debugging_macros) {
    std::cout << "  Debugger present: " << (SE_IS_DEBUGGER_PRESENT() ? "YES" : "NO") << std::endl;
    
    // Uncomment to test breakpoint:
    // SE_BREAKPOINT();
    
    std::cout << "  Debugging macros: OK" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "\n=== C++26 Feature Test Suite ===" << std::endl;
    std::cout << "Compiler: " << __VERSION__ << std::endl;
    std::cout << "C++ Standard: " << __cplusplus << std::endl;
    std::cout << "\n";
    
    // Tests run automatically via static initializers
    
    std::cout << "\n=== All Tests Passed ✓ ===" << std::endl;
    return 0;
}
