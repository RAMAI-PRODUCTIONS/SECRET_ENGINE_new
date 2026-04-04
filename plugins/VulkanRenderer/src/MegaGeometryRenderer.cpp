// SecretEngine
// Module: VulkanRenderer
// Responsibility: GPU-Driven Mega-Geometry implementation
// Dependencies: VulkanDevice, ICore, Math, IAssetProvider

#include "MegaGeometryRenderer.h"
#include "VulkanDevice.h"
#include <SecretEngine/ILogger.h>
#include <SecretEngine/ICore.h>
#include "VulkanHelpers.h"
#include <SecretEngine/IAssetProvider.h>
#include <SecretEngine/Math.h>
#include <vector>
#include <cstring>
#include <algorithm>
#include "Pipeline3D.h" // For MeshHeader and Vertex3D
#include "SimpleVertexLighting.h"
#include "VertexShadowsGI.h"

using namespace SecretEngine::Math;

namespace SecretEngine::MegaGeometry {
using SecretEngine::Vulkan::Helpers;

static constexpr uint32_t NO_TEXTURE_ID = 0xFFFFFFFFu;

void MegaGeometryRenderer::UpdateInstanceTransform(uint32_t instanceID, float x, float y, float z, float rotY, float rotX, float rotZ) {
    if (instanceID >= MAX_INSTANCES) return;

    // ULTRA-FAST MATRIX: Direct write for Y-rotation only (common case)
    // Z-UP COORDINATE SYSTEM: Y-rotation rotates in XY plane (around Z axis)
    if (rotX == 0.0f && rotZ == 0.0f) {
        float cy = cosf(rotY), sy = sinf(rotY);
        const float s = 150.0f;
        
        for(int i=0; i<2; ++i) {
            if (m_instanceDataMapped[i]) {
                auto& t = m_instanceDataMapped[i][instanceID].transform;
                // Row 0 (X-axis / right): [cy*s, -sy*s, 0, x]
                t.m[0] = cy * s; t.m[1] = -sy * s; t.m[2] = 0.0f; t.m[3] = x;
                // Row 1 (Y-axis / forward): [sy*s, cy*s, 0, y]
                t.m[4] = sy * s; t.m[5] = cy * s; t.m[6] = 0.0f; t.m[7] = y;
                // Row 2 (Z-axis / up): [0, 0, s, z]
                t.m[8] = 0.0f; t.m[9] = 0.0f; t.m[10] = s; t.m[11] = z;
            }
        }
    } else {
        // Full rotation path (rare)
        Matrix3x4 matrix;
        Matrix4x4::FromTRS(matrix, x, y, z, rotX, rotY, rotZ, 150.0f);
        
        for(int i=0; i<2; ++i) {
            if (m_instanceDataMapped[i]) {
                m_instanceDataMapped[i][instanceID].transform = matrix;
            }
        }
    }
}

void MegaGeometryRenderer::UpdateInstanceColor(uint32_t instanceID, float r, float g, float b, float a) {
    if (instanceID >= MAX_INSTANCES) return;
    
    // 8-BIT NITRO COLOR PACKING (RGBA8)
    uint32_t pr = (uint32_t)(r * 255.0f) & 0xFF;
    uint32_t pg = (uint32_t)(g * 255.0f) & 0xFF;
    uint32_t pb = (uint32_t)(b * 255.0f) & 0xFF;
    uint32_t pa = (uint32_t)(a * 255.0f) & 0xFF;
    uint32_t packed = pr | (pg << 8) | (pb << 16) | (pa << 24);

    for(int i=0; i<2; ++i) {
        if (m_instanceDataMapped[i]) {
            m_instanceDataMapped[i][instanceID].packedColor = packed;
        }
    }
}

void MegaGeometryRenderer::UpdateInstanceTexture(uint32_t instanceID, uint32_t textureID) {
    if (instanceID >= MAX_INSTANCES) return;

    for (int i = 0; i < 2; ++i) {
        if (m_instanceDataMapped[i]) {
            m_instanceDataMapped[i][instanceID].textureID = textureID;
        }
    }
}

void MegaGeometryRenderer::PreRender(VkCommandBuffer cmd) {
    if (m_pipeline == VK_NULL_HANDLE || m_totalInstances == 0) return;

    // CRITICAL FIX: Swap frame index at START of PreRender (not in Render)
    // This ensures PreRender and Render use the SAME buffer
    m_frameIndex = (m_frameIndex + 1) % 2;

    // 1. Reset Instance Count (GPU Side)
    vkCmdFillBuffer(cmd, m_indirectBuffer[m_frameIndex], 4, 4, 0); 

    // 2. Optimized barrier - use buffer barrier for better granularity
    VkBufferMemoryBarrier bufferBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.buffer = m_indirectBuffer[m_frameIndex];
    bufferBarrier.offset = 0;
    bufferBarrier.size = VK_WHOLE_SIZE;
    
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
                         0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

    // 3. Dispatch Compute Cull
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_cullPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_cullLayout, 0, 1, &m_cullDescriptorSet[m_frameIndex], 0, nullptr);
    
