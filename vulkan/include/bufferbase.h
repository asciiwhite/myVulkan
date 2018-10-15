#pragma once

#include "deviceref.h"
#include "noncopyable.h"

#include <functional>
#include <assert.h>
#include <cstring>
#include <vulkan/vulkan.h>

class Device;

class BufferBase : public DeviceRef, NonCopyable
{
public:
    ~BufferBase();

    operator VkBuffer() const { return m_buffer; }
    bool isValid() const;

    uint64_t size() const;
    VkBuffer buffer() const;
    VkDeviceMemory memory() const;  

    using FillFunc = std::function<void(void*)>;

protected:
    BufferBase() = default;
    BufferBase(const Device&, uint64_t size, VkBufferUsageFlagBits usageFlags, VkMemoryPropertyFlags memoryProperties);

    void swap(BufferBase& other);

    void* map(uint64_t size = VK_WHOLE_SIZE, uint64_t offset = 0) const;
    void unmap() const;

    template<typename T>
    void assign(T* inputData, uint64_t size) const
    {
        assert(size <= m_size);
        const auto dstData = map(m_size, 0);
        std::memcpy(dstData, inputData, size);
        unmap();
    }

    void fill(const FillFunc& fillFunc) const;

private:
    uint64_t m_size = 0;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
};
