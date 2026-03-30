// SecretEngine
// Module: VulkanRenderer
// Responsibility: 3D Graphics Pipeline (Multi-Mesh, Instanced)
// Dependencies: VulkanDevice, ICore, Math, IAssetProvider

#include "Pipeline3D.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/ILogger.h>
#include <SecretEngine/Math.h>
#include <SecretEngine/IAssetProvider.h>
#include "VulkanHelpers.h"
#include <cstring>
#include <vector>
#include <map>

using namespace SecretEngine::Math;
using SecretEngine::Vulkan::Helpers;

// Instance data structure is now in SecretEngine/Math.h
using SecretEngine::Math::InstanceData;

Pipeline3D::Pipeline3D() : m_device(nullptr), m_core(nullptr), m_renderPass(VK_NULL_HANDLE), m_pipeline(VK_NULL_HANDLE),
    m_pipelineLayout(VK_NULL_HANDLE), m_instanceBuffer(VK_NULL_HANDLE),
    m_instanceMemory(VK_NULL_HANDLE), m_currentColorIndex(0) 
{
}

Pipeline3D::~Pipeline3D() { Cleanup(); }

bool Pipeline3D::Initialize(VulkanDevice* device, VkRenderPass renderPass, SecretEngine::ICore* core) {
    m_device = device; m_renderPass = renderPass; m_core = core;
    
    // Create Instance Buffer
    VkDeviceSize instanceBufferSize = sizeof(InstanceData) * MAX_INSTANCES;
    Helpers::CreateBuffer(m_device->GetDevice(), m_device->GetPhysicalDevice(), instanceBufferSize,
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         m_instanceBuffer, m_instanceMemory);

    if (!CreatePipeline()) return false;
    m_core->GetLogger()->LogInfo("Pipeline3D", "✓ 3D Multi-Mesh Pipeline initialized");
    return true;
}

bool Pipeline3D::LoadMesh(const char* filename) {
    if (m_meshes.count(filename)) return true; // Already loaded

    auto buffer = m_core->GetAssetProvider()->LoadBinary(filename);
    if (buffer.size() < sizeof(MeshHeader)) return false;
    
    MeshHeader* header = reinterpret_cast<MeshHeader*>(buffer.data());
    
    MeshData mesh = {};
    mesh.vertexCount = header->vertexCount;
    mesh.indexCount = header->indexCount;
    Vertex3D* vertices = reinterpret_cast<Vertex3D*>(buffer.data() + sizeof(MeshHeader));
    uint32_t* indices = reinterpret_cast<uint32_t*>(buffer.data() + sizeof(MeshHeader) + mesh.vertexCount * sizeof(Vertex3D));

    VkDeviceSize vertexBufferSize = mesh.vertexCount * sizeof(Vertex3D);
    Helpers::CreateBuffer(m_device->GetDevice(), m_device->GetPhysicalDevice(), vertexBufferSize,
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         mesh.vertexBuffer, mesh.vertexMemory);
    Helpers::MapAndCopy(m_device->GetDevice(), mesh.vertexMemory, vertexBufferSize, vertices);

    VkDeviceSize indexBufferSize = mesh.indexCount * sizeof(uint32_t);
    Helpers::CreateBuffer(m_device->GetDevice(), m_device->GetPhysicalDevice(), indexBufferSize,
                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         mesh.indexBuffer, mesh.indexMemory);
    Helpers::MapAndCopy(m_device->GetDevice(), mesh.indexMemory, indexBufferSize, indices);

    m_meshes[filename] = mesh;
    m_core->GetLogger()->LogInfo("Pipeline3D", (std::string("Loaded mesh resource: ") + filename).c_str());
    return true;
}

