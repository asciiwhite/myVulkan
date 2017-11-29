#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <list>

class DescriptorSet
{
public:
    void addSampler(uint32_t bindingId, VkImageView textureImageView, VkSampler sampler);
    void addUniformBuffer(uint32_t bindingId, VkShaderStageFlags shaderStage, VkBuffer uniformBuffer);

    void finalize(VkDevice device);

    void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setId) const;
    static void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t firstSet, const std::vector<VkDescriptorSet>& descriptorSets);

    VkDescriptorSetLayout getLayout() const { return m_layout; }
    VkDescriptorSet getDescriptorSet() const { return m_descriptorSet; }

    void destroy(VkDevice device);

private:
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayoutBinding> m_bindings;

    std::vector<VkDescriptorPoolSize> m_poolSizes;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    std::vector<VkWriteDescriptorSet> m_descriptorWrites;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    std::list<VkDescriptorImageInfo> m_imageInfos;
    std::list<VkDescriptorBufferInfo> m_bufferInfos;
};