#pragma once

#include "deviceref.h"
#include "vulkanhelper.h"
#include "device.h"

#include <memory>

enum class SubmissionQueue
{
    Graphics,
    Compute
};

class CommandBuffer : public DeviceRef
{
public:
    CommandBuffer(const Device& device, VkCommandPool commandPool);
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

    template<SubmissionQueue Queue>
    void submitAsync(const VkSemaphore waitSemaphore = VK_NULL_HANDLE, const VkSemaphore signalSemaphore = VK_NULL_HANDLE, VkFence submitFence = VK_NULL_HANDLE)
    {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.waitSemaphoreCount = waitSemaphore ? 1 : 0;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffer;
        submitInfo.signalSemaphoreCount = signalSemaphore ? 1 : 0;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        if constexpr (Queue == SubmissionQueue::Graphics)
        {
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.pWaitDstStageMask = waitStages;
        }

        submit<Queue>(submitInfo, submitFence);
    }

    template<SubmissionQueue Queue>
    void submitBlocking()
    {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffer;

        submit<Queue>(submitInfo);

        if constexpr (Queue == SubmissionQueue::Graphics)
            VK_CHECK_RESULT(vkQueueWaitIdle(device().getGraphicsQueue()))
        else
            VK_CHECK_RESULT(vkQueueWaitIdle(device().getComputeQueue()))
    }

    operator VkCommandBuffer() { return m_commandBuffer; }

private:
    template<SubmissionQueue Queue>
    void submit(const VkSubmitInfo& submitInfo, VkFence submitFence = VK_NULL_HANDLE)
    {
        if constexpr (Queue == SubmissionQueue::Graphics)
            submit(device().getGraphicsQueue(), submitInfo, submitFence);
        else
            submit(device().getComputeQueue(), submitInfo, submitFence);
    }

    void submit(VkQueue queue, const VkSubmitInfo& submitInfo, VkFence submitFence);

    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    VkCommandPool m_usedCommandPool = VK_NULL_HANDLE;
};

using CommandBufferPtr = std::unique_ptr<CommandBuffer>;
