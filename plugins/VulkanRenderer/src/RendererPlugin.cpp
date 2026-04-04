#include "RendererPlugin.h"
#include "VulkanDevice.h"
#include <SecretEngine/ILogger.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/IRenderer.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/IInputSystem.h>
#include <SecretEngine/Components.h>
#include <cstring>
#include <random>
#include <SecretEngine/Fast/FastData.h>
#include "../../CameraPlugin/src/CameraPlugin.h"
#include "../../AndroidInput/src/InputPlugin.h"
#include "../../AndroidInput/src/UIConfig.h"
#include "../../DebugPlugin/src/Profiler.h"
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
#include <SecretEngine/IAssetProvider.h>
#include "VulkanHelpers.h"

using SecretEngine::Vulkan::Helpers;

void RendererPlugin::OnLoad(SecretEngine::ICore* core) {
    m_core = core;
    m_core->RegisterCapability("rendering", this);
    m_core->GetLogger()->LogInfo("VulkanRenderer", "Plugin Loaded.");
}

void RendererPlugin::OnActivate() {
    // PIVOT: Do NOT create windows or devices here on Android.
    // We wait for the Core to pass us a native window handle later.
    m_core->GetLogger()->LogInfo("VulkanRenderer", "Renderer Waiting for Native Window...");
}

