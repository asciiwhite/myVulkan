#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <list>

class DescriptorSetLayout
{
public:
    struct BindingDesc
    {
        BindingDesc(uint32_t _bindingId, VkDescriptorType _descriptorType, VkShaderStageFlags _stageFlags)
            : bindingId(_bindingId), descriptorType(_descriptorType), stageFlags(_stageFlags)
        {}

        uint32_t bindingId = 0;
        VkDescriptorType descriptorType;
        VkShaderStageFlags stageFlags;
    };

    void init(VkDevice device, std::initializer_list<BindingDesc> bindingDescs);
    void destroy(VkDevice device);

    const VkDescriptorSetLayout& getVkLayout() const { return m_layout; }

private:
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
};

//////////////////////////////////////////////////////////////////////////

class DescriptorPool
{
public:
    void init(VkDevice device, uint32_t count, const std::vector<VkDescriptorPoolSize>& sizes);
    void destroy(VkDevice device);

    VkDescriptorPool getVkPool() const { return m_descriptorPool; }

private:
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;    
};

//////////////////////////////////////////////////////////////////////////

class DescriptorSet
{
public:
    void addSampler(uint32_t bindingId, VkImageView textureImageView, VkSampler sampler);
    void addUniformBuffer(uint32_t bindingId, VkBuffer uniformBuffer);

    void finalize(VkDevice device, const DescriptorSetLayout& layout, const DescriptorPool& pool);

    void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setId) const;
    static void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t firstSet, const std::vector<VkDescriptorSet>& descriptorSets);

    VkDescriptorSet getVkDescriptorSet() const { return m_descriptorSet; }

private:
    std::vector<VkWriteDescriptorSet> m_descriptorWrites;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    std::list<VkDescriptorImageInfo> m_imageInfos;
    std::list<VkDescriptorBufferInfo> m_bufferInfos;
};