bool Pipeline3D::CreatePipeline() {
    auto vertCode = m_core->GetAssetProvider()->LoadBinary("shaders/basic3d_vert.spv");
    auto fragCode = m_core->GetAssetProvider()->LoadBinary("shaders/basic3d_frag.spv");
    
    if (vertCode.empty() || fragCode.empty()) return false;
    
    VkShaderModule vertMod = Helpers::CreateShaderModule(m_device->GetDevice(), vertCode);
    VkShaderModule fragMod = Helpers::CreateShaderModule(m_device->GetDevice(), fragCode);

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT; stages[0].module = vertMod; stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module = fragMod; stages[1].pName = "main";

    VkVertexInputBindingDescription bindings[2] = {
        {0, sizeof(Vertex3D), VK_VERTEX_INPUT_RATE_VERTEX},
        {1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE}
    };
    VkVertexInputAttributeDescription attrs[7] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}, // Pos
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12}, // Norm
        {2, 0, VK_FORMAT_R32G32_SFLOAT, 24},    // UV
        {3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0},  // Matrix R0
        {4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 16}, // Matrix R1
        {5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 32}, // Matrix R2
        {6, 1, VK_FORMAT_R32_UINT, 48}             // Packed Color (8-bit Nitrogen)
    };

    VkPipelineVertexInputStateCreateInfo vInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vInput.vertexBindingDescriptionCount = 2; vInput.pVertexBindingDescriptions = bindings;
    vInput.vertexAttributeDescriptionCount = 7; vInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAsm = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAsm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vport = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    vport.viewportCount = 1; vport.pViewports = nullptr; // Dynamic
    vport.scissorCount = 1; vport.pScissors = nullptr; // Dynamic

    VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynStates;

    VkPipelineRasterizationStateCreateInfo raster = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    raster.lineWidth = 1.0f; raster.cullMode = VK_CULL_MODE_NONE; raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo ms = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo ds = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    ds.depthTestEnable = VK_FALSE; ds.depthWriteEnable = VK_FALSE; ds.depthCompareOp = VK_COMPARE_OP_ALWAYS;

    VkPipelineColorBlendAttachmentState cbAs = {}; cbAs.colorWriteMask = 0xF;
    VkPipelineColorBlendStateCreateInfo cb = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    cb.attachmentCount = 1; cb.pAttachments = &cbAs;

    VkPushConstantRange push = {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16};
    VkPipelineLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layoutInfo.pushConstantRangeCount = 1; layoutInfo.pPushConstantRanges = &push;
    vkCreatePipelineLayout(m_device->GetDevice(), &layoutInfo, nullptr, &m_pipelineLayout);

    VkGraphicsPipelineCreateInfo pipe = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipe.stageCount = 2; pipe.pStages = stages; pipe.pVertexInputState = &vInput; pipe.pInputAssemblyState = &inputAsm;
    pipe.pViewportState = &vport; pipe.pRasterizationState = &raster; pipe.pMultisampleState = &ms; pipe.pColorBlendState = &cb; pipe.pDepthStencilState = &ds;
    pipe.pDynamicState = &dynamicState;
    pipe.layout = m_pipelineLayout; pipe.renderPass = m_renderPass;
    vkCreateGraphicsPipelines(m_device->GetDevice(), VK_NULL_HANDLE, 1, &pipe, nullptr, &m_pipeline);

    vkDestroyShaderModule(m_device->GetDevice(), vertMod, nullptr); vkDestroyShaderModule(m_device->GetDevice(), fragMod, nullptr);
    return true;
}

#include <SecretEngine/IWorld.h>
#include <SecretEngine/Components.h>