// NEW FUNCTION: This is what we call from AndroidMain when the window is ready
void RendererPlugin::InitializeHardware(void* nativeWindow) {
    if (!m_core || !m_core->GetLogger()) {
        // Emergency fallback if logger isn't available
        return;
    }
    
    auto logger = m_core->GetLogger();
    logger->LogInfo("VulkanRenderer", "InitializeHardware() called - Starting Vulkan setup");
    
    // Step 1: Create Vulkan Device
    m_device = new VulkanDevice(m_core);
    if (!m_device) {
        logger->LogError("VulkanRenderer", "CRITICAL: Failed to allocate VulkanDevice");
        return;
    }
    logger->LogInfo("VulkanRenderer", "✓ VulkanDevice allocated");
    
    // Step 2: Initialize Vulkan Device
    if (!m_device->Initialize()) {
        logger->LogError("VulkanRenderer", "CRITICAL: VulkanDevice::Initialize() failed");
        delete m_device;
        m_device = nullptr;
        return;
    }
    logger->LogInfo("VulkanRenderer", "✓ VulkanDevice initialized");
    
    // Step 3: Create Surface
    if (!nativeWindow) {
        logger->LogError("VulkanRenderer", "CRITICAL: nativeWindow is nullptr");
        delete m_device;
        m_device = nullptr;
        return;
    }
    
    m_device->CreateSurface(nativeWindow);
    VkSurfaceKHR surf = m_device->GetSurface();
    if (surf == VK_NULL_HANDLE) {
        logger->LogError("VulkanRenderer", "CRITICAL: Failed to create Vulkan Surface");
        delete m_device;
        m_device = nullptr;
        return;
    }
    logger->LogInfo("VulkanRenderer", "✓ Vulkan Surface created successfully");
    
    // Step 4: Get Surface Capabilities
    VkSurfaceCapabilitiesKHR caps;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        m_device->GetPhysicalDevice(), surf, &caps);
    
    if (result != VK_SUCCESS) {
        logger->LogError("VulkanRenderer", "CRITICAL: Failed to get surface capabilities");
        delete m_device;
        m_device = nullptr;
        return;
    }
    logger->LogInfo("VulkanRenderer", "✓ Surface capabilities retrieved");
    
    // Step 5: Create Swapchain
    m_swapchain = new Swapchain(m_device, m_core);
    if (!m_swapchain) {
        logger->LogError("VulkanRenderer", "CRITICAL: Failed to allocate Swapchain");
        delete m_device;
        m_device = nullptr;
        return;
    }
    
    if (!m_swapchain->Create(surf, caps.currentExtent.width, caps.currentExtent.height)) {
        logger->LogError("VulkanRenderer", "CRITICAL: Swapchain::Create() failed");
        delete m_swapchain;
        delete m_device;
        m_swapchain = nullptr;
        m_device = nullptr;
        return;
    }
    logger->LogInfo("VulkanRenderer", "✓ Swapchain created successfully");
    
    // Step 6: Create Render Pass
    if (!CreateRenderPass()) {
        logger->LogError("VulkanRenderer", "CRITICAL: CreateRenderPass() failed");
        delete m_swapchain;
        delete m_device;
        m_swapchain = nullptr;
        m_device = nullptr;
        return;
    }
    logger->LogInfo("VulkanRenderer", "✓ Render Pass created");
    
    // Step 7: Create Framebuffers
    if (!CreateFramebuffers()) {
        logger->LogError("VulkanRenderer", "CRITICAL: CreateFramebuffers() failed");
        delete m_swapchain;
        delete m_device;
        m_swapchain = nullptr;
        m_device = nullptr;
        return;
    }
    logger->LogInfo("VulkanRenderer", "✓ Framebuffers created");
    
    // Step 8: Create Sync Objects
    if (!CreateSyncObjects()) {
        logger->LogError("VulkanRenderer", "CRITICAL: CreateSyncObjects() failed");
        delete m_swapchain;
        delete m_device;
        m_swapchain = nullptr;
        m_device = nullptr;
        return;
    }
    logger->LogInfo("VulkanRenderer", "✓ Sync objects created");
    
    // Step 9: Initialize Command Buffers
    if (!InitCommands()) {
        logger->LogError("VulkanRenderer", "CRITICAL: InitCommands() failed");
        delete m_swapchain;
        delete m_device;
        m_swapchain = nullptr;
        m_device = nullptr;
        return;
    }
    logger->LogInfo("VulkanRenderer", "✓ Command buffers initialized");
    
    // Step 10: Create 2D Text Rendering Pipeline
    if (!Create2DPipeline()) {
        logger->LogWarning("VulkanRenderer", "2D pipeline creation failed (non-critical)");
    } else if (!Create2DVertexBuffer()) {
        logger->LogWarning("VulkanRenderer", "2D vertex buffer creation failed (non-critical)");
    } else {
        logger->LogInfo("VulkanRenderer", "✓ 2D text rendering pipeline enabled");
    }

    // Step 11: Mega Geometry System (Primary 3D Renderer)
    logger->LogInfo("VulkanRenderer", "Initializing Mega Geometry System...");
    
    // Step 12: Initialize Texture System (bindless textures for Mega Geometry)
    m_textureManager = new SecretEngine::Textures::TextureManager();
    if (!m_textureManager->Initialize(m_device, m_core)) {
        logger->LogError("VulkanRenderer", "Failed to initialize TextureManager");
        delete m_textureManager;
        m_textureManager = nullptr;
    } else {
        logger->LogInfo("VulkanRenderer", "✓ TextureManager initialized");
    }

    // Step 13: Initialize Mega Geometry System
    m_megaGeometry = new SecretEngine::MegaGeometry::MegaGeometryRenderer();
    if (!m_megaGeometry->Initialize(m_device, m_renderPass, m_core, m_textureManager)) {
        logger->LogError("VulkanRenderer", "Failed to create mega geometry renderer");
        delete m_megaGeometry;
        m_megaGeometry = nullptr;
    } else {
        logger->LogInfo("VulkanRenderer", "✓ Mega Geometry System initialized - meshes will be loaded on-demand");
        
        // Meshes are now loaded dynamically via GetOrLoadMeshSlot when entities are created
        // No need to pre-load any specific mesh
        
        // Instances are already handled by logic/scattering
        // NOTE: Test code for 4000 instances disabled - entities now loaded from level system
        /*
            // Spawn 4000 Instances in a Spherical Distribution around the player
            // Target: 6M triangles (4000 instances × ~1500 tris/instance)
            std::mt19937 rng(1337); 
            std::uniform_real_distribution<float> distRadius(300.0f, 2000.0f);
            std::uniform_real_distribution<float> distPhi(0.0f, 3.14159f);
            std::uniform_real_distribution<float> distTheta(0.0f, 6.28318f);
            std::uniform_real_distribution<float> distColor(0.2f, 1.0f);

            // Preload default character texture once (cached in TextureManager)
            uint32_t defaultCharacterTextureID = UINT32_MAX;
            if (m_textureManager) {
                defaultCharacterTextureID = m_textureManager->LoadTexture("textures/diffuse.jpeg");
                // Load normal map as second texture
                m_textureManager->LoadTexture("textures/NormalMap.png");
            }
            
            m_instancePositions.clear();
            m_instanceRotationSpeeds.clear();
            m_instanceColors.clear();
            
            uint32_t instancesAdded = 0;
            for(int i = 0; i < 4000; i++) {
                 float r = distRadius(rng);
                 float phi = distPhi(rng);
                 float theta = distTheta(rng);
                 
                 float posX = r * sinf(phi) * cosf(theta);
                 float posY = r * sinf(phi) * sinf(theta);
                 float posZ = r * cosf(phi);
                 
                 uint32_t instanceID = m_megaGeometry->AddInstance(0, posX, posY, posZ, defaultCharacterTextureID);
                 if (instanceID < 65536) { // Valid instance ID (not MAX_INSTANCES error)
                     instancesAdded++;
                     m_instancePositions.push_back({posX, posY, posZ});
                     
                     // Random speeds: 0.1 to 2.0 rad/sec
                     std::uniform_real_distribution<float> distSpeed(0.1f, 2.0f);
                     m_instanceRotationSpeeds.push_back(distSpeed(rng));
                     
                     // Set color to WHITE (1,1,1) so texture shows properly without tinting
                     m_instanceColors.push_back({1.0f, 1.0f, 1.0f});
                     m_megaGeometry->UpdateInstanceColor(instanceID, 1.0f, 1.0f, 1.0f, 1.0f);
                 }
            }
            char msg[256];
            snprintf(msg, sizeof(msg), "✓ Spawned %u Instances in Sphere (R=2000) - Target: 6M triangles", instancesAdded);
            logger->LogInfo("VulkanRenderer", msg);
            
            // Verify instance count
            auto stats = m_megaGeometry->GetStats();
            snprintf(msg, sizeof(msg), "✓ Instance count verification: totalInstances=%u, totalTriangles=%u", 
                stats.totalInstances, stats.totalTriangles);
            logger->LogInfo("VulkanRenderer", msg);
        */
        logger->LogInfo("VulkanRenderer", "✓ Mega Geometry Renderer initialized");
    }
    
    // Step 13.5: Initialize Mesh Rendering System
    if (m_megaGeometry && m_textureManager) {
        m_meshRenderingSystem = new SecretEngine::MeshRenderingSystem(m_core, m_megaGeometry, m_textureManager);
        logger->LogInfo("VulkanRenderer", "✓ Mesh Rendering System initialized");
    }
    
    // Step 14: Query Plugin Systems (Modular Architecture)
    logger->LogInfo("VulkanRenderer", "Querying plugin systems...");
    m_lightingSystem = reinterpret_cast<SecretEngine::ILightingSystem*>(m_core->GetCapability("lighting"));
    m_materialSystem = reinterpret_cast<SecretEngine::IMaterialSystem*>(m_core->GetCapability("materials"));
    m_textureSystemPlugin = reinterpret_cast<SecretEngine::ITextureSystem*>(m_core->GetCapability("textures"));
    m_shadowSystem = reinterpret_cast<SecretEngine::IShadowSystem*>(m_core->GetCapability("shadows"));
    
    if (m_lightingSystem) logger->LogInfo("VulkanRenderer", "✓ LightingSystem plugin found");
    if (m_materialSystem) logger->LogInfo("VulkanRenderer", "✓ MaterialSystem plugin found");
    if (m_textureSystemPlugin) logger->LogInfo("VulkanRenderer", "✓ TextureSystem plugin found");
    if (m_shadowSystem) logger->LogInfo("VulkanRenderer", "✓ ShadowSystem plugin found");
    
    // Step 15: Create GPU Buffers for Plugin Data
    if (!CreatePluginBuffers()) {
        logger->LogWarning("VulkanRenderer", "Failed to create plugin buffers (non-critical)");
    } else {
        logger->LogInfo("VulkanRenderer", "✓ Plugin GPU buffers created");
    }
    
    // All 3D rendering now goes through MegaGeometryRenderer
    logger->LogInfo("VulkanRenderer", "✓ Mega Geometry System is the primary 3D renderer");
    
    // --- Populate World with Entities (Day 3.9) ---
    if (m_core->GetWorld()) {
            auto world = m_core->GetWorld();
            auto logger = m_core->GetLogger();

            std::string sceneJson = LoadAssetAsString("scene.json");
            if (!sceneJson.empty()) {
                try {
                    auto data = json::parse(sceneJson);
                    if (data.contains("entities") && data["entities"].is_array()) {
                        for (const auto& entityData : data["entities"]) {
                            auto e = world->CreateEntity();
                            
                            // Transform
                            auto transform = new SecretEngine::TransformComponent();
                            // Support both "transform" and "transformed" as the user mentioned
                            std::string transformKey = entityData.contains("transformed") ? "transformed" : "transform";
                            
                            if (entityData.contains(transformKey)) {
                                auto t = entityData[transformKey];
                                if (t.contains("position") && t["position"].is_array()) {
                                    transform->position[0] = t["position"].size() > 0 ? t["position"][0].get<float>() : 0.0f;
                                    transform->position[1] = t["position"].size() > 1 ? t["position"][1].get<float>() : 0.0f;
                                    transform->position[2] = t["position"].size() > 2 ? t["position"][2].get<float>() : 0.0f;
                                }
                                if (t.contains("rotation") && t["rotation"].is_array()) {
                                    const float DEG_TO_RAD = 3.14159265f / 180.0f;
                                    transform->rotation[0] = t["rotation"].size() > 0 ? t["rotation"][0].get<float>() * DEG_TO_RAD : 0.0f;
                                    transform->rotation[1] = t["rotation"].size() > 1 ? t["rotation"][1].get<float>() * DEG_TO_RAD : 0.0f;
                                    transform->rotation[2] = t["rotation"].size() > 2 ? t["rotation"][2].get<float>() * DEG_TO_RAD : 0.0f;
                                }
                                if (t.contains("scale") && t["scale"].is_array()) {
                                    transform->scale[0] = t["scale"].size() > 0 ? t["scale"][0].get<float>() : 1.0f;
                                    transform->scale[1] = t["scale"].size() > 1 ? t["scale"][1].get<float>() : 1.0f;
                                    transform->scale[2] = t["scale"].size() > 2 ? t["scale"][2].get<float>() : 1.0f;
                                }
                            }
                            world->AddComponent(e, SecretEngine::TransformComponent::TypeID, transform);
                            
                            // Mesh
                            if (entityData.contains("mesh")) {
                                auto m = entityData["mesh"];
                                auto mesh = new SecretEngine::MeshComponent();
                                if (m.contains("path")) {
                                    std::string meshPath = m["path"].get<std::string>();
                                    strncpy(mesh->meshPath, meshPath.c_str(), 255);
                                    mesh->meshPath[255] = '\0';

                                    // Parse color first (before instancing)
                                    if (m.contains("color") && m["color"].is_array()) {
                                        mesh->color[0] = m["color"].size() > 0 ? m["color"][0].get<float>() : 1.0f;
                                        mesh->color[1] = m["color"].size() > 1 ? m["color"][1].get<float>() : 1.0f;
                                        mesh->color[2] = m["color"].size() > 2 ? m["color"][2].get<float>() : 1.0f;
                                        mesh->color[3] = m["color"].size() > 3 ? m["color"][3].get<float>() : 1.0f;
                                    }

                                    // Optional per-entity texture paths from scene.json
                                    if (m.contains("texture")) {
                                        std::string tex = m["texture"].get<std::string>();
                                        strncpy(mesh->texturePath, tex.c_str(), 255);
                                        mesh->texturePath[255] = '\0';
                                    }
                                    if (m.contains("normalMap")) {
                                        std::string nrm = m["normalMap"].get<std::string>();
                                        strncpy(mesh->normalMapPath, nrm.c_str(), 255);
                                        mesh->normalMapPath[255] = '\0';
                                    }

                                    // Determine if this is a character mesh for default texture assignment
                                    const bool isCharacterMesh = meshPath.find("Character.meshbin") != std::string::npos;

                                    // Provide sensible defaults for character meshes if scene doesn't specify textures
                                    if (isCharacterMesh) {
                                        if (mesh->texturePath[0] == '\0') {
                                            const char* defaultTex = "textures/diffuse.jpeg";
                                            strncpy(mesh->texturePath, defaultTex, 255);
                                            mesh->texturePath[255] = '\0';
                                        }
                                        if (mesh->normalMapPath[0] == '\0') {
                                            const char* defaultNrm = "textures/NormalMap.png";
                                            strncpy(mesh->normalMapPath, defaultNrm, 255);
                                            mesh->normalMapPath[255] = '\0';
                                        }
                                    }
                                    
                                    // Instancing: Add this entity to MegaGeometry system WITH COLOR + TEXTURE
                                    if (m_megaGeometry) {
                                        // Get or load the mesh slot dynamically based on mesh path
                                        uint32_t meshSlot = m_megaGeometry->GetOrLoadMeshSlot(meshPath.c_str());
                                        
                                        uint32_t textureID = UINT32_MAX;
                                        if (m_textureManager && mesh->texturePath[0] != '\0') {
                                            textureID = m_textureManager->LoadTexture(mesh->texturePath);
                                        }

                                        uint32_t instanceID = m_megaGeometry->AddInstance(
                                            meshSlot,
                                            transform->position[0],
                                            transform->position[1],
                                            transform->position[2],
                                            textureID
                                        );
                                        // Apply color to the instance
                                        m_megaGeometry->UpdateInstanceColor(instanceID, mesh->color[0], mesh->color[1], mesh->color[2], mesh->color[3]);
                                    }
                                    
                                    // Mesh loading handled by MegaGeometryRenderer
                                }
                                world->AddComponent(e, SecretEngine::MeshComponent::TypeID, mesh);
                            }

                            if (entityData.contains("isPlayerStart") && entityData["isPlayerStart"].get<bool>()) {
                                m_playerStartEntity = e;
                            }
                        }
                        logger->LogInfo("VulkanRenderer", ("✓ Scene loaded successfully: " + std::to_string(data["entities"].size()) + " entities").c_str());
                    }
                } catch (const std::exception& e) {
                    logger->LogError("VulkanRenderer", (std::string("Failed to parse scene.json: ") + e.what()).c_str());
                }
            } else {
                logger->LogError("VulkanRenderer", "scene.json could not be loaded (empty or missing)!");
            }
    }
    
    // SUCCESS!
    m_core->SetRendererReady(true);
    
    // Initialize GPU timing
    if (!CreateTimestampQueries()) {
        logger->LogWarning("VulkanRenderer", "GPU timestamp queries not available - GPU timing will be 0");
    } else {
        logger->LogInfo("VulkanRenderer", "✓ GPU timestamp queries enabled");
    }
    
    logger->LogInfo("VulkanRenderer", "========================================");
    logger->LogInfo("VulkanRenderer", "Vulkan Renderer FULLY INITIALIZED");
    logger->LogInfo("VulkanRenderer", "Ready to render 3D + 2D!");
    logger->LogInfo("VulkanRenderer", "========================================");
}

