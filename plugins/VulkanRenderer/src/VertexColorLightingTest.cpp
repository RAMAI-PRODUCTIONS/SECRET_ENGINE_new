// ============================================================================
// VERTEX COLOR LIGHTING - VERIFICATION TEST
// Quick test to verify the lighting system is working
// ============================================================================

#include "SimpleVertexLighting.h"
#include "Pipeline3D.h"
#include <SecretEngine/ILogger.h>
#include <SecretEngine/ICore.h>

namespace SecretEngine {

// ============================================================================
// VERIFICATION TEST
// ============================================================================
bool VerifyVertexColorLighting(ICore* core) {
    auto logger = core ? core->GetLogger() : nullptr;
    
    if (logger) {
        logger->LogInfo("VertexLighting", "=== VERTEX COLOR LIGHTING VERIFICATION ===");
    }
    
    // Test 1: Create lighting system
    SimpleVertexLighting lighting;
    if (logger) {
        logger->LogInfo("VertexLighting", "✓ Test 1: Lighting system created");
    }
    
    // Test 2: Add lights
    lighting.SetAmbient(glm::vec3(0.2f, 0.2f, 0.3f), 0.1f);
    lighting.AddDirectionalLight(
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f
    );
    lighting.AddPointLight(
        glm::vec3(0.0f, 0.0f, 2.0f),
        glm::vec3(1.0f, 0.5f, 0.2f),
        3.0f,
        10.0f
    );
    if (logger) {
        logger->LogInfo("VertexLighting", "✓ Test 2: Lights added (1 directional, 1 point)");
    }
    
    // Test 3: Compute lighting for test vertex
    glm::vec3 testPos(0.0f, 0.0f, 0.0f);
    glm::vec3 testNormal(0.0f, 0.0f, 1.0f);
    glm::vec3 result = lighting.ComputeVertexLighting(testPos, testNormal);
    
    if (logger) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "✓ Test 3: Lighting computed - R=%.2f G=%.2f B=%.2f",
            result.r, result.g, result.b);
        logger->LogInfo("VertexLighting", msg);
    }
    
    // Test 4: Pack/Unpack R11G11B10F
    uint32_t packed = SimpleVertexLighting::PackR11G11B10F(result);
    glm::vec3 unpacked = SimpleVertexLighting::UnpackR11G11B10F(packed);
    
    float error = glm::length(result - unpacked);
    if (error < 0.1f) {
        if (logger) {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                "✓ Test 4: Pack/Unpack successful (error=%.4f)", error);
            logger->LogInfo("VertexLighting", msg);
        }
    } else {
        if (logger) {
            logger->LogError("VertexLighting", "✗ Test 4: Pack/Unpack error too high!");
        }
        return false;
    }
    
    // Test 5: Verify vertex format size
    size_t vertexSize = sizeof(Vertex3DNitro);
    if (vertexSize == 20) {
        if (logger) {
            logger->LogInfo("VertexLighting", "✓ Test 5: Vertex format correct (20 bytes)");
        }
    } else {
        if (logger) {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                "✗ Test 5: Vertex format wrong size (%zu bytes, expected 20)",
                vertexSize);
            logger->LogError("VertexLighting", msg);
        }
        return false;
    }
    
    // Test 6: Create test mesh and compute lighting
    const uint32_t testVertexCount = 8; // Cube vertices
    Vertex3DNitro testVertices[testVertexCount];
    
    // Initialize test cube vertices
    glm::vec3 cubePositions[8] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
    };
    
    for (uint32_t i = 0; i < testVertexCount; ++i) {
        // Encode position
        testVertices[i].pos[0] = static_cast<int16_t>(cubePositions[i].x * 4096.0f);
        testVertices[i].pos[1] = static_cast<int16_t>(cubePositions[i].y * 4096.0f);
        testVertices[i].pos[2] = static_cast<int16_t>(cubePositions[i].z * 4096.0f);
        testVertices[i].pos[3] = 0;
        
        // Encode normal (pointing up)
        testVertices[i].norm[0] = 0;
        testVertices[i].norm[1] = 0;
        testVertices[i].norm[2] = 127;
        testVertices[i].norm[3] = 0;
        
        // UV (unused)
        testVertices[i].uv[0] = 0;
        testVertices[i].uv[1] = 0;
        
        // Compute lighting
        glm::vec3 pos = cubePositions[i];
        glm::vec3 normal(0.0f, 0.0f, 1.0f);
        glm::vec3 color = lighting.ComputeVertexLighting(pos, normal);
        testVertices[i].vertexColor = SimpleVertexLighting::PackR11G11B10F(color);
    }
    
    if (logger) {
        logger->LogInfo("VertexLighting", "✓ Test 6: Test mesh lighting computed (8 vertices)");
    }
    
    // Test 7: Verify all vertex colors are non-zero
    bool allNonZero = true;
    for (uint32_t i = 0; i < testVertexCount; ++i) {
        if (testVertices[i].vertexColor == 0) {
            allNonZero = false;
            break;
        }
    }
    
    if (allNonZero) {
        if (logger) {
            logger->LogInfo("VertexLighting", "✓ Test 7: All vertex colors non-zero");
        }
    } else {
        if (logger) {
            logger->LogError("VertexLighting", "✗ Test 7: Some vertex colors are zero!");
        }
        return false;
    }
    
    // All tests passed!
    if (logger) {
        logger->LogInfo("VertexLighting", "=== ALL TESTS PASSED ===");
        logger->LogInfo("VertexLighting", "✅ Vertex color lighting system is READY");
    }
    
    return true;
}

} // namespace SecretEngine