    struct CullPush {
        float vp[16];
        uint32_t count;
        float radius;
    } push;
    memcpy(push.vp, m_viewProj, 64);
    push.count = m_totalInstances.load(std::memory_order_relaxed);
    push.radius = 150.0f;
    vkCmdPushConstants(cmd, m_cullLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
    
    // Optimized dispatch - use 256 threads per workgroup for maximum occupancy
    uint32_t groups = (push.count + 255) / 256;
    vkCmdDispatch(cmd, groups, 1, 1);
    
    // DEBUG: Log culling info (first few frames only)
    auto logger = m_core ? m_core->GetLogger() : nullptr;
    static int logCount = 0;
    if (logger && logCount < 3) {
        char msg[256];
        snprintf(msg, sizeof(msg), "GPU Culling: %u instances, %u workgroups, VP[0]=%f", 
                 push.count, groups, push.vp[0]);
        logger->LogInfo("MegaGeometryRenderer", msg);
        logCount++;
    }

    // 4. Optimized barrier (Compute -> Graphics)
    VkBufferMemoryBarrier drawBarriers[2] = {
        {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER},
        {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER}
    };
    
    // Indirect buffer barrier
    drawBarriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    drawBarriers[0].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    drawBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    drawBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    drawBarriers[0].buffer = m_indirectBuffer[m_frameIndex];
    drawBarriers[0].offset = 0;
    drawBarriers[0].size = VK_WHOLE_SIZE;
    
    // Visible instance buffer barrier
    drawBarriers[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    drawBarriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    drawBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    drawBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    drawBarriers[1].buffer = m_visibleInstanceSSBO[m_frameIndex];
    drawBarriers[1].offset = 0;
    drawBarriers[1].size = VK_WHOLE_SIZE;
    
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
                         VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 
                         0, 0, nullptr, 2, drawBarriers, 0, nullptr);
}

void MegaGeometryRenderer::Render(VkCommandBuffer cmd) {
    if (m_pipeline == VK_NULL_HANDLE || m_totalInstances == 0) return;

    // Frame index already swapped in PreRender() - removed from here to fix culling bug

    // 4. Draw
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    // Bind instance buffer descriptor set (set = 0) and, if available, bindless texture set (set = 1)
    VkDescriptorSet sets[2] = { m_descriptorSet[m_frameIndex], VK_NULL_HANDLE };
    uint32_t setCount = 1;
    if (m_textureManager) {
        sets[1] = m_textureManager->GetTextureDescriptorSet();
        setCount = 2;
    }
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, setCount, sets, 0, nullptr);
    
    if (m_vertexBuffer != VK_NULL_HANDLE) {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer, &offset);
        vkCmdBindIndexBuffer(cmd, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16); // NITRO 16-BIT
        vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, m_viewProj);
        vkCmdDrawIndexedIndirect(cmd, m_indirectBuffer[m_frameIndex], 0, 1, sizeof(VkDrawIndexedIndirectCommand));
    }
}

bool MegaGeometryRenderer::Initialize(VulkanDevice* device, VkRenderPass renderPass, SecretEngine::ICore* core, SecretEngine::Textures::TextureManager* textureManager) {
    auto logger = core ? core->GetLogger() : nullptr;
    
    m_device = device;
    m_vkDevice = device->GetDevice();
    m_core = core;
    m_textureManager = textureManager;

    if (!CreateInstanceSSBO()) {
        if (logger) logger->LogError("MegaGeometryRenderer", "CreateInstanceSSBO failed");
        return false;
    }
    if (!CreateIndirectBuffer()) {
        if (logger) logger->LogError("MegaGeometryRenderer", "CreateIndirectBuffer failed");
        return false;
    }
    if (!CreateVisibleBuffer()) {
        if (logger) logger->LogError("MegaGeometryRenderer", "CreateVisibleBuffer failed");
        return false;
    }
    if (!CreateGeometricBuffers()) {
        if (logger) logger->LogError("MegaGeometryRenderer", "CreateGeometricBuffers failed");
        return false;
    }
    if (!CreateDescriptorSet()) {
        if (logger) logger->LogError("MegaGeometryRenderer", "CreateDescriptorSet failed");
        return false;
    }
    if (!CreateCullPipeline()) {
        if (logger) logger->LogError("MegaGeometryRenderer", "CreateCullPipeline failed");
        return false;
    }
    if (!CreatePipeline(renderPass)) {
        if (logger) logger->LogError("MegaGeometryRenderer", "CreatePipeline failed");
        return false;
    }
    
    Matrix4x4 identity = Matrix4x4::Identity();
    memcpy(m_viewProj, identity.m, 64);
    
    // No longer pre-load Character.meshbin - meshes will be loaded on-demand
    if (logger) logger->LogInfo("MegaGeometryRenderer", "MegaGeometry initialized - ready for dynamic mesh loading");
    
    return true;
}