void Pipeline3D::Render(VkCommandBuffer cmd, float rotation, float aspect, const float camPos[3], const float camRot[3], const float* viewProj) {
    if (m_pipeline == VK_NULL_HANDLE || m_meshes.empty()) return;
    
    Matrix4x4 vp;
    if (viewProj) {
        memcpy(vp.m, viewProj, 16 * sizeof(float));
    } else {
        Matrix4x4 proj = Matrix4x4::Perspective(0.785f, aspect, 0.1f, 100.0f);
        Matrix4x4 view = Matrix4x4::RotationX(-camRot[0]) * 
                         Matrix4x4::RotationY(-camRot[1]) * 
                         Matrix4x4::RotationZ(-camRot[2]) * 
                         Matrix4x4::Translation(-camPos[0], -camPos[1], -camPos[2]);

        vp = proj * view;
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float)*16, vp.m);

    struct DrawCall {
        std::string meshPath;
        uint32_t firstInstance;
        uint32_t instanceCount;
    };
    std::vector<DrawCall> drawCalls;
    std::vector<InstanceData> allInstanceData;

    if (m_core->GetWorld()) {
        auto world = m_core->GetWorld();
        const auto& entities = world->GetAllEntities();
        
        // Group by mesh
        std::map<std::string, std::vector<size_t>> meshToEntities;
        for (size_t i = 0; i < entities.size(); ++i) {
            auto meshComp = static_cast<SecretEngine::MeshComponent*>(world->GetComponent(entities[i], SecretEngine::MeshComponent::TypeID));
            if (meshComp) meshToEntities[meshComp->meshPath].push_back(i);
        }

        for (auto const& [path, indices] : meshToEntities) {
            if (!m_meshes.count(path)) continue;
            
            DrawCall dc = { path, (uint32_t)allInstanceData.size(), 0 };
            for (size_t idx : indices) {
                if (allInstanceData.size() >= MAX_INSTANCES) break;
                auto e = entities[idx];
                auto transform = static_cast<SecretEngine::TransformComponent*>(world->GetComponent(e, SecretEngine::TransformComponent::TypeID));
                auto meshComp = static_cast<SecretEngine::MeshComponent*>(world->GetComponent(e, SecretEngine::MeshComponent::TypeID));
                
                InstanceData inst = {};
                Matrix4x4::FromTRS(inst.transform, transform->position[0], transform->position[1], transform->position[2], 0.0f, transform->rotation[1], 0.0f, transform->scale[0]);
                
                // Pack 8-bit color
                uint32_t r = (uint32_t)(meshComp->color[0] * 255.0f) & 0xFF;
                uint32_t g = (uint32_t)(meshComp->color[1] * 255.0f) & 0xFF;
                uint32_t b = (uint32_t)(meshComp->color[2] * 255.0f) & 0xFF;
                inst.packedColor = r | (g << 8) | (b << 16) | (0xFF << 24);
                
                allInstanceData.push_back(inst);
                dc.instanceCount++;
            }
            if (dc.instanceCount > 0) drawCalls.push_back(dc);
        }
    }

    if (allInstanceData.empty()) return;

    // Map once for the whole frame
    void* data;
    vkMapMemory(m_device->GetDevice(), m_instanceMemory, 0, allInstanceData.size() * sizeof(InstanceData), 0, &data);
    memcpy(data, allInstanceData.data(), allInstanceData.size() * sizeof(InstanceData));
    vkUnmapMemory(m_device->GetDevice(), m_instanceMemory);

    for (const auto& dc : drawCalls) {
        const auto& mesh = m_meshes[dc.meshPath];
        VkBuffer vbs[] = {mesh.vertexBuffer, m_instanceBuffer};
        VkDeviceSize offsets[] = {0, dc.firstInstance * sizeof(InstanceData)};
        vkCmdBindVertexBuffers(cmd, 0, 2, vbs, offsets);
        vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, mesh.indexCount, dc.instanceCount, 0, 0, 0);
    }
}

void Pipeline3D::SetCubeColor(int idx) { m_currentColorIndex = idx % 6; }
void Pipeline3D::UpdateEntityPosition(int i, float x, float y) {}

void Pipeline3D::Cleanup() {
    if (m_device) {
        if (m_pipeline) vkDestroyPipeline(m_device->GetDevice(), m_pipeline, nullptr);
        if (m_pipelineLayout) vkDestroyPipelineLayout(m_device->GetDevice(), m_pipelineLayout, nullptr);
        for (auto& pair : m_meshes) {
            vkDestroyBuffer(m_device->GetDevice(), pair.second.vertexBuffer, nullptr);
            vkFreeMemory(m_device->GetDevice(), pair.second.vertexMemory, nullptr);
            vkDestroyBuffer(m_device->GetDevice(), pair.second.indexBuffer, nullptr);
            vkFreeMemory(m_device->GetDevice(), pair.second.indexMemory, nullptr);
        }
        if (m_instanceBuffer) { vkDestroyBuffer(m_device->GetDevice(), m_instanceBuffer, nullptr); vkFreeMemory(m_device->GetDevice(), m_instanceMemory, nullptr); }
    }
}