void RendererPlugin::OnUpdate(float dt) {
    m_totalTime += dt;
    
    // Update mesh rendering system to process new entities
    if (m_meshRenderingSystem) {
        m_meshRenderingSystem->Update(dt);
    }
    
    if (m_megaGeometry) {
        // ULTRA-FAST BATCH UPDATE - Minimize cache misses
        m_megaGeometry->BeginBatchUpdate();
        
        uint32_t count = (uint32_t)m_instancePositions.size();
        
        // Pre-compute time factors once
        float timeX = m_totalTime;
        float timeY = m_totalTime * 0.7f;
        float timeZ = m_totalTime * 0.3f;
        
        for (uint32_t i = 0; i < count; ++i) {
            float speed = m_instanceRotationSpeeds[i];
            
            // Y-rotation only for 10x faster matrix construction
            float rotY = timeY * speed;
            
            m_megaGeometry->UpdateInstanceTransform(
                i, 
                m_instancePositions[i].x, 
                m_instancePositions[i].y, 
                m_instancePositions[i].z, 
                rotY, 0.0f, 0.0f  // Only Y rotation - triggers fast path
            );
        }
        
        m_megaGeometry->EndBatchUpdate();
    }
}
void RendererPlugin::OnDeactivate() {
    // Optional: Log deactivation or handle backgrounding
    if(m_core && m_core->GetLogger()) m_core->GetLogger()->LogInfo("VulkanRenderer", "Deactivating renderer...");
}