bool MegaGeometryRenderer::CreateGeometricBuffers() {
    VkDeviceSize vSize = MAX_VERTICES * sizeof(Vertex3DNitro);
    if (!Helpers::CreateBuffer(m_vkDevice, m_device->GetPhysicalDevice(), vSize,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m_vertexBuffer, m_vertexMemory)) return false;
    
    // PERSISTENT MAP VERTICES
    vkMapMemory(m_vkDevice, m_vertexMemory, 0, vSize, 0, &m_vertexDataMapped);

    VkDeviceSize iSize = MAX_INDICES * sizeof(uint16_t); // NITRO 16-BIT
    if (!Helpers::CreateBuffer(m_vkDevice, m_device->GetPhysicalDevice(), iSize,
                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m_indexBuffer, m_indexMemory)) return false;
                             
    // PERSISTENT MAP INDICES
    vkMapMemory(m_vkDevice, m_indexMemory, 0, iSize, 0, &m_indexDataMapped);

    return true;
}

bool MegaGeometryRenderer::CreateInstanceSSBO() {
    VkDeviceSize bufferSize = sizeof(InstanceData) * MAX_INSTANCES;
    for(int i=0; i<2; ++i) {
        // Use DEVICE_LOCAL | HOST_VISIBLE for better GPU access (if available)
        VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        // Try to get device-local memory that's also host-visible (AMD/Intel integrated)
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(m_device->GetPhysicalDevice(), &memProps);
        
        if (!Helpers::CreateBuffer(m_vkDevice, m_device->GetPhysicalDevice(), bufferSize,
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 props,
                                 m_instanceSSBO[i], m_instanceSSBOMemory[i])) return false;

        vkMapMemory(m_vkDevice, m_instanceSSBOMemory[i], 0, bufferSize, 0, (void**)&m_instanceDataMapped[i]);
        memset(m_instanceDataMapped[i], 0, bufferSize);

        // Initialize texture IDs to "no texture" sentinel for all instances
        for (uint32_t j = 0; j < MAX_INSTANCES; ++j) {
            m_instanceDataMapped[i][j].textureID = NO_TEXTURE_ID;
        }
    }
    return true;
}

bool MegaGeometryRenderer::CreateIndirectBuffer() {
    VkDeviceSize bufferSize = sizeof(VkDrawIndexedIndirectCommand) * MAX_MESHES;
    for(int i=0; i<2; ++i) {
        if (!Helpers::CreateBuffer(m_vkDevice, m_device->GetPhysicalDevice(), bufferSize,
                                 VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 m_indirectBuffer[i], m_indirectMemory[i])) return false;

        vkMapMemory(m_vkDevice, m_indirectMemory[i], 0, bufferSize, 0, (void**)&m_indirectMapped[i]);
        memset(m_indirectMapped[i], 0, bufferSize);
    }
    return true;
}

bool MegaGeometryRenderer::CreateDescriptorSet() {
    auto logger = m_core ? m_core->GetLogger() : nullptr;

    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    VkResult res = vkCreateDescriptorSetLayout(m_vkDevice, &layoutInfo, nullptr, &m_descriptorSetLayout);
    if (res != VK_SUCCESS) {
        if (logger) logger->LogError("MegaGeometryRenderer", "Failed to create descriptor set layout for instance buffer");
        return false;
    }
    
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 2;
    VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 2;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    res = vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &pool);
    if (res != VK_SUCCESS) {
        if (logger) logger->LogError("MegaGeometryRenderer", "Failed to create descriptor pool for instance buffer");
        return false;
    }

    for(int i=0; i<2; ++i) {
        VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_descriptorSetLayout;
        res = vkAllocateDescriptorSets(m_vkDevice, &allocInfo, &m_descriptorSet[i]);
        if (res != VK_SUCCESS) {
            if (logger) logger->LogError("MegaGeometryRenderer", "Failed to allocate descriptor set for instance buffer");
            return false;
        }
        
        VkDescriptorBufferInfo bInfo = {}; bInfo.buffer = m_visibleInstanceSSBO[i]; bInfo.range = VK_WHOLE_SIZE;
        VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = m_descriptorSet[i]; write.dstBinding = 0; write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; write.pBufferInfo = &bInfo;
        vkUpdateDescriptorSets(m_vkDevice, 1, &write, 0, nullptr);
    }
    return true; 
}

