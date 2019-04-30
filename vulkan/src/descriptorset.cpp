#include "descriptorset.h"
#include "vulkanhelper.h"

void DescriptorSet::setImageSampler(uint32_t bindingId, VkImageView textureImageView, VkSampler sampler)
{
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView;
    imageInfo.sampler = sampler;
    m_imageInfos.push_back({ imageInfo });

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstBinding = bindingId;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    m_descriptorWrites.push_back(descriptorWrite);
}

void DescriptorSet::setSampler(uint32_t bindingId, VkSampler sampler)
{
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = nullptr;
    imageInfo.sampler = sampler;
    m_imageInfos.push_back({ imageInfo });

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstBinding = bindingId;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    m_descriptorWrites.push_back(descriptorWrite);
}

void DescriptorSet::setImageArray(uint32_t bindingId, const std::vector<VkImageView>& imageViews)
{
    std::vector<VkDescriptorImageInfo> imageInfos(imageViews.size());
    for (uint32_t i = 0u; i < imageViews.size(); i++)
    {
        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[i].imageView = imageViews[i];
        imageInfos[i].sampler = nullptr;
    }
    m_imageInfos.push_back(imageInfos);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstBinding = bindingId;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorWrite.descriptorCount = static_cast<uint32_t>(imageInfos.size());
    m_descriptorWrites.push_back(descriptorWrite);
}

void DescriptorSet::setUniformBuffer(uint32_t bindingId, VkBuffer uniformBuffer)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;
    m_bufferInfos.push_back(bufferInfo);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstBinding = bindingId;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    m_descriptorWrites.push_back(descriptorWrite);
}

void DescriptorSet::setStorageBuffer(uint32_t bindingId, VkBuffer storageBuffer, VkDeviceSize offset, VkDeviceSize size)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = storageBuffer;
    bufferInfo.offset = offset;
    bufferInfo.range = size;
    m_bufferInfos.push_back(bufferInfo);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstBinding = bindingId;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite.descriptorCount = 1;
    m_descriptorWrites.push_back(descriptorWrite);
}

void DescriptorSet::allocate(VkDevice device, VkDescriptorSetLayout layout, VkDescriptorPool pool)
{
    assert(m_descriptorSet == VK_NULL_HANDLE);

    const VkDescriptorSetLayout descriptorSetLayout{ layout };

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &m_descriptorSet));
}

void DescriptorSet::update(VkDevice device)
{
    assert(m_descriptorSet != VK_NULL_HANDLE);

    auto bufferIter = m_bufferInfos.begin();
    auto imageIter = m_imageInfos.begin();

    for (auto& descriptorWrite : m_descriptorWrites)
    {
        descriptorWrite.dstSet = m_descriptorSet;
        if (descriptorWrite.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
            descriptorWrite.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        {
            assert(bufferIter != m_bufferInfos.end());
            descriptorWrite.pBufferInfo = const_cast<const VkDescriptorBufferInfo*>(&(*bufferIter));
            bufferIter++;
        }
        else
        {
            assert(imageIter != m_imageInfos.end());
            descriptorWrite.pImageInfo = const_cast<const VkDescriptorImageInfo*>(&imageIter->front());
            imageIter++;
        }
    }
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_descriptorWrites.size()), m_descriptorWrites.data(), 0, nullptr);

    m_imageInfos.clear();
    m_bufferInfos.clear();
    m_descriptorWrites.clear();
    m_isValid = true;
}

void DescriptorSet::allocateAndUpdate(VkDevice device, VkDescriptorSetLayout layout, VkDescriptorPool pool)
{
    allocate(device, layout, pool);
    update(device);
}

void DescriptorSet::bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setId) const
{
    assert(m_descriptorSet != VK_NULL_HANDLE);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, setId, 1, &m_descriptorSet, 0, nullptr);
}

void DescriptorSet::bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t firstSet, const std::vector<VkDescriptorSet>& descriptorSets)
{
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, firstSet, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
}

void DescriptorSet::free(VkDevice device, VkDescriptorPool pool)
{
    assert(m_descriptorSet != VK_NULL_HANDLE);
    vkFreeDescriptorSets(device, pool, 1, &m_descriptorSet);
    m_descriptorSet = VK_NULL_HANDLE;
    m_isValid = false;
}

bool DescriptorSet::isValid() const
{
    return m_isValid;
}