void RendererPlugin::OnUnload() {
    if(m_core && m_core->GetLogger()) m_core->GetLogger()->LogInfo("VulkanRenderer", "Unloading plugin...");
    
    // Cleanup GPU timing
    if (m_queryPool && m_device) {
        vkDestroyQueryPool(m_device->GetDevice(), m_queryPool, nullptr);
        m_queryPool = VK_NULL_HANDLE;
    }
    
    if (m_device) {
        vkDeviceWaitIdle(m_device->GetDevice());
        
        CleanupSwapchain();
        
        // Cleanup MegaGeometry (primary renderer)
        if (m_megaGeometry) {
            delete m_megaGeometry;
            m_megaGeometry = nullptr;
        }
        
        if (m_meshRenderingSystem) {
            delete m_meshRenderingSystem;
            m_meshRenderingSystem = nullptr;
        }

        if (m_textureManager) {
            m_textureManager->Shutdown();
            delete m_textureManager;
            m_textureManager = nullptr;
        }
        
        // Cleanup plugin buffers
        if (m_lightBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device->GetDevice(), m_lightBuffer, nullptr);
            m_lightBuffer = VK_NULL_HANDLE;
        }
        if (m_lightMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device->GetDevice(), m_lightMemory, nullptr);
            m_lightMemory = VK_NULL_HANDLE;
        }
        if (m_materialBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device->GetDevice(), m_materialBuffer, nullptr);
            m_materialBuffer = VK_NULL_HANDLE;
        }
        if (m_materialMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device->GetDevice(), m_materialMemory, nullptr);
            m_materialMemory = VK_NULL_HANDLE;
        }
        if (m_lightDescriptorLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_device->GetDevice(), m_lightDescriptorLayout, nullptr);
            m_lightDescriptorLayout = VK_NULL_HANDLE;
        }
        if (m_materialDescriptorLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_device->GetDevice(), m_materialDescriptorLayout, nullptr);
            m_materialDescriptorLayout = VK_NULL_HANDLE;
        }

        if (m_inFlightFence) vkDestroyFence(m_device->GetDevice(), m_inFlightFence, nullptr);
        if (m_imageAvailableSemaphore) vkDestroySemaphore(m_device->GetDevice(), m_imageAvailableSemaphore, nullptr);
        if (m_renderFinishedSemaphore) vkDestroySemaphore(m_device->GetDevice(), m_renderFinishedSemaphore, nullptr);
        if (m_commandPool) vkDestroyCommandPool(m_device->GetDevice(), m_commandPool, nullptr);
        
        if (m_renderPass != VK_NULL_HANDLE) vkDestroyRenderPass(m_device->GetDevice(), m_renderPass, nullptr);
        if (m_2dPipeline != VK_NULL_HANDLE) vkDestroyPipeline(m_device->GetDevice(), m_2dPipeline, nullptr);
        if (m_2dPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(m_device->GetDevice(), m_2dPipelineLayout, nullptr);

        if (m_2dVertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device->GetDevice(), m_2dVertexBuffer, nullptr);
            vkFreeMemory(m_device->GetDevice(), m_2dVertexMemory, nullptr);
        }

        m_device->Shutdown();
        delete m_device;
        m_device = nullptr;
    }
}

void RendererPlugin::GetCameraMatrix(float* outViewProj) {
    // Get camera plugin
    auto* camera = m_core->GetCapability("camera");
    if (!camera) {
        // Fallback: identity matrix
        memset(outViewProj, 0, 16*sizeof(float));
        outViewProj[0] = outViewProj[5] = outViewProj[10] = outViewProj[15] = 1.0f;
        return;
    }
    
    // Get view-projection from camera (needs interface method)
    // For now, use direct access via void* interface
    auto* cameraPlugin = static_cast<SecretEngine::CameraPlugin*>(camera);
    auto vp = cameraPlugin->GetViewProjection();
    memcpy(outViewProj, vp.data(), 16*sizeof(float));
}

void RendererPlugin::Submit() {
    // Primary consumer of the Command Stream
    ProcessFastStream();
}

void RendererPlugin::ProcessFastStream() {
    SecretEngine::Fast::UltraPacket packet;
    // Drain batch for performance
    while (m_commandStream.Pop(packet)) {
        // Route to Mega Geometry first
        if (m_megaGeometry) {
             m_megaGeometry->ProcessPacket(packet);
        }

        switch (packet.GetType()) {
            case SecretEngine::Fast::PacketType::RenderTransform:
                m_rotation = static_cast<float>(packet.dataB) / 32767.0f;
                break;
            case SecretEngine::Fast::PacketType::RenderCommand:
                if (packet.GetMetadata() == 1) { // Set Color
                    SetCubeColor(packet.dataA);
                }
                break;
            default:
                break;
        }
    }
}