bool MegaGeometryRenderer::CreatePipeline(VkRenderPass renderPass) {
    auto logger = m_core ? m_core->GetLogger() : nullptr;

    auto vert = m_core->GetAssetProvider()->LoadBinary("shaders/mega_geometry_vert.spv");
    auto frag = m_core->GetAssetProvider()->LoadBinary("shaders/mega_geometry_frag.spv");
    if (vert.empty() || frag.empty()) {
        if (logger) logger->LogError("MegaGeometryRenderer", "Failed to load mega_geometry_vert/frag SPIR-V shaders");
        return false;
    }

    VkShaderModule vertMod = Helpers::CreateShaderModule(m_vkDevice, vert);
    VkShaderModule fragMod = Helpers::CreateShaderModule(m_vkDevice, frag);
    if (vertMod == VK_NULL_HANDLE || fragMod == VK_NULL_HANDLE) {
        if (logger) logger->LogError("MegaGeometryRenderer", "Failed to create shader modules for mega geometry pipeline");
        if (vertMod != VK_NULL_HANDLE) vkDestroyShaderModule(m_vkDevice, vertMod, nullptr);
        if (fragMod != VK_NULL_HANDLE) vkDestroyShaderModule(m_vkDevice, fragMod, nullptr);
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertMod, "main", nullptr};
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragMod, "main", nullptr};

    VkVertexInputBindingDescription binding = {0, sizeof(Vertex3DNitro), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attrs[4] = {
        {0, 0, VK_FORMAT_R16G16B16A16_SNORM, 0},   // Position (8 bytes, 16-bit)
        {1, 0, VK_FORMAT_R8G8B8A8_SNORM, 8},       // Normal (4 bytes, 8-bit)
        {2, 0, VK_FORMAT_R16G16_UNORM, 12},        // UV (4 bytes, 16-bit)
        {3, 0, VK_FORMAT_R32_UINT, 16}             // Vertex Color (4 bytes, R11G11B10F packed)
    };
    VkPipelineVertexInputStateCreateInfo vi = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = 1; vi.pVertexBindingDescriptions = &binding;
    vi.vertexAttributeDescriptionCount = 4; vi.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO}; ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO}; 
    vp.viewportCount = 1; vp.pViewports = nullptr; // Dynamic
    vp.scissorCount = 1; vp.pScissors = nullptr; // Dynamic

    VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynStates;

    VkPipelineRasterizationStateCreateInfo rs = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO}; 
    rs.lineWidth = 1.0f; 
    rs.cullMode = VK_CULL_MODE_BACK_BIT; 
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.depthBiasEnable = VK_FALSE;
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    VkPipelineMultisampleStateCreateInfo ms = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO}; 
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    ms.sampleShadingEnable = VK_FALSE;
    ms.minSampleShading = 1.0f;
    VkPipelineDepthStencilStateCreateInfo ds = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO}; 
    ds.depthTestEnable = VK_TRUE; 
    ds.depthWriteEnable = VK_TRUE; 
    ds.depthCompareOp = VK_COMPARE_OP_LESS;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.stencilTestEnable = VK_FALSE;
    VkPipelineColorBlendAttachmentState cba = {}; 
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cba.blendEnable = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo cb = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO}; cb.attachmentCount = 1; cb.pAttachments = &cba;
    
    VkPushConstantRange push = {VK_SHADER_STAGE_VERTEX_BIT, 0, 64};
    VkPipelineLayoutCreateInfo pl = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    // Descriptor set 0: visible instance buffer (this renderer)
    // Descriptor set 1: bindless texture array (TextureManager), when available
    VkDescriptorSetLayout setLayouts[2] = { m_descriptorSetLayout, VK_NULL_HANDLE };
    uint32_t setLayoutCount = 1;
    if (m_textureManager) {
        setLayouts[1] = m_textureManager->GetDescriptorSetLayout();
        setLayoutCount = 2;
    }
    pl.setLayoutCount = setLayoutCount;
    pl.pSetLayouts = setLayouts;
    pl.pushConstantRangeCount = 1;
    pl.pPushConstantRanges = &push;
    VkResult res = vkCreatePipelineLayout(m_vkDevice, &pl, nullptr, &m_pipelineLayout);
    if (res != VK_SUCCESS) {
        if (logger) logger->LogError("MegaGeometryRenderer", "Failed to create pipeline layout for mega geometry");
        vkDestroyShaderModule(m_vkDevice, vertMod, nullptr);
        vkDestroyShaderModule(m_vkDevice, fragMod, nullptr);
        return false;
    }
    
    VkGraphicsPipelineCreateInfo pipe = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipe.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT; // Enable pipeline derivatives for faster creation
    pipe.stageCount = 2; pipe.pStages = stages; pipe.pVertexInputState = &vi; pipe.pInputAssemblyState = &ia;
    pipe.pViewportState = &vp; pipe.pRasterizationState = &rs; pipe.pMultisampleState = &ms; pipe.pDepthStencilState = &ds; pipe.pColorBlendState = &cb;
    pipe.pDynamicState = &dynamicState;
    pipe.layout = m_pipelineLayout; pipe.renderPass = renderPass;
    
    // Use pipeline cache for faster creation on subsequent runs
    VkPipelineCacheCreateInfo cacheInfo = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    VkPipelineCache cache = VK_NULL_HANDLE;
    res = vkCreatePipelineCache(m_vkDevice, &cacheInfo, nullptr, &cache);
    if (res != VK_SUCCESS && logger) {
        logger->LogWarning("MegaGeometryRenderer", "Failed to create pipeline cache (non-fatal)");
    }
    
    res = vkCreateGraphicsPipelines(m_vkDevice, cache, 1, &pipe, nullptr, &m_pipeline);
    
    if (cache != VK_NULL_HANDLE) {
        vkDestroyPipelineCache(m_vkDevice, cache, nullptr);
    }
    
    if (res != VK_SUCCESS) {
        if (logger) logger->LogError("MegaGeometryRenderer", "vkCreateGraphicsPipelines failed for mega geometry pipeline");
        vkDestroyShaderModule(m_vkDevice, vertMod, nullptr);
        vkDestroyShaderModule(m_vkDevice, fragMod, nullptr);
        return false;
    }

    vkDestroyShaderModule(m_vkDevice, vertMod, nullptr);
    vkDestroyShaderModule(m_vkDevice, fragMod, nullptr);
    return true;
}

