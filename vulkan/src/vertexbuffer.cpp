#include "vertexbuffer.h"
#include "vulkanhelper.h"
#include "device.h"

#include <cstring>

const static bool useStaging = true;

namespace
{
    VkFormat getAttributeFormat(uint32_t numVertices)
    {
        switch (numVertices)
        {
        case 1: return VK_FORMAT_R32_SFLOAT;
        case 2: return VK_FORMAT_R32G32_SFLOAT;
        case 3: return VK_FORMAT_R32G32B32_SFLOAT;
        case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:
            assert(!"Unknown vertex attribute format");
            return VK_FORMAT_UNDEFINED;
        }
    }
}

VertexBuffer::VertexBuffer(Device& device)
    : DeviceRef(device)
{
}

VertexBuffer::~VertexBuffer()
{
    m_bindingDescriptions.clear();
    m_attributesDescriptions.clear();
    m_bindingOffsets.clear();
    m_numVertices = 0;
    m_numIndices = 0;
}

void VertexBuffer::createFromSeparateAttributes(const std::vector<AttributeDescription>& descriptions)
{
    if (descriptions.empty())
        return;

    m_numVertices = descriptions[0].attributeCount;

    auto totalSize = 0;
    m_attributesDescriptions.resize(descriptions.size());
    m_bindingDescriptions.resize(descriptions.size());
    m_bindingOffsets.resize(descriptions.size());
    for (auto i = 0u; i < descriptions.size(); i++)
    {
        const auto& desc = descriptions[i];
        assert(desc.perInstance || m_numVertices == desc.attributeCount);

        VkVertexInputAttributeDescription& attribDesc = m_attributesDescriptions[i];
        attribDesc.binding = i;
        attribDesc.location = desc.location;
        attribDesc.format = getAttributeFormat(desc.componentCount);
        attribDesc.offset = 0;

        const auto attributeSize = desc.componentCount * 4;

        VkVertexInputBindingDescription& bindingDesc = m_bindingDescriptions[i];
        bindingDesc.binding = i;
        bindingDesc.stride = attributeSize;
        bindingDesc.inputRate = desc.perInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;

        m_bindingOffsets[i] = totalSize;

        totalSize += desc.attributeCount * attributeSize;
    }

    auto memcpyFunc = [&](void *mappedMemory)
    {
        auto data = reinterpret_cast<float*>(mappedMemory);
        for (auto& desc : descriptions)
        {
            const auto attributeSize = desc.componentCount * desc.attributeCount;
            std::memcpy(data, desc.attributeData, attributeSize * 4);
            data += attributeSize;
        }
    };

    m_vertexBuffer = GPUAttributeStorageBuffer(device(), totalSize);
    fillBuffer(m_vertexBuffer, memcpyFunc);
}

void VertexBuffer::createFromInterleavedAttributes(uint32_t vertexCount, uint32_t vertexSize, float* attributeData, const std::vector<InterleavedAttributeDescription>& descriptions)
{
    if (vertexCount == 0 || descriptions.empty() || attributeData == nullptr)
        return;

     m_numVertices = vertexCount;

    const auto totalSize = vertexCount * vertexSize;
    m_attributesDescriptions.resize(descriptions.size());
    for (auto i = 0u; i < descriptions.size(); i++)
    {
        const auto& desc = descriptions[i];

        VkVertexInputAttributeDescription& attribDesc = m_attributesDescriptions[i];
        attribDesc.binding = 0;
        attribDesc.location = desc.location;
        attribDesc.format = getAttributeFormat(desc.componentCount);
        attribDesc.offset = desc.interleavedOffset;
    }

    m_bindingDescriptions.assign(1, { 0, vertexSize, VK_VERTEX_INPUT_RATE_VERTEX });
    m_bindingOffsets.assign(1, 0);
    
    auto memcpyFunc = [&](void *mappedMemory)
    {
        auto data = reinterpret_cast<float*>(mappedMemory);
        std::memcpy(data, attributeData, totalSize);
    };

    m_vertexBuffer = GPUAttributeStorageBuffer(device(), totalSize);
    fillBuffer(m_vertexBuffer, memcpyFunc);
}

void VertexBuffer::setIndices(const uint16_t *indices, uint32_t numIndices)
{
     createIndexBuffer(indices, numIndices, VK_INDEX_TYPE_UINT16);
}

void VertexBuffer::setIndices(const uint32_t *indices, uint32_t numIndices)
{
     createIndexBuffer(indices, numIndices, VK_INDEX_TYPE_UINT32);
}

void VertexBuffer::createIndexBuffer(const void *indices, uint32_t numIndices, VkIndexType indexType)
{
    m_numIndices = numIndices;
    m_indexType = indexType;

    const uint32_t size = numIndices * (indexType == VK_INDEX_TYPE_UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));

    auto memcpyFunc = [=](void *mappedMemory) {
        std::memcpy(mappedMemory, indices, size);
    };

    m_indexBuffer = GPUIndexBuffer(device(), size);
    fillBuffer(m_indexBuffer, memcpyFunc);
}

void VertexBuffer::bind(VkCommandBuffer commandBuffer) const
{
    for (auto i = 0u; i < m_bindingDescriptions.size(); i++)
    {
        VkBuffer buffer{ m_vertexBuffer };
        vkCmdBindVertexBuffers(commandBuffer, i, 1, &buffer, &m_bindingOffsets[i]);
    }

    if (m_indexBuffer.isValid())
    {
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, m_indexType);
    }
}

void VertexBuffer::drawIndexed(VkCommandBuffer commandBuffer, uint32_t instanceCount, uint32_t firstIndex, uint32_t indexCount) const
{
    assert(m_indexBuffer.isValid());
    vkCmdDrawIndexed(commandBuffer, indexCount == 0 ? m_numIndices : indexCount, instanceCount, firstIndex, 0, 0);
}

void VertexBuffer::draw(VkCommandBuffer commandBuffer, uint32_t instanceCount) const
{
    assert(!m_indexBuffer.isValid());
    vkCmdDraw(commandBuffer, m_numVertices, instanceCount, 0, 0);
}

const std::vector<VkVertexInputAttributeDescription>& VertexBuffer::getAttributeDescriptions() const
{
    return m_attributesDescriptions;
}

const std::vector<VkVertexInputBindingDescription>& VertexBuffer::getBindingDescriptions() const
{
    return m_bindingDescriptions;
}