void RendererPlugin::Present() {
    // Defensive checks - if any component is missing, skip rendering
    if (!m_device || !m_swapchain || !m_swapchain->GetSwapchain()) {
        return;
    }
    
    if (!m_commandBuffer || m_framebuffers.empty()) {
        // Command buffers or framebuffers not ready
        return;
    }
    
    // Log first present call
    static bool firstPresent = true;
    if (firstPresent && m_core && m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("VulkanRenderer", "Present() called - starting to render frames");
        firstPresent = false;
    }

    // OPTIMIZATION: Update plugin buffers (only if dirty)
    UpdateLightBuffer();
    UpdateMaterialBuffer();

    // Acquire image first, then wait on fence (reduces latency)
    uint32_t imageIndex;
    // Use 16ms timeout (60 FPS) instead of infinite wait to avoid long blocks
    VkResult result = vkAcquireNextImageKHR(m_device->GetDevice(), m_swapchain->GetSwapchain(), 16666666, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    
    vkWaitForFences(m_device->GetDevice(), 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        if (m_core && m_core->GetLogger()) {
            m_core->GetLogger()->LogError("VulkanRenderer", "Failed to acquire swapchain image!");
        }
        return;
    }

    // Safety check for framebuffer index
    if (imageIndex >= m_framebuffers.size()) {
        if (m_core && m_core->GetLogger()) {
            m_core->GetLogger()->LogError("VulkanRenderer", "Image index out of range of framebuffers!");
        }
        return;
    }

    // Only reset fence if we are actually going to submit work
    vkResetFences(m_device->GetDevice(), 1, &m_inFlightFence);

    // Update global time-based rotation (used by shaders/logic)
    m_rotation += 0.016f;
    
    // Begin recording with ONE_TIME_SUBMIT flag for better performance
    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
    
    // Record GPU start timestamp
    RecordGPUTimestamps(m_commandBuffer);
    
    // OPTIMIZATION: Shadow Pass (before main render pass)
    RenderShadowPass(m_commandBuffer);
    
    // OPTIMIZATION: Volumetric Lighting Compute (after shadow pass)
    DispatchVolumetricLighting(m_commandBuffer);
    
    VkRenderPassBeginInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchain->GetExtent();
    
    
    // Neutral dark gray background for better visibility
    VkClearValue clearColor = {{{0.15f, 0.15f, 0.15f, 1.0f}}};
    
    // Log once to confirm
    static bool loggedColor = false;
    if (!loggedColor && m_core && m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("VulkanRenderer", "Rendering 3D scene on DARK GREY background");
        loggedColor = true;
    }
    
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    // GPU-Driven Virtual Geometry Prep (Culling) - Done OUTSIDE RenderPass
    if (m_megaGeometry) {
        m_megaGeometry->PreRender(m_commandBuffer);
    }

    vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Set Dynamic Viewport and Scissor
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swapchain->GetExtent().width;
    viewport.height = (float)m_swapchain->GetExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchain->GetExtent();
    vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
    
    
    
    // Get camera matrix from CameraPlugin and send to MegaGeometry
    float viewProj[16];
    GetCameraMatrix(viewProj);
    if (m_megaGeometry) {
        m_megaGeometry->SetViewProjection(viewProj);
    }
    
    //    // 3D Rendering
    float aspect = 1.0f;
    if (m_swapchain) {
        VkExtent2D ext = m_swapchain->GetExtent();
        if (ext.height > 0) aspect = (float)ext.width / (float)ext.height;
    }

    // Use unified view-projection from CameraPlugin
    float vp[16];
    GetCameraMatrix(vp);
    
    // Draw Mega Geometry (GPU Driven - Primary 3D Renderer)
    if (m_megaGeometry) {
        m_megaGeometry->Render(m_commandBuffer);
    }

        // Draw 2D UI overlay on top
    DrawWelcomeText(m_commandBuffer);
    
    vkCmdEndRenderPass(m_commandBuffer);
    
    // Write GPU end timestamp
    if (m_timestampsSupported && m_queryPool) {
        vkCmdWriteTimestamp(m_commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPool, 1);
    }
    
    vkEndCommandBuffer(m_commandBuffer);

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;
    
    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &submitInfo, m_inFlightFence) != VK_SUCCESS) {
        if (m_core && m_core->GetLogger()) {
            m_core->GetLogger()->LogError("VulkanRenderer", "Failed to submit draw command buffer!");
        }
    }
    
    // Read GPU timestamps from previous frame (after fence wait)
    ReadGPUTimestamps();

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapchains[] = { m_swapchain->GetSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    // Check present result too
    result = vkQueuePresentKHR(m_device->GetGraphicsQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapchain();
    } else if (result != VK_SUCCESS) {
        if (m_core && m_core->GetLogger()) {
            m_core->GetLogger()->LogError("VulkanRenderer", "Failed to present swapchain image!");
        }
    }
    
    // Log successful frame every 60 frames
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 60 == 0 && m_core && m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("VulkanRenderer", "Rendering frames successfully");
    }
}

void RendererPlugin::SetCubeColor(int colorIndex) {
    // Color setting now handled through MegaGeometryRenderer::UpdateInstanceColor
}

void RendererPlugin::SetDebugInfo(int slot, const char* text) {
    if (text) {
        m_debugStrings[slot] = text;
    } else {
        m_debugStrings.erase(slot);
    }
}

void RendererPlugin::GetStats(uint32_t& instances, uint32_t& triangles, uint32_t& drawCalls) {
    instances = 0; triangles = 0; drawCalls = 1; // Base calls
    
    if (m_megaGeometry) {
        auto s = m_megaGeometry->GetStats();
        instances += s.totalInstances;
        triangles += s.totalTriangles;
        drawCalls += s.drawCalls;
    }
}

uint64_t RendererPlugin::GetVRAMUsage() {
    return SecretEngine::Vulkan::Helpers::GetVRAMAllocated();
}

void RendererPlugin::ClearAllInstances() {
    if (m_megaGeometry) {
        m_megaGeometry->ClearAllInstances();
    }
    
    if (m_core && m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("VulkanRenderer", "All instances cleared for level change");
    }
}

bool RendererPlugin::CreateRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_swapchain->GetFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    
    // Add subpass dependency for proper synchronization
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device->GetDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        m_core->GetLogger()->LogError("VulkanRenderer", "Failed to create RenderPass!");
        return false;
    }
    return true;
}

bool RendererPlugin::CreateFramebuffers() {
    auto& imageViews = m_swapchain->GetImageViews();
    m_framebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        VkImageView attachments[] = { imageViews[i] };
        VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapchain->GetExtent().width;
        framebufferInfo.height = m_swapchain->GetExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device->GetDevice(), &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
            m_core->GetLogger()->LogError("VulkanRenderer", "Failed to create Framebuffer!");
            return false;
        }
    }
    return true;
}

bool RendererPlugin::CreateSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(m_device->GetDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(m_device->GetDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(m_device->GetDevice(), &fenceInfo, nullptr, &m_inFlightFence) != VK_SUCCESS) {
        
        m_core->GetLogger()->LogError("VulkanRenderer", "Failed to create Sync Objects!");
        return false;
    }
    return true;
}

bool RendererPlugin::InitCommands() {
    VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.queueFamilyIndex = 0; // Assume 0 for now as we did in VulkanDevice
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_device->GetDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        m_core->GetLogger()->LogError("VulkanRenderer", "Failed to create Command Pool!");
        return false;
    }

    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(m_device->GetDevice(), &allocInfo, &m_commandBuffer) != VK_SUCCESS) {
        m_core->GetLogger()->LogError("VulkanRenderer", "Failed to allocate Command Buffer!");
        return false;
    }
    return true;
}

// ============================================================================
// 2D TEXT RENDERING IMPLEMENTATION
// ============================================================================

struct Vertex2D {
    float x, y;       // Position
    float r, g, b;    // Color
};

bool RendererPlugin::Create2DVertexBuffer() {
    // Allocate a large enough buffer for Sky (6) + Dynamic UI (2048)
    // Total = 2054 vertices
    
    // 1. SKY GRADIENT REMOVED (User request)
    std::vector<Vertex2D> vertices;
    m_skyVertexCount = 0;
    
    // Reserve space for dynamic text
    size_t maxDynamic = 2048;
    vertices.resize(maxDynamic);
    // Fill with 0 to be safe
    memset(vertices.data(), 0, maxDynamic * sizeof(Vertex2D));
    
    m_2dVertexCount = static_cast<uint32_t>(vertices.size());
    VkDeviceSize bufferSize = sizeof(Vertex2D) * vertices.size();
    
    // Create and copy INITIAL data (Sky + Zeros)
    if (!Helpers::CreateBuffer(m_device->GetDevice(), m_device->GetPhysicalDevice(), bufferSize,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m_2dVertexBuffer, m_2dVertexMemory)) return false;

    std::span<const std::byte> vertexSpan(reinterpret_cast<const std::byte*>(vertices.data()), bufferSize);
    Helpers::MapAndCopy(m_device->GetDevice(), m_2dVertexMemory, vertexSpan);
    
    return true;
}