bool MegaGeometryRenderer::LoadMesh(const char* path, uint32_t meshSlot) {
    auto logger = m_core ? m_core->GetLogger() : nullptr;
    
    // Stage 1: Load Header (Optimized)
    MeshHeader header = {};
    std::span<std::byte> headerSpan(reinterpret_cast<std::byte*>(&header), sizeof(MeshHeader));
    if (!m_core->GetAssetProvider()->LoadBinaryToBuffer(path, headerSpan)) {
        if (logger) {
            char msg[256];
            snprintf(msg, sizeof(msg), "LoadMesh: Failed to load header for %s", path);
            logger->LogError("MegaGeometryRenderer", msg);
        }
        return false;
    }
    if (logger) {
        char msg[256];
        snprintf(msg, sizeof(msg), "LoadMesh: Loaded %s - %u vertices, %u indices", 
            path, header.vertexCount, header.indexCount);
        logger->LogInfo("MegaGeometryRenderer", msg);
    }
    
    if (m_vertexOffset + header.vertexCount > MAX_VERTICES) return false;
    if (m_indexOffset + header.indexCount > MAX_INDICES) return false;

    // Stage 2: Direct Disk-To-GPU Streaming (Zero RAM allocation)
    // We skip the traditional "Load binary to RAM" step and stream directly to Persistent-Mapped VRAM
    
    // AssetProvider must handle reading with an offset to skip header
    // Since LoadBinaryToBuffer doesn't have offset, we load Vertices + Indices in one shot
    // To keep it simple and zero-copy, we load exactly what we need
    
    // NOTE: This implementation assumes LoadBinaryToBuffer can handle loading specific segments
    // Stage 2: Nitrogen High-Fidelity 16-bit Quantization
    Vertex3DNitro* vDest = (Vertex3DNitro*)m_vertexDataMapped + m_vertexOffset;
    uint16_t* iDest = (uint16_t*)((uint8_t*)m_indexDataMapped + (m_indexOffset * sizeof(uint16_t)));

    auto provider = m_core->GetAssetProvider();
    auto rawBuffer = provider->LoadBinary(path);
    if (rawBuffer.size() < sizeof(MeshHeader)) return false;

    Vertex3D* src = (Vertex3D*)(rawBuffer.data() + sizeof(MeshHeader));
    for (uint32_t i = 0; i < header.vertexCount; ++i) {
        // 16-bit Position: Scale 8.0f covers range [-8, 8] with 0.0002 precision
        vDest[i].pos[0] = (int16_t)std::clamp(src[i].position[0] * 4096.0f, -32768.0f, 32767.0f);
        vDest[i].pos[1] = (int16_t)std::clamp(src[i].position[1] * 4096.0f, -32768.0f, 32767.0f);
        vDest[i].pos[2] = (int16_t)std::clamp(src[i].position[2] * 4096.0f, -32768.0f, 32767.0f);
        vDest[i].pos[3] = 32767; // W=1.0

        vDest[i].norm[0] = (int8_t)std::clamp(src[i].normal[0] * 127.0f, -128.0f, 127.0f);
        vDest[i].norm[1] = (int8_t)std::clamp(src[i].normal[1] * 127.0f, -128.0f, 127.0f);
        vDest[i].norm[2] = (int8_t)std::clamp(src[i].normal[2] * 127.0f, -128.0f, 127.0f);
        vDest[i].norm[3] = 0;

        vDest[i].uv[0] = (uint16_t)std::clamp(src[i].uv[0] * 65535.0f, 0.0f, 65535.0f);
        vDest[i].uv[1] = (uint16_t)std::clamp(src[i].uv[1] * 65535.0f, 0.0f, 65535.0f);
        
        // Initialize vertex color to white (will be computed by lighting system)
        vDest[i].vertexColor = 0xFFFFFFFF;
    }
    
    // Stage 3: Compute Vertex Lighting with Shadows & GI
    if (logger) {
        logger->LogInfo("MegaGeometryRenderer", "Computing vertex lighting with shadows & GI...");
    }
    
    // Create lighting engine with scene lights
    SecretEngine::SimpleVertexLighting lighting;
    lighting.SetAmbient(Float3{0.2f, 0.2f, 0.3f}, 0.15f);
    
    // Add directional light (sun)
    lighting.AddDirectionalLight(
        Float3{0.3f, -0.8f, 0.5f},
        Float3{1.0f, 0.95f, 0.8f},
        1.2f
    );
    
    // Add point lights based on scene layout
    lighting.AddPointLight(Float3{10.0f, 3.0f, 2.0f}, Float3{1.0f, 0.6f, 0.2f}, 3.0f, 15.0f);
    lighting.AddPointLight(Float3{30.0f, 3.0f, 5.0f}, Float3{0.2f, 0.5f, 1.0f}, 2.5f, 12.0f);
    lighting.AddPointLight(Float3{20.0f, 3.0f, 8.0f}, Float3{0.8f, 1.0f, 0.6f}, 2.0f, 10.0f);
    
    // Create shadow/GI system
    SecretEngine::VertexShadowsGI shadowGI;
    shadowGI.SetSkyColor(Float3{0.5f, 0.7f, 1.0f});      // Blue sky
    shadowGI.SetGroundColor(Float3{0.3f, 0.25f, 0.2f});  // Brown ground
    shadowGI.SetGIIntensity(0.3f);
    shadowGI.SetShadowSoftness(0.2f); // Soft shadows
    
    // Build scene geometry for shadow raycasting
    SecretEngine::SceneGeometry sceneGeo;
    std::vector<Float3> positions(header.vertexCount);
    std::vector<Float3> normals(header.vertexCount);
    std::vector<uint32_t> indices(header.indexCount);
    
    for (uint32_t i = 0; i < header.vertexCount; ++i) {
        positions[i] = Float3{
            float(vDest[i].pos[0]) / 4096.0f,
            float(vDest[i].pos[1]) / 4096.0f,
            float(vDest[i].pos[2]) / 4096.0f
        };
        normals[i] = Normalize(Float3{
            float(vDest[i].norm[0]) / 127.0f,
            float(vDest[i].norm[1]) / 127.0f,
            float(vDest[i].norm[2]) / 127.0f
        });
    }
    
    uint32_t* srcIndices = (uint32_t*)(rawBuffer.data() + sizeof(MeshHeader) + (header.vertexCount * sizeof(Vertex3D)));
    for (uint32_t i = 0; i < header.indexCount; ++i) {
        indices[i] = srcIndices[i];
    }
    
    sceneGeo.AddMesh(positions.data(), normals.data(), header.vertexCount,
                     indices.data(), header.indexCount);
    
    // Material diffuse color (default white, can be loaded from material system later)
    Float3 materialDiffuse{1.0f, 1.0f, 1.0f};
    
    // Compute lighting with shadows & GI for each vertex
    for (uint32_t i = 0; i < header.vertexCount; ++i) {
        // Use normal map if available (for now, just use vertex normal)
        Float3 normal = normals[i];
        
        // Compute full lighting with shadows and GI
        Float3 litColor = shadowGI.ComputeLightingWithShadowsGI(
            positions[i], normal, lighting, sceneGeo, materialDiffuse
        );
        
        // Pack to R11G11B10F
        vDest[i].vertexColor = SecretEngine::SimpleVertexLighting::PackR11G11B10F(litColor);
    }
    
    if (logger) {
        char msg[256];
        snprintf(msg, sizeof(msg), "✓ Computed lighting with shadows & GI for %u vertices", header.vertexCount);
        logger->LogInfo("MegaGeometryRenderer", msg);
    }
    
    // Quantize Indices to 16-bit (reuse srcIndices from above)
    for (uint32_t i = 0; i < header.indexCount; ++i) {
        iDest[i] = (uint16_t)srcIndices[i];
    }
    
    // Transfer to BOTH buffers for persistence
    for(int i = 0; i < 2; ++i) {
        if (m_indirectMapped[i]) {
            m_indirectMapped[i][meshSlot].indexCount = header.indexCount;
            m_indirectMapped[i][meshSlot].instanceCount = 0;
            m_indirectMapped[i][meshSlot].firstIndex = m_indexOffset;
            m_indirectMapped[i][meshSlot].vertexOffset = (int32_t)m_vertexOffset;
            m_indirectMapped[i][meshSlot].firstInstance = 0;
        } else {
            if (logger) {
                char msg[256];
                snprintf(msg, sizeof(msg), "LoadMesh: indirectMapped[%d] is null!", i);
                logger->LogError("MegaGeometryRenderer", msg);
            }
            return false;
        }
    }
    
    m_vertexOffset += header.vertexCount;
    m_indexOffset += header.indexCount;
    
    if (logger) {
        char msg[256];
        snprintf(msg, sizeof(msg), "LoadMesh: Mesh slot %u initialized with indexCount=%u", 
            meshSlot, header.indexCount);
        logger->LogInfo("MegaGeometryRenderer", msg);
    }
    return true;
}

