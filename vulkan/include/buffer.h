#pragma once

#include <vulkan/vulkan.h>

class Device;

class Buffer
{
public:
    void create(Device& device, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void destroy(Device& device);

    void* map(Device& device, uint64_t size = VK_WHOLE_SIZE, uint64_t offset = 0);
    void unmap(Device& device);

    VkBuffer getVkBuffer() const
    {
        return m_buffer;
    }

private:
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
};
