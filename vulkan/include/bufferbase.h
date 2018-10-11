#pragma once

#include "deviceref.h"

#include <vulkan/vulkan.h>

struct NonCopyable
{
    constexpr NonCopyable() {}
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable& operator=(const NonCopyable &) = delete;
};

class Device;

struct BufferBase : public DeviceRef, NonCopyable
{
public:
    ~BufferBase();

    operator VkBuffer() const { return m_buffer; }
    bool isValid() const;

    uint64_t size() const;
    VkBuffer buffer() const;
    VkDeviceMemory memory() const;

    void* map(uint64_t size = VK_WHOLE_SIZE, uint64_t offset = 0) const;
    void unmap() const;

protected:
    BufferBase() = default;
    BufferBase(const Device&, uint64_t size, VkBufferUsageFlagBits usageFlags, VkMemoryPropertyFlags memoryProperties);

    void swap(BufferBase& other);

private:
    uint64_t m_size = 0;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
};
