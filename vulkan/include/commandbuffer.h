#pragma once

#include "deviceref.h"
#include "vulkanhelper.h"

class CommandBuffer : public DeviceRef
{
    friend class Device;
public:
    ~CommandBuffer();

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    void begin();
    void end();

    void bindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent2D resolution);

    void pipelineBarrier(VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage, VkImageMemoryBarrier barrier);
    void pipelineBarrier(VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage, VkBufferMemoryBarrier barrier);

    void beginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D renderAreaExtent, const VkClearColorValue *clearColor = nullptr);
    void endRenderPass();

    operator VkCommandBuffer() { return m_commandBuffer; }

private:
    CommandBuffer(const Device& device, VkCommandPool commandPool);

    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    VkCommandPool m_usedCommandPool = VK_NULL_HANDLE;
};
