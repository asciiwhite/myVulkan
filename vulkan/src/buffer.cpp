#include "buffer.h"
#include "vulkanhelper.h"
#include "device.h"

void Buffer::create(Device& device, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    device.createBuffer(size, usage, properties, m_buffer, m_memory);
}

void Buffer::destroy(Device& device)
{
    vkDestroyBuffer(device.getVkDevice(), m_buffer, nullptr);
    m_buffer = VK_NULL_HANDLE;

    vkFreeMemory(device.getVkDevice(), m_memory, nullptr);
    m_memory = VK_NULL_HANDLE;
}

void* Buffer::map(Device& device, uint64_t size, uint64_t offset)
{
    void* data;
    VK_CHECK_RESULT(vkMapMemory(device.getVkDevice(), m_memory, offset, size, 0, &data));
    return data;
}

void Buffer::unmap(Device& device)
{
    vkUnmapMemory(device.getVkDevice(), m_memory);
}
