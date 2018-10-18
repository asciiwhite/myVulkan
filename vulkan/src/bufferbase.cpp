#include "bufferbase.h"
#include "device.h"

#include "vulkanhelper.h"

namespace
{
    std::pair<VkBuffer, VkDeviceMemory> createBufferAndMemory(const Device& device, uint64_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties)
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer;
        vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, memoryProperties);

        VkDeviceMemory memory;
        vkAllocateMemory(device, &allocInfo, nullptr, &memory);
        vkBindBufferMemory(device, buffer, memory, 0);

        return { buffer, memory };
    }
}

BufferBase::BufferBase(const Device& device, uint64_t size, VkBufferUsageFlagBits usageFlags, VkMemoryPropertyFlags memoryProperties)
    : DeviceRef(device)
    , m_size(size)
{
    std::tie(m_buffer, m_memory) = createBufferAndMemory(device, size, usageFlags, memoryProperties);
}

BufferBase::~BufferBase()
{
    destroy(m_buffer);
    destroy(m_memory);
}

bool BufferBase::isValid() const
{
    return m_buffer != VK_NULL_HANDLE;
}

uint64_t BufferBase::size() const
{
    return m_size;
}

VkBuffer BufferBase::buffer() const
{
    return m_buffer;
}

VkDeviceMemory BufferBase::memory() const
{
    return m_memory;
}

void BufferBase::fill(const FillFunc& fillfunc) const
{
    auto data = map(m_size, 0);
    fillfunc(data);
    unmap();
}

void* BufferBase::map(uint64_t size, uint64_t offset) const
{
    void* data;
    VK_CHECK_RESULT(vkMapMemory(device(), m_memory, offset, size, 0, &data));
    return data;
}

void BufferBase::unmap() const
{
    vkUnmapMemory(device(), m_memory);
}

void BufferBase::swap(BufferBase& other)
{
    DeviceRef::swap(other);
    std::swap(m_size, other.m_size);
    std::swap(m_buffer, other.m_buffer);
    std::swap(m_memory, other.m_memory);
}