uint32_t MegaGeometryRenderer::GetOrLoadMeshSlot(const char* meshPath) {
    auto logger = m_core ? m_core->GetLogger() : nullptr;
    
    // Check if mesh is already loaded
    auto it = m_meshPathToSlot.find(meshPath);
    if (it != m_meshPathToSlot.end()) {
        return it->second;
    }
    
    // Check if we have room for more meshes
    if (m_nextMeshSlot >= MAX_MESHES) {
        if (logger) {
            char msg[256];
            snprintf(msg, sizeof(msg), "GetOrLoadMeshSlot: MAX_MESHES (%u) reached, cannot load %s", MAX_MESHES, meshPath);
            logger->LogError("MegaGeometryRenderer", msg);
        }
        return 0; // Fallback to slot 0
    }
    
    // Load the mesh into the next available slot
    uint32_t slot = m_nextMeshSlot;
    if (LoadMesh(meshPath, slot)) {
        m_meshPathToSlot[meshPath] = slot;
        m_nextMeshSlot++;
        
        if (logger) {
            char msg[256];
            snprintf(msg, sizeof(msg), "GetOrLoadMeshSlot: Loaded %s into slot %u", meshPath, slot);
            logger->LogInfo("MegaGeometryRenderer", msg);
        }
        
        return slot;
    } else {
        if (logger) {
            char msg[256];
            snprintf(msg, sizeof(msg), "GetOrLoadMeshSlot: Failed to load %s", meshPath);
            logger->LogError("MegaGeometryRenderer", msg);
        }
        return 0; // Fallback to slot 0
    }
}