bool RendererPlugin::Create2DPipeline() {
    auto logger = m_core->GetLogger();
    logger->LogInfo("VulkanRenderer", "=== Create2DPipeline START ===");
    
    if (!m_device || !m_renderPass || !m_swapchain) return false;
    
    // Load shaders via Core Service
    auto vertCode = m_core->GetAssetProvider()->LoadBinary("shaders/ui_vert.spv");
    auto fragCode = m_core->GetAssetProvider()->LoadBinary("shaders/ui_frag.spv");
    
    if (vertCode.empty() || fragCode.empty()) {
        logger->LogError("VulkanRenderer", "Failed to load 2D shaders");
        return false;
    }
    
    VkShaderModule vertMod = Helpers::CreateShaderModule(m_device->GetDevice(), vertCode);
    VkShaderModule fragMod = Helpers::CreateShaderModule(m_device->GetDevice(), fragCode);
    
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertMod;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragMod;
    stages[1].pName = "main";
    
    VkVertexInputBindingDescription bind = {0, sizeof(Vertex2D), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attrs[2] = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, x)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex2D, r)}
    };
    
    VkPipelineVertexInputStateCreateInfo vi = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vi.vertexBindingDescriptionCount = 1; vi.pVertexBindingDescriptions = &bind;
    vi.vertexAttributeDescriptionCount = 2; vi.pVertexAttributeDescriptions = attrs;
    
    VkPipelineInputAssemblyStateCreateInfo ia = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkPipelineViewportStateCreateInfo vp = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    vp.viewportCount = 1; vp.scissorCount = 1;
    
    VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo ds = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    ds.dynamicStateCount = 2; ds.pDynamicStates = dynStates;
    
    VkPipelineRasterizationStateCreateInfo rs = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rs.lineWidth = 1.0f; rs.cullMode = VK_CULL_MODE_NONE; rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
    
    VkPipelineMultisampleStateCreateInfo ms = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineColorBlendAttachmentState cba = {};
    cba.colorWriteMask = 0xF; cba.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo cb = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    cb.attachmentCount = 1; cb.pAttachments = &cba;
    
    VkPushConstantRange push = {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4};
    VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layoutInfo.pushConstantRangeCount = 1; layoutInfo.pPushConstantRanges = &push;
    vkCreatePipelineLayout(m_device->GetDevice(), &layoutInfo, nullptr, &m_2dPipelineLayout);
    
    VkGraphicsPipelineCreateInfo pipe = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipe.stageCount = 2; pipe.pStages = stages; pipe.pVertexInputState = &vi; pipe.pInputAssemblyState = &ia;
    pipe.pViewportState = &vp; pipe.pRasterizationState = &rs; pipe.pMultisampleState = &ms; pipe.pColorBlendState = &cb;
    pipe.pDynamicState = &ds; pipe.layout = m_2dPipelineLayout; pipe.renderPass = m_renderPass;
    
    if (vkCreateGraphicsPipelines(m_device->GetDevice(), VK_NULL_HANDLE, 1, &pipe, nullptr, &m_2dPipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(m_device->GetDevice(), fragMod, nullptr);
        vkDestroyShaderModule(m_device->GetDevice(), vertMod, nullptr);
        return false;
    }
    
    vkDestroyShaderModule(m_device->GetDevice(), fragMod, nullptr);
    vkDestroyShaderModule(m_device->GetDevice(), vertMod, nullptr);
    
    logger->LogInfo("VulkanRenderer", "=== Create2DPipeline SUCCESS ===");
    return true;
}

// Helper for text rendering

static void addQuad(std::vector<Vertex2D>& verts, float x, float y, float w, float h, float r, float g, float b) {
    verts.push_back({x, y, r, g, b});
    verts.push_back({x + w, y, r, g, b});
    verts.push_back({x, y + h, r, g, b});
    verts.push_back({x + w, y, r, g, b});
    verts.push_back({x + w, y + h, r, g, b});
    verts.push_back({x, y + h, r, g, b});
}

static void DrawChar(std::vector<Vertex2D>& verts, char c, float x, float y, float w, float h, float r, float g, float b) {
    float t = 0.005f;
    // Simple segment-based font for digits and some letters
    if (c >= '0' && c <= '9') {
        int d = c - '0';
        bool s[10][7] = {
            {1,1,1,0,1,1,1}, {0,0,1,0,0,1,0}, {1,0,1,1,1,0,1}, {1,0,1,1,0,1,1}, {0,1,1,1,0,1,0},
            {1,1,0,1,0,1,1}, {1,1,0,1,1,1,1}, {1,0,1,0,0,1,0}, {1,1,1,1,1,1,1}, {1,1,1,1,0,1,1}
        };
        if(s[d][0]) addQuad(verts, x, y, w, t, r, g, b); // Top
        if(s[d][1]) addQuad(verts, x, y, t, h/2, r, g, b); // Top-Left
        if(s[d][2]) addQuad(verts, x + w - t, y, t, h/2, r, g, b); // Top-Right
        if(s[d][3]) addQuad(verts, x, y + h/2, w, t, r, g, b); // Mid
        if(s[d][4]) addQuad(verts, x, y + h/2, t, h/2, r, g, b); // Bot-Left
        if(s[d][5]) addQuad(verts, x + w - t, y + h/2, t, h/2, r, g, b); // Bot-Right
        if(s[d][6]) addQuad(verts, x, y + h - t, w, t, r, g, b); // Bottom
    } else {
        // Fallback for letters - just some boxes to make them recognizable as "text"
        c = toupper(c);
        if (c == 'F') {
            addQuad(verts, x, y, w, t, r, g, b);
            addQuad(verts, x, y, t, h, r, g, b);
            addQuad(verts, x, y + h/2, w*0.7f, t, r, g, b);
        } else if (c == 'P') {
            addQuad(verts, x, y, w, t, r, g, b);
            addQuad(verts, x, y, t, h, r, g, b);
            addQuad(verts, x + w - t, y, t, h/2, r, g, b);
            addQuad(verts, x, y + h/2, w, t, r, g, b);
        } else if (c == 'S') {
            addQuad(verts, x, y, w, t, r, g, b);
            addQuad(verts, x, y, t, h/2, r, g, b);
            addQuad(verts, x, y + h/2, w, t, r, g, b);
            addQuad(verts, x + w - t, y + h/2, t, h/2, r, g, b);
            addQuad(verts, x, y + h - t, w, t, r, g, b);
        } else if (c == 'I') {
            addQuad(verts, x + w/2 - t/2, y, t, h, r, g, b);
            addQuad(verts, x, y, w, t, r, g, b);
            addQuad(verts, x, y + h - t, w, t, r, g, b);
        } else if (c == 'N') {
            addQuad(verts, x, y, t, h, r, g, b);
            addQuad(verts, x + w - t, y, t, h, r, g, b);
            addQuad(verts, x, y, w, t, r, g, b);
        } else if (c == 'T') {
            addQuad(verts, x, y, w, t, r, g, b);
            addQuad(verts, x + w/2 - t/2, y, t, h, r, g, b);
        } else if (c == 'R') {
            addQuad(verts, x, y, t, h, r, g, b);
            addQuad(verts, x, y, w, t, r, g, b);
            addQuad(verts, x + w - t, y, t, h/2, r, g, b);
            addQuad(verts, x, y + h/2, w, t, r, g, b);
        } else if (c == ':') {
            addQuad(verts, x + w/2 - t, y + h*0.3f, t*2, t*2, r, g, b);
            addQuad(verts, x + w/2 - t, y + h*0.7f, t*2, t*2, r, g, b);
        } else if (c == '.') {
            addQuad(verts, x + w/2 - t, y + h - t*2, t*2, t*2, r, g, b);
        } else {
            // Box for everything else
            addQuad(verts, x, y + h - t*4, w*0.8f, t*4, 0.4f, 0.4f, 0.4f);
        }
    }
}

