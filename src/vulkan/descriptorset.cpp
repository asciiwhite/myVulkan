#include "descriptorset.h"
#include "vulkanhelper.h"

void DescriptorSet::addSampler(VkImageView textureImageView, VkSampler sampler)
{
    const uint32_t bindingId = static_cast<uint32_t>(m_descriptorWrites.size());

    m_poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 1 });

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = bindingId;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    m_bindings.push_back(samplerLayoutBinding);

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView;
    imageInfo.sampler = sampler;
    m_imageInfos.push_back(imageInfo);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    descriptorWrite.dstSet = m_descriptorSet; // filled in finalize
    descriptorWrite.dstBinding = bindingId;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &m_imageInfos.back();
    m_descriptorWrites.push_back(descriptorWrite);
}

void DescriptorSet::addUniformBuffer(VkShaderStageFlags shaderStage, VkBuffer uniformBuffer)
{
    const uint32_t bindingId = static_cast<uint32_t>(m_descriptorWrites.size());

    m_poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1 });

    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = bindingId;
    layoutBinding.descriptorCount = 1;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.pImmutableSamplers = nullptr;
    layoutBinding.stageFlags = shaderStage;
    m_bindings.push_back(layoutBinding);

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;
    m_bufferInfos.push_back(bufferInfo);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //    descriptorWrite.dstSet = m_descriptorSet; // filled in finalize
    descriptorWrite.dstBinding = bindingId;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &m_bufferInfos.back();
    m_descriptorWrites.push_back(descriptorWrite);
}

void DescriptorSet::finalize(VkDevice device)
{
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());
    layoutInfo.pBindings = m_bindings.data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_layout));

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(m_poolSizes.size());
    poolInfo.pPoolSizes = m_poolSizes.data();
    poolInfo.maxSets = 1;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool));

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_layout;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &m_descriptorSet));

    for (auto& descriptorWrite : m_descriptorWrites)
    {
        descriptorWrite.dstSet = m_descriptorSet;
    }
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_descriptorWrites.size()), m_descriptorWrites.data(), 0, nullptr);

    m_imageInfos.clear();
    m_bufferInfos.clear();
    m_descriptorWrites.clear();
    m_bindings.clear();
}

void DescriptorSet::bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);
}

void DescriptorSet::destroy(VkDevice device)
{
    vkDestroyDescriptorSetLayout(device, m_layout, nullptr);
    m_layout = VK_NULL_HANDLE;

    // descriptor set is destroyed by pool destruction
    vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
    m_descriptorPool = VK_NULL_HANDLE;
}
