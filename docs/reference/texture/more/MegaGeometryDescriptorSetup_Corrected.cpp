// ============================================================================
// DESCRIPTOR SET CREATION - CORRECTED FOR BINDLESS TEXTURES
// This replaces the CreateDescriptorSet() function in MegaGeometryRenderer.cpp
// ============================================================================

bool MegaGeometryRenderer::CreateDescriptorSet() {
    // ============================================================================
    // BINDING 0: Instance SSBO (Standard Storage Buffer)
    // ============================================================================
    VkDescriptorSetLayoutBinding ssboBind = {};
    ssboBind.binding = 0;
    ssboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboBind.descriptorCount = 1;
    ssboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    ssboBind.pImmutableSamplers = nullptr;
    
    // ============================================================================
    // BINDING 1: Bindless Texture Array (1024 or 16384 textures)
    // ============================================================================
    VkDescriptorSetLayoutBinding texBind = {};
    texBind.binding = 1;
    texBind.descriptorCount = MAX_TEXTURES;  // 1024 recommended, 16384 maximum
    texBind.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texBind.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    texBind.pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutBinding bindings[] = { ssboBind, texBind };
    
    // ============================================================================
    // CRITICAL: Binding Flags for Bindless Operation
    // ============================================================================
    VkDescriptorBindingFlags bindFlags[] = {
        // Binding 0 (SSBO): Standard, no special flags
        0,
        
        // Binding 1 (Textures): Enable bindless features
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |           // Allow unused slots
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |         // Update while in use
        VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT // Update unused slots anytime
    };
    
    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO
    };
    flagsInfo.bindingCount = 2;
    flagsInfo.pBindingFlags = bindFlags;
    
    // ============================================================================
    // DESCRIPTOR SET LAYOUT CREATION
    // ============================================================================
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
    };
    layoutInfo.pNext = &flagsInfo;  // Chain the binding flags
    
    // CRITICAL: Allow updates after binding (required for bindless)
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;
    
    if (vkCreateDescriptorSetLayout(m_vkDevice, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        m_core->GetLogger()->LogError("MegaGeometry", "Failed to create descriptor set layout");
        return false;
    }
    
    // ============================================================================
    // DESCRIPTOR POOL CREATION
    // ============================================================================
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2},  // 2 for double buffering
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_TEXTURES * 2}  // Double buffered
    };
    
    VkDescriptorPoolCreateInfo poolInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO
    };
    
    // CRITICAL: Allow updates after binding
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
    
    poolInfo.maxSets = 2;  // Double buffered
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    
    VkDescriptorPool pool;
    if (vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        m_core->GetLogger()->LogError("MegaGeometry", "Failed to create descriptor pool");
        return false;
    }
    
    // ============================================================================
    // ALLOCATE DESCRIPTOR SETS (Double Buffered)
    // ============================================================================
    for (int i = 0; i < 2; ++i) {
        VkDescriptorSetAllocateInfo allocInfo = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO
        };
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_descriptorSetLayout;
        
        if (vkAllocateDescriptorSets(m_vkDevice, &allocInfo, &m_descriptorSet[i]) != VK_SUCCESS) {
            m_core->GetLogger()->LogError("MegaGeometry", "Failed to allocate descriptor sets");
            return false;
        }
        
        // ============================================================================
        // WRITE SSBO BINDING (Binding 0)
        // ============================================================================
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_instanceSSBO[i];
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;
        
        VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = m_descriptorSet[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;
        
        vkUpdateDescriptorSets(m_vkDevice, 1, &write, 0, nullptr);
        
        // Note: Texture bindings (Binding 1) are updated dynamically in LoadTexture()
    }
    
    m_core->GetLogger()->LogInfo("MegaGeometry", "✓ Bindless descriptor sets created");
    return true;
}

// ============================================================================
// TEXTURE LOADING - RUNTIME DESCRIPTOR UPDATE
// ============================================================================

int32_t MegaGeometryRenderer::LoadTexture(const char* path) {
    // 1. Check cache
    if (m_texturePathToID.count(path)) {
        return m_texturePathToID[path];
    }
    
    // 2. Safety limit
    if (m_loadedTextures.size() >= MAX_TEXTURES) {
        m_core->GetLogger()->LogError("MegaGeometry", "Texture limit reached!");
        return 0;  // Return slot 0 (default white texture)
    }
    
    // 3. Load texture using TextureManager or direct Texture class
    Texture* tex = new Texture(m_device);
    
    // Need command pool for upload - create one in Initialize() if needed
    if (!tex->Load(path, m_transferCommandPool, m_device->GetGraphicsQueue())) {
        m_core->GetLogger()->LogError("MegaGeometry", 
            (std::string("Failed to load: ") + path).c_str());
        delete tex;
        return -1;
    }
    
    // 4. Assign ID
    uint32_t id = (uint32_t)m_loadedTextures.size();
    m_loadedTextures.push_back(tex);
    m_texturePathToID[path] = id;
    
    // 5. Update GPU descriptor immediately
    UpdateTextureDescriptor(id, tex);
    
    m_core->GetLogger()->LogInfo("MegaGeometry", 
        (std::string("✓ Loaded texture [") + std::to_string(id) + "]: " + path).c_str());
    
    return id;
}

void MegaGeometryRenderer::UpdateTextureDescriptor(uint32_t slot, Texture* tex) {
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = tex->GetImageView();
    imageInfo.sampler = tex->GetSampler();
    
    VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = 1;              // Texture array binding
    write.dstArrayElement = slot;      // Which texture slot (0, 1, 2...)
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;
    
    // Update BOTH double-buffered descriptor sets
    write.dstSet = m_descriptorSet[0];
    vkUpdateDescriptorSets(m_vkDevice, 1, &write, 0, nullptr);
    
    write.dstSet = m_descriptorSet[1];
    vkUpdateDescriptorSets(m_vkDevice, 1, &write, 0, nullptr);
}

void MegaGeometryRenderer::SetInstanceTexture(uint32_t instanceID, int32_t textureID) {
    if (instanceID >= MAX_INSTANCES) return;
    
    // Update both CPU-mapped buffers (persistent mapping)
    for (int i = 0; i < 2; ++i) {
        if (m_instanceDataMapped[i]) {
            m_instanceDataMapped[i][instanceID].textureID = (uint32_t)textureID;
        }
    }
}