static void DrawJoystick(std::vector<Vertex2D>& verts, const SecretEngine::JoystickConfig& config, float joyX, float joyY) {
    if (!config.visible || !config.enabled) return;
    
    float sz = config.baseSize;
    // Draw base circle
    addQuad(verts, config.xNDC, config.yNDC, sz, sz, 
            config.baseColor[0], config.baseColor[1], config.baseColor[2]);
    
    // Draw inner stick (offset by joystick position)
    float stickSz = config.stickSize;
    float maxOffset = (sz - stickSz) * config.maxOffsetMultiplier;
    float stickX = config.xNDC + maxOffset + (joyX * maxOffset);
    float stickY = config.yNDC + maxOffset + (joyY * maxOffset);
    addQuad(verts, stickX, stickY, stickSz, stickSz, 
            config.stickColor[0], config.stickColor[1], config.stickColor[2]);
}

void RendererPlugin::DrawWelcomeText(VkCommandBuffer cmd) {
    if (m_2dPipeline == VK_NULL_HANDLE || m_2dVertexBuffer == VK_NULL_HANDLE) return;

    // Get UI configuration from input system
    const SecretEngine::UIConfig* uiConfig = nullptr;
    auto* inputSystem = m_core->GetInput();
    if (inputSystem) {
        auto* androidInput = static_cast<SecretEngine::AndroidInput*>(inputSystem);
        uiConfig = &androidInput->GetUIConfig();
    }
    
    // Build Dynamic Vertices
    std::vector<Vertex2D> frameVertices;

    // === UI BUTTONS (Configurable) ===
    if (uiConfig && uiConfig->GetScreenZones().uiButtonsEnabled) {
        const auto& buttons = uiConfig->GetButtons();
        const auto& layout = uiConfig->GetLayout();
        const auto& zones = uiConfig->GetScreenZones();
        
        // Calculate button area
        float heightPercent = zones.uiButtonsHeightPercent / 100.0f;
        float buttonTop = -1.0f;
        float buttonBottom = -1.0f + (2.0f * heightPercent);
        float buttonHeight = buttonBottom - buttonTop;
        
        // Count visible buttons
        int visibleCount = 0;
        for (const auto& btn : buttons) {
            if (btn.visible) visibleCount++;
        }
        
        if (visibleCount > 0) {
            float buttonWidth = 2.0f / static_cast<float>(visibleCount);
            
            // Draw each visible button
            int currentIndex = 0;
            for (const auto& btn : buttons) {
                if (!btn.visible) continue;
                
                float btnLeft = -1.0f + (currentIndex * buttonWidth);
                
                // Draw button background
                addQuad(frameVertices, 
                        btnLeft + layout.buttonInset, 
                        buttonTop + layout.buttonInset, 
                        buttonWidth - layout.buttonInset * 2, 
                        buttonHeight - layout.buttonInset * 2,
                        btn.color[0], btn.color[1], btn.color[2]);
                
                // Draw button label (centered)
                int labelLen = btn.label.length();
                float labelWidth = labelLen * layout.buttonTextSpacing;
                float labelX = btnLeft + (buttonWidth - labelWidth) * 0.5f;
                float labelY = buttonTop + (buttonHeight * layout.buttonTextYOffset);
                
                for (int j = 0; j < labelLen; j++) {
                    DrawChar(frameVertices, btn.label[j], 
                            labelX + j * layout.buttonTextSpacing, labelY, 
                            layout.buttonTextScale, 0.06f, 
                            btn.textColor[0], btn.textColor[1], btn.textColor[2]);
                }
                
                currentIndex++;
            }
        }
    }

    // Layout Settings for debug text
    float startX = 0.05f;
    float startY = -0.60f;
    float lineDist = 0.08f;
    
    // Draw all registered debug strings
    int lineIndex = 0;
    for (auto const& [slot, text] : m_debugStrings) {
        float x = startX;
        float y = startY + (lineIndex * lineDist);
        lineIndex++;
        
        for (char c : text) {
            if (c == ' ') {
                x += 0.03f;
                continue;
            }
            DrawChar(frameVertices, c, x, y, 0.035f, 0.06f, 1.0f, 1.0f, 1.0f);
            x += 0.045f;
        }
    }
    
    // Draw Joystick (Configurable)
    if (uiConfig && inputSystem) {
        float joyX = 0.0f, joyY = 0.0f;
        auto* androidInput = static_cast<SecretEngine::AndroidInput*>(inputSystem);
        androidInput->GetJoystickPosition(joyX, joyY);
        DrawJoystick(frameVertices, uiConfig->GetJoystick(), joyX, joyY);
    }

    // Update GPU (Offset by Sky Count)
    uint32_t offset = m_skyVertexCount * sizeof(Vertex2D);
    
    // Safety check buffer size
    if (frameVertices.size() > 2000) frameVertices.resize(2000);
    
    // Map & Copy
    void* data;
    vkMapMemory(m_device->GetDevice(), m_2dVertexMemory, offset, sizeof(Vertex2D) * frameVertices.size(), 0, &data);
    memcpy(data, frameVertices.data(), sizeof(Vertex2D) * frameVertices.size());
    vkUnmapMemory(m_device->GetDevice(), m_2dVertexMemory);
    
    // Draw Text & Joystick
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_2dPipeline);
    struct { float sX, sY, oX, oY; } pc = {1.0f, 1.0f, 0.0f, 0.0f}; // Reset offset to 0
    vkCmdPushConstants(cmd, m_2dPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
    
    VkBuffer vertexBuffers[] = {m_2dVertexBuffer};
    VkDeviceSize voffsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, voffsets);
    
    vkCmdDraw(cmd, (uint32_t)frameVertices.size(), 1, m_skyVertexCount, 0);
}

extern "C" {
    // Standard plugin entry point for dynamic loading
    SecretEngine::IPlugin* CreatePlugin() {
        return new RendererPlugin();
    }
}

// Static linkage entry point for Android
extern "C" SecretEngine::IPlugin* CreateVulkanRendererPlugin() {
    return new RendererPlugin();
}

std::string RendererPlugin::LoadAssetAsString(const char* filename) {
    return m_core->GetAssetProvider()->LoadText(filename);
}

void RendererPlugin::CleanupSwapchain() {
    if (!m_device) return;
    VkDevice dev = m_device->GetDevice();

    for (auto framebuffer : m_framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(dev, framebuffer, nullptr);
        }
    }
    m_framebuffers.clear();

    if (m_swapchain) {
        m_swapchain->Cleanup();
    }
}

