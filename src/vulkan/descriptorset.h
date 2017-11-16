#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class DescriptorSet
{
public:
    void addSampler(VkImageView textureImageView, VkSampler sampler);
    void addUniformBuffer(VkShaderStageFlags shaderStage, VkBuffer uniformBuffer);

    void finalize(VkDevice device);

    void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const;

    VkDescriptorSetLayout getLayout() const { return m_layout; }

    void destroy(VkDevice device);

private:
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayoutBinding> m_bindings;

    std::vector<VkDescriptorPoolSize> m_poolSizes;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    std::vector<VkWriteDescriptorSet> m_descriptorWrites;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    std::vector<VkDescriptorImageInfo> m_imageInfos;
    std::vector<VkDescriptorBufferInfo> m_bufferInfos;
};