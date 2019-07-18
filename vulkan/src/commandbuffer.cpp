#include "commandbuffer.h"
#include "vulkanhelper.h"
#include "device.h"

#include <array>

CommandBuffer::CommandBuffer(const Device& device, VkCommandPool commandPool)
    : DeviceRef(device)
    , m_usedCommandPool(commandPool)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &m_commandBuffer));
}

CommandBuffer::~CommandBuffer()
{
    vkFreeCommandBuffers(device(), m_usedCommandPool, 1, &m_commandBuffer);
}

void CommandBuffer::begin()
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK_RESULT(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
}

void CommandBuffer::end()
{
    VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffer));
}

void CommandBuffer::beginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D renderAreaExtent, const VkClearColorValue *clearColor)
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = renderAreaExtent;
    if (clearColor)
    {
        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = *clearColor;
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
    }

    vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(renderAreaExtent.width), static_cast<float>(renderAreaExtent.height), 0.0f, 1.0f };
    vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = { { 0, 0 }, renderAreaExtent };
    vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
}

void CommandBuffer::endRenderPass()
{
    vkCmdEndRenderPass(m_commandBuffer);
}

void CommandBuffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkBufferCopy copyRegion = {};
    copyRegion.size = size;

    vkCmdCopyBuffer(m_commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void CommandBuffer::copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent2D resolution)
{
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { resolution.width, resolution.height, 1 };

    vkCmdCopyBufferToImage(m_commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void CommandBuffer::pipelineBarrier(VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage, VkImageMemoryBarrier barrier)
{
    vkCmdPipelineBarrier(m_commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void CommandBuffer::pipelineBarrier(VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage, VkBufferMemoryBarrier barrier)
{
    vkCmdPipelineBarrier(m_commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

void CommandBuffer::bindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    vkCmdBindPipeline(m_commandBuffer, pipelineBindPoint, pipeline);
}