bool RendererPlugin::RecreateSwapchain() {
    if (!m_device || !m_swapchain) return false;
    
    m_core->GetLogger()->LogInfo("VulkanRenderer", "Recreating Swapchain...");
    vkDeviceWaitIdle(m_device->GetDevice());
    
    CleanupSwapchain();

    // Get new window size from surface
    if (!m_swapchain->Create(m_device->GetSurface(), 0, 0)) {
        m_core->GetLogger()->LogError("VulkanRenderer", "Failed to recreate swapchain!");
        return false;
    }

    if (!CreateFramebuffers()) {
        m_core->GetLogger()->LogError("VulkanRenderer", "Failed to recreate framebuffers!");
        return false;
    }

    m_core->GetLogger()->LogInfo("VulkanRenderer", "✓ Swapchain recreated successfully");
    return true;
}

// ============================================================================
// PLUGIN SYSTEM INTEGRATION
// ============================================================================

bool RendererPlugin::CreatePluginBuffers() {
    auto logger = m_core ? m_core->GetLogger() : nullptr;
    
    // Create Light Buffer (256 lights × 64 bytes = 16KB)
    VkDeviceSize lightBufferSize = 256 * 64;
    if (!Helpers::CreateBuffer(m_device->GetDevice(), m_device->GetPhysicalDevice(), lightBufferSize,
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m_lightBuffer, m_lightMemory)) {
        if (logger) logger->LogError("VulkanRenderer", "Failed to create light buffer");
        return false;
    }
    
    // Create Material Buffer (4096 materials × 64 bytes = 256KB)
    VkDeviceSize materialBufferSize = 4096 * 64;
    if (!Helpers::CreateBuffer(m_device->GetDevice(), m_device->GetPhysicalDevice(), materialBufferSize,
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m_materialBuffer, m_materialMemory)) {
        if (logger) logger->LogError("VulkanRenderer", "Failed to create material buffer");
        return false;
    }
    
    // Create descriptor set layouts and sets for lights and materials
    // (Simplified - in production you'd create proper descriptor sets)
    
    if (logger) logger->LogInfo("VulkanRenderer", "✓ Plugin buffers created (16KB lights + 256KB materials)");
    return true;
}

void RendererPlugin::UpdateLightBuffer() {
    if (!m_lightingSystem || !m_lightBuffer) return;
    
    // C++26: Use std::span for type-safe buffer access
    auto lightData = m_lightingSystem->GetLightBuffer();
    
    if (!lightData.empty()) {
        void* mapped;
        vkMapMemory(m_device->GetDevice(), m_lightMemory, 0, lightData.size_bytes(), 0, &mapped);
        memcpy(mapped, lightData.data(), lightData.size_bytes());
        vkUnmapMemory(m_device->GetDevice(), m_lightMemory);
    }
}

void RendererPlugin::UpdateMaterialBuffer() {
    if (!m_materialSystem || !m_materialBuffer) return;
    
    // C++26: Use std::span for type-safe buffer access
    auto materialData = m_materialSystem->GetMaterialBuffer();
    
    if (!materialData.empty()) {
        void* mapped;
        vkMapMemory(m_device->GetDevice(), m_materialMemory, 0, materialData.size_bytes(), 0, &mapped);
        memcpy(mapped, materialData.data(), materialData.size_bytes());
        vkUnmapMemory(m_device->GetDevice(), m_materialMemory);
    }
}

void RendererPlugin::RenderShadowPass(VkCommandBuffer cmd) {
    if (!m_shadowSystem) return;
    
    // Shadow pass rendering would go here
    // For now, this is a placeholder for future implementation
    // In production, you would:
    // 1. Begin shadow pass for each shadow map
    // 2. Render geometry from light's perspective
    // 3. End shadow pass
    
    // Example:
    // auto shadowHandle = m_shadowSystem->GetShadowMapHandle(0);
    // m_shadowSystem->BeginShadowPass(shadowHandle);
    // ... render geometry ...
    // m_shadowSystem->EndShadowPass(shadowHandle);
}

void RendererPlugin::DispatchVolumetricLighting(VkCommandBuffer cmd) {
    if (!m_shadowSystem) return;
    
    // Volumetric lighting compute dispatch would go here
    // For now, this is a placeholder for future implementation
    // In production, you would:
    // 1. Bind volumetric lighting compute pipeline
    // 2. Bind shadow maps as input
    // 3. Dispatch compute shader
    // 4. Output to 3D texture for main pass
    
    // Example:
    // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_volumetricPipeline);
    // vkCmdDispatch(cmd, workGroupsX, workGroupsY, workGroupsZ);
}



// ============================================================================
// GPU TIMING WITH VULKAN TIMESTAMPS
// ============================================================================

bool RendererPlugin::CreateTimestampQueries() {
    if (!m_device) return false;
    
    // Check if timestamps are supported
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_device->GetPhysicalDevice(), &props);
    
    if (props.limits.timestampComputeAndGraphics == VK_FALSE) {
        m_timestampsSupported = false;
        return false;
    }
    
    m_timestampPeriod = props.limits.timestampPeriod;
    m_timestampsSupported = true;
    
    // Create query pool for 2 timestamps (start and end)
    VkQueryPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    poolInfo.queryCount = 2;
    
    if (vkCreateQueryPool(m_device->GetDevice(), &poolInfo, nullptr, &m_queryPool) != VK_SUCCESS) {
        m_timestampsSupported = false;
        return false;
    }
    
    return true;
}

void RendererPlugin::RecordGPUTimestamps(VkCommandBuffer cmd) {
    if (!m_timestampsSupported || !m_queryPool) return;
    
    // Reset queries at the start of the command buffer
    vkCmdResetQueryPool(cmd, m_queryPool, 0, 2);
    
    // Write start timestamp
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_queryPool, 0);
}

void RendererPlugin::ReadGPUTimestamps() {
    if (!m_timestampsSupported || !m_queryPool) {
        // Set GPU time to 0 if not supported
        auto& profiler = SecretEngine::Profiler::Instance();
        profiler.GetStats().gpu_frame_time.store(0.0f, std::memory_order_relaxed);
        return;
    }
    
    // Read timestamp results
    uint64_t timestamps[2] = {0, 0};
    VkResult result = vkGetQueryPoolResults(
        m_device->GetDevice(),
        m_queryPool,
        0, 2,
        sizeof(timestamps),
        timestamps,
        sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
    );
    
    if (result == VK_SUCCESS && timestamps[1] > timestamps[0]) {
        // Calculate GPU time in milliseconds
        uint64_t delta = timestamps[1] - timestamps[0];
        float gpuTimeMs = (delta * m_timestampPeriod) / 1000000.0f;
        
        // Update profiler stats
        auto& profiler = SecretEngine::Profiler::Instance();
        profiler.GetStats().gpu_frame_time.store(gpuTimeMs, std::memory_order_relaxed);
    }
}
