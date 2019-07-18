#pragma once

#include <vulkan/vulkan.h>

class CommandBuffer;

class Queue
{
    friend class Device;
public:
    void submitAsync(
        CommandBuffer& commandBuffer,
        VkSemaphore waitSemaphore = VK_NULL_HANDLE,
        VkSemaphore signalSemaphore = VK_NULL_HANDLE,
        VkFence submitFence = VK_NULL_HANDLE,
        VkPipelineStageFlags waitStages = 0) const;

    void submitBlocking(CommandBuffer& commandBuffer) const;

    uint32_t familyId() const { return m_queueFamilyIndex; }
    operator VkQueue() const { return m_queue; }

private:
    Queue();

    VkQueue m_queue = VK_NULL_HANDLE;
    uint32_t m_queueFamilyIndex = UINT32_MAX;
};
