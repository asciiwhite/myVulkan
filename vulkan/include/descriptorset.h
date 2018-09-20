#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <list>

class DescriptorSetLayout
{
public:
    struct BindingDesc
    {
        BindingDesc(uint32_t _bindingId, uint32_t _count, VkDescriptorType _descriptorType, VkShaderStageFlags _stageFlags)
            : bindingId(_bindingId), count(_count), descriptorType(_descriptorType), stageFlags(_stageFlags)
        {}

        uint32_t bindingId = 0;
        uint32_t count = 1;
        VkDescriptorType descriptorType;
        VkShaderStageFlags stageFlags;
    };

    void init(VkDevice device, std::initializer_list<BindingDesc> bindingDescs);
    void destroy(VkDevice device);

    operator VkDescriptorSetLayout() const { return m_layout; }

private:
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
};

//////////////////////////////////////////////////////////////////////////

class DescriptorPool
{
public:
    void init(VkDevice device, uint32_t count, const std::vector<VkDescriptorPoolSize>& sizes);
    void destroy(VkDevice device);

    operator VkDescriptorPool() const { return m_descriptorPool; }

private:
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;    
};

//////////////////////////////////////////////////////////////////////////

class DescriptorSet
{
public:
    void setImageSampler(uint32_t bindingId, VkImageView textureImageView, VkSampler sampler);
    void setSampler(uint32_t bindingId, VkSampler sampler);
    void setImageArray(uint32_t bindingId, const std::vector<VkImageView>& imageViews);
    void setUniformBuffer(uint32_t bindingId, VkBuffer uniformBuffer);
    void setStorageBuffer(uint32_t bindingId, VkBuffer storageBuffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    void allocate(VkDevice device, const DescriptorSetLayout& layout, const DescriptorPool& pool);
    void allocateAndUpdate(VkDevice device, const DescriptorSetLayout& layout, const DescriptorPool& pool);
    void update(VkDevice device);
    void free(VkDevice device, const DescriptorPool& pool);

    void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setId) const;
    static void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t firstSet, const std::vector<VkDescriptorSet>& descriptorSets);

    operator VkDescriptorSet() const { return m_descriptorSet; }

private:
    std::vector<VkWriteDescriptorSet> m_descriptorWrites;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    std::list<std::vector<VkDescriptorImageInfo>> m_imageInfos;
    std::vector<VkDescriptorBufferInfo> m_bufferInfos;
};
