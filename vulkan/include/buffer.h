#pragma once

#include "bufferbase.h"

#include <assert.h>

enum class BufferUsage
{
    AttributeBit = int(VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
    IndexBit = int(VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
    UniformBit = int(VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
    StorageBit = int(VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
    TransferSrc = int(VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
    TransferDst = int(VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT)
};

constexpr BufferUsage operator|(BufferUsage a, BufferUsage b)
{
    return BufferUsage(uint32_t(a) | uint32_t(b));
}

enum class MemoryType
{
    DeviceLocal = int(VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    CpuVisible = int(VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
};

constexpr MemoryType operator|(MemoryType a, MemoryType b)
{
    return MemoryType(uint32_t(a) | uint32_t(b));
}

template<BufferUsage Usage, MemoryType Memory>
class Buffer : public BufferBase
{
protected:
    template<typename T>
    static constexpr bool has(T a, T b)
    {
        return (uint32_t(a) & uint32_t(b)) == uint32_t(b);
    }

    static constexpr bool is_compatible(BufferUsage U, MemoryType M)
    {
        return has(U, Usage) && has(M, Memory);
    }

public:
    static constexpr bool isCpuVisible = has(Memory, MemoryType::CpuVisible);

    Buffer() = default;

    Buffer(const Device& device, uint64_t size)
        : BufferBase(device, size, VkBufferUsageFlagBits(Usage), VkMemoryPropertyFlags(Memory))
    {
    }

    Buffer(const Device& device, uint64_t size, void* data)
        : BufferBase(device, size, VkBufferUsageFlagBits(Usage), VkMemoryPropertyFlags(Memory))
    {
        assign(data, size);
    }

    template<typename T>
    Buffer(const Device& device, T* data)
        : BufferBase(device, sizeof(data), VkBufferUsageFlagBits(Usage), VkMemoryPropertyFlags(Memory))
    {
        assign(data, size());
    }

    template<BufferUsage U, MemoryType M>
    Buffer(Buffer<U, M>&& other)
    {
        static_assert(is_compatible(U, M));
        swap(other);
    }

    template<BufferUsage U, MemoryType M>
    Buffer& operator=(Buffer<U,M>&& other)
    {
        static_assert(is_compatible(U,M));
        swap(other);
        return *this;
    }

    void* map(uint64_t size = VK_WHOLE_SIZE, uint64_t offset = 0) const
    {
        static_assert(isCpuVisible);
        return BufferBase::map(size, offset);
    }

    void unmap() const
    {
        static_assert(isCpuVisible);
        BufferBase::unmap();
    }

    template<typename T>
    void assign(T* inputData, uint64_t size) const
    {
        static_assert(isCpuVisible);
        BufferBase::assign(inputData, size);
    }

    void fill(const FillFunc& fillFunc) const
    {
        static_assert(isCpuVisible);
        BufferBase::fill(fillFunc);
    }
};

using GPUAttributeStorageBuffer = Buffer<BufferUsage::AttributeBit | BufferUsage::StorageBit | BufferUsage::TransferDst, MemoryType::DeviceLocal>;
using GPUAttributeBuffer = Buffer<BufferUsage::AttributeBit | BufferUsage::TransferDst, MemoryType::DeviceLocal>;
using GPUIndexBuffer = Buffer<BufferUsage::IndexBit | BufferUsage::TransferDst, MemoryType::DeviceLocal>;

using CPUAttributeBuffer = Buffer<BufferUsage::AttributeBit, MemoryType::CpuVisible>;
using CPUIndexBuffer = Buffer<BufferUsage::IndexBit, MemoryType::CpuVisible>;

using StagingBuffer = Buffer<BufferUsage::TransferSrc, MemoryType::CpuVisible>;
using UniformBuffer = Buffer<BufferUsage::UniformBit, MemoryType::CpuVisible>;
