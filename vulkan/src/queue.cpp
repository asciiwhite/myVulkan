#include "queue.h"
#include "commandbuffer.h"
#include "vulkanhelper.h"

Queue::Queue()
{
}

void Queue::submitAsync(CommandBuffer& commandBuffer, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence submitFence, VkPipelineStageFlags waitStages) const
{
    const VkCommandBuffer cmdBuf = commandBuffer;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = waitSemaphore ? 1 : 0;
    submitInfo.pWaitSemaphores = &waitSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;
    submitInfo.signalSemaphoreCount = signalSemaphore ? 1 : 0;
    submitInfo.pSignalSemaphores = &signalSemaphore;
    submitInfo.pWaitDstStageMask = waitStages != 0 ? &waitStages : nullptr;

    VK_CHECK_RESULT(vkQueueSubmit(m_queue, 1, &submitInfo, submitFence));
}

void Queue::submitBlocking(CommandBuffer& commandBuffer) const
{
    const VkCommandBuffer cmdBuf = commandBuffer;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    VK_CHECK_RESULT(vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK_RESULT(vkQueueWaitIdle(m_queue));
}