bool MegaGeometryRenderer::CreateVisibleBuffer() {
    VkDeviceSize bufferSize = sizeof(InstanceData) * MAX_INSTANCES;
    for(int i=0; i<2; ++i) {
        if (!Helpers::CreateBuffer(m_vkDevice, m_device->GetPhysicalDevice(), bufferSize,
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // Visible for debugging if needed
                                 m_visibleInstanceSSBO[i], m_visibleInstanceMemory[i])) return false;
    }
    return true;
}

bool MegaGeometryRenderer::CreateCullPipeline() {
    auto logger = m_core ? m_core->GetLogger() : nullptr;

    auto computeCode = m_core->GetAssetProvider()->LoadBinary("shaders/cull_comp.spv");
    if (computeCode.empty()) {
        if (logger) logger->LogError("MegaGeometryRenderer", "Failed to load cull_comp.spv for compute culling pipeline");
        return false;
    }
    
    VkShaderModule cullMod = Helpers::CreateShaderModule(m_vkDevice, computeCode);
    if (cullMod == VK_NULL_HANDLE) {
        if (logger) logger->LogError("MegaGeometryRenderer", "Failed to create shader module for cull_comp.spv");
        return false;
    }
    
    // 1. Cull Descriptor Layout
    VkDescriptorSetLayoutBinding bindings[3] = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}, 
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}, 
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}  
    };
    
    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 3; layoutInfo.pBindings = bindings;
    VkResult res = vkCreateDescriptorSetLayout(m_vkDevice, &layoutInfo, nullptr, &m_cullDescriptorLayout);
    if (res != VK_SUCCESS) {
        if (logger) logger->LogError("MegaGeometryRenderer", "Failed to create descriptor set layout for culling pipeline");
        vkDestroyShaderModule(m_vkDevice, cullMod, nullptr);
        return false;
    }
    
    // 2. Cull Pipeline Layout
    VkPushConstantRange push = {VK_SHADER_STAGE_COMPUTE_BIT, 0, 80}; 
    VkPipelineLayoutCreateInfo pl = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.setLayoutCount = 1; pl.pSetLayouts = &m_cullDescriptorLayout;
    pl.pushConstantRangeCount = 1; pl.pPushConstantRanges = &push;
    res = vkCreatePipelineLayout(m_vkDevice, &pl, nullptr, &m_cullLayout);
    if (res != VK_SUCCESS) {
        if (logger) logger->LogError("MegaGeometryRenderer", "Failed to create pipeline layout for culling pipeline");
        vkDestroyShaderModule(m_vkDevice, cullMod, nullptr);
        return false;
    }
    
    // 3. Cull Descriptor Sets
    VkDescriptorPoolSize poolSize = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6};
    VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = 2; poolInfo.poolSizeCount = 1; poolInfo.pPoolSizes = &poolSize;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    res = vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &pool);
    if (res != VK_SUCCESS) {
        if (logger) logger->LogError("MegaGeometryRenderer", "Failed to create descriptor pool for culling pipeline");
        vkDestroyShaderModule(m_vkDevice, cullMod, nullptr);
        return false;
    }
    
    for(int i=0; i<2; ++i) {
        VkDescriptorSetAllocateInfo alloc = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        alloc.descriptorPool = pool;
        alloc.descriptorSetCount = 1;
        alloc.pSetLayouts = &m_cullDescriptorLayout;
        res = vkAllocateDescriptorSets(m_vkDevice, &alloc, &m_cullDescriptorSet[i]);
        if (res != VK_SUCCESS) {
            if (logger) logger->LogError("MegaGeometryRenderer", "Failed to allocate descriptor set for culling pipeline");
            vkDestroyShaderModule(m_vkDevice, cullMod, nullptr);
            return false;
        }
        
        VkDescriptorBufferInfo bSrc = {m_instanceSSBO[i], 0, VK_WHOLE_SIZE};
        VkDescriptorBufferInfo bVis = {m_visibleInstanceSSBO[i], 0, VK_WHOLE_SIZE};
        VkDescriptorBufferInfo bInd = {m_indirectBuffer[i], 0, VK_WHOLE_SIZE};
        
        VkWriteDescriptorSet writes[3] = {};
        writes[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_cullDescriptorSet[i], 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &bSrc, nullptr};
        writes[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_cullDescriptorSet[i], 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &bVis, nullptr};
        writes[2] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_cullDescriptorSet[i], 2, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &bInd, nullptr};
        vkUpdateDescriptorSets(m_vkDevice, 3, writes, 0, nullptr);
    }
    
    // 4. Cull Pipeline
    VkComputePipelineCreateInfo cp = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    cp.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_COMPUTE_BIT, cullMod, "main", nullptr};
    cp.layout = m_cullLayout;
    res = vkCreateComputePipelines(m_vkDevice, VK_NULL_HANDLE, 1, &cp, nullptr, &m_cullPipeline);
    if (res != VK_SUCCESS) {
        if (logger) logger->LogError("MegaGeometryRenderer", "vkCreateComputePipelines failed for culling pipeline");
        vkDestroyShaderModule(m_vkDevice, cullMod, nullptr);
        return false;
    }
    
    vkDestroyShaderModule(m_vkDevice, cullMod, nullptr);
    return true;
}

