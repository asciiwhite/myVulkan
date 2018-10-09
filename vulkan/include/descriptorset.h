#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <list>

class DescriptorSet
{
public:
    void setImageSampler(uint32_t bindingId, VkImageView textureImageView, VkSampler sampler);
    void setSampler(uint32_t bindingId, VkSampler sampler);
    void setImageArray(uint32_t bindingId, const std::vector<VkImageView>& imageViews);
    void setUniformBuffer(uint32_t bindingId, VkBuffer uniformBuffer);
    void setStorageBuffer(uint32_t bindingId, VkBuffer storageBuffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    void allocate(VkDevice device, VkDescriptorSetLayout layout, VkDescriptorPool pool);
    void allocateAndUpdate(VkDevice device, VkDescriptorSetLayout layout, VkDescriptorPool pool);
    void update(VkDevice device);
    void free(VkDevice device, VkDescriptorPool pool);

    void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setId) const;
    static void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t firstSet, const std::vector<VkDescriptorSet>& descriptorSets);

    operator VkDescriptorSet() const { return m_descriptorSet; }

private:
    std::vector<VkWriteDescriptorSet> m_descriptorWrites;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    std::list<std::vector<VkDescriptorImageInfo>> m_imageInfos;
    std::vector<VkDescriptorBufferInfo> m_bufferInfos;
};
