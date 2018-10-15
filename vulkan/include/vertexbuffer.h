#pragma once

#include "buffer.h"
#include "deviceref.h"
#include "device.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <functional>

class VertexBuffer : public DeviceRef
{
public:
    struct AttributeDescription
    {
        AttributeDescription(uint32_t _location, uint32_t _componentCount, uint32_t _attributeCount, const float* _attributeData, uint32_t _interleavedOffset = 0, bool _perInstance = false)
            : location(_location)
            , componentCount(_componentCount)
            , attributeCount(_attributeCount)
            , interleavedOffset(_interleavedOffset)
            , attributeData(_attributeData)
            , perInstance(_perInstance)
        {}

        uint32_t location = 0;
        uint32_t componentCount = 0;
        uint32_t attributeCount = 0;
        uint32_t interleavedOffset = 0;
        const float* attributeData = nullptr;
        bool perInstance = false;
    };

    struct InterleavedAttributeDescription
    {
        InterleavedAttributeDescription(uint32_t _location, uint32_t _componentCount, uint32_t _interleavedOffset = 0)
            : location(_location)
            , componentCount(_componentCount)
            , interleavedOffset(_interleavedOffset)

        {}

        uint32_t location = 0;
        uint32_t componentCount = 0;
        uint32_t interleavedOffset = 0;
    };

    VertexBuffer(Device& device);
    ~VertexBuffer();

    void createFromSeparateAttributes(const std::vector<AttributeDescription>& descriptions);
    void createFromInterleavedAttributes(uint32_t vertexCount, uint32_t vertexSize, float* attributeData, const std::vector<InterleavedAttributeDescription>& descriptions);

    void setIndices(const uint16_t *indices, uint32_t numIndices);
    void setIndices(const uint32_t *indices, uint32_t numIndices);

    void bind(VkCommandBuffer commandBuffer) const;
    void draw(VkCommandBuffer commandBuffer, uint32_t instanceCount = 1) const;
    void drawIndexed(VkCommandBuffer commandBuffer, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t indexCount = 0) const;

    const std::vector<VkVertexInputBindingDescription>& getBindingDescriptions() const;
    const std::vector<VkVertexInputAttributeDescription>& getAttributeDescriptions() const;

    const uint32_t numVertices() const { return m_numVertices; }
    const uint32_t numIndices() const { return m_numIndices; }

    operator VkBuffer() const { return m_vertexBuffer; }

private:
    void createIndexBuffer(const void *indices, uint32_t numIndices, VkIndexType indexType);

    template<BufferUsage Usage, MemoryType Memory>
    void fillBuffer(Buffer<Usage, Memory>& buffer, const BufferBase::FillFunc& memcpyFunc)
    {
        static_assert(!Buffer<Usage, Memory>::isCpuVisible);
        // TODO: use single persistent staging buffer?
        StagingBuffer stagingBuffer(device(), buffer.size());
        stagingBuffer.fill(memcpyFunc);
        device().copyBuffer(stagingBuffer, buffer, buffer.size());
    }

    GPUAttributeBuffer m_vertexBuffer;
    GPUIndexBuffer m_indexBuffer;
    VkIndexType m_indexType = VK_INDEX_TYPE_UINT16;

    uint32_t m_numVertices = 0;
    uint32_t m_numIndices = 0;
    std::vector<VkVertexInputAttributeDescription> m_attributesDescriptions;
    std::vector<VkVertexInputBindingDescription> m_bindingDescriptions;
    std::vector<VkDeviceSize> m_bindingOffsets;
};