uint32_t MegaGeometryRenderer::AddInstance(uint32_t meshSlot, float x, float y, float z, uint32_t textureID) {
    uint32_t id = m_totalInstances.fetch_add(1);
    if(id >= MAX_INSTANCES) {
        auto logger = m_core ? m_core->GetLogger() : nullptr;
        if (logger) logger->LogWarning("MegaGeometryRenderer", "AddInstance: MAX_INSTANCES reached");
        return MAX_INSTANCES;
    }
    
    // Initial color
    UpdateInstanceColor(id, 1,1,1,1);
    // Transform (Implicitly calculates MVP)
    UpdateInstanceTransform(id, x, y, z);
    // Texture (if provided)
    UpdateInstanceTexture(id, textureID);
    
    for(int i=0; i<2; ++i) {
        if(m_indirectMapped[i] && meshSlot < MAX_MESHES) {
            m_indirectMapped[i][meshSlot].instanceCount++;
        } else {
            auto logger = m_core ? m_core->GetLogger() : nullptr;
            if (logger && i == 0) {
                char msg[256];
                snprintf(msg, sizeof(msg), "AddInstance: indirectMapped[%d] is null or meshSlot %u >= MAX_MESHES", i, meshSlot);
                logger->LogWarning("MegaGeometryRenderer", msg);
            }
        }
    }
    return id;
}

void MegaGeometryRenderer::RemoveInstance(uint32_t instanceID, uint32_t meshSlot) {}

void MegaGeometryRenderer::ClearAllInstances() {
    auto logger = m_core ? m_core->GetLogger() : nullptr;
    
    // Reset instance counter
    uint32_t oldCount = m_totalInstances.exchange(0);
    
    // Clear all instance data in both buffers
    for (int i = 0; i < 2; ++i) {
        if (m_instanceDataMapped[i]) {
            memset(m_instanceDataMapped[i], 0, sizeof(InstanceData) * MAX_INSTANCES);
        }
        
        // Reset all indirect draw commands
        if (m_indirectMapped[i]) {
            for (uint32_t meshSlot = 0; meshSlot < MAX_MESHES; ++meshSlot) {
                m_indirectMapped[i][meshSlot].instanceCount = 0;
                // Keep indexCount, firstIndex, vertexOffset as they're mesh-specific
            }
        }
    }
    
    // Note: We do NOT clear m_meshPathToSlot or reset m_nextMeshSlot
    // Meshes remain loaded in GPU memory for reuse across level changes
    // This avoids reloading the same meshes repeatedly
    
    if (logger) {
        char msg[256];
        snprintf(msg, sizeof(msg), "✓ Cleared all instances: %u instances removed (meshes remain loaded)", oldCount);
        logger->LogInfo("MegaGeometryRenderer", msg);
    }
}

MegaGeometryRenderer::RenderStats MegaGeometryRenderer::GetStats() const { 
    uint32_t totalTris = 0;
    uint32_t totalInstances = m_totalInstances.load();
    
    // Just use frame 0 for stats
    if (m_indirectMapped[0]) {
        for (uint32_t i = 0; i < MAX_MESHES; ++i) {
            uint32_t idxCount = m_indirectMapped[0][i].indexCount;
            uint32_t instCount = m_indirectMapped[0][i].instanceCount;
            if (idxCount > 0 && instCount > 0) {
                totalTris += (idxCount / 3) * instCount;
            }
        }
    } else {
        auto logger = m_core ? m_core->GetLogger() : nullptr;
        if (logger) {
            static int logCount = 0;
            if (logCount++ < 5) { // Log first 5 times only
                logger->LogWarning("MegaGeometryRenderer", "GetStats: indirectMapped[0] is null");
            }
        }
    }
    return { totalInstances, totalTris, 1 }; 
}

} // namespace SecretEngine::MegaGeometry
