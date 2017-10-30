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
        default:
            assert(!"Unknown number of vertices");
            return VK_FORMAT_UNDEFINED;
        }
    }
}

void VertexBuffer::init(Device* device, const std::vector<AttributeDescription>& descriptions)
{
    if (descriptions.size() == 0)
        return;

    m_device = device;
    m_numVertices = descriptions[0].vertexCount;

    auto totalSize = 0;
    m_attributesDescriptions.resize(descriptions.size());
    m_bindingDescriptions.resize(descriptions.size());
    for (auto i = 0u; i < descriptions.size(); i++)
    {
        const auto& desc = descriptions[i];
        assert(m_numVertices == desc.vertexCount);

        VkVertexInputAttributeDescription& attribDesc = m_attributesDescriptions[i];
        attribDesc.binding = i;
        attribDesc.location = desc.location;
        attribDesc.format = getAttributeFormat(desc.componentCount);
        attribDesc.offset = totalSize;

        const auto attributeSize = desc.componentCount * 4;

        VkVertexInputBindingDescription& bindingDesc = m_bindingDescriptions[i];
        bindingDesc.binding = i;
        bindingDesc.stride = attributeSize;
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        totalSize += desc.vertexCount * attributeSize;
    }

    auto memcpyFunc = [&](void *mappedMemory)
    {
        auto data = reinterpret_cast<float*>(mappedMemory);
        for (auto& desc : descriptions)
        {
            const auto attributeSize = desc.componentCount * desc.vertexCount;
            std::memcpy(data, desc.vertexData, attributeSize * 4);
            data += attributeSize;
        }
    };

    createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, totalSize, m_vertexBuffer, m_vertexBufferMemory, memcpyFunc);
}

void VertexBuffer::createBuffer(VkBufferUsageFlags usage, uint32_t size, VkBuffer& buffer, VkDeviceMemory& bufferMemory, const MemcpyFunc& memcpyFunc)
{
    if (useStaging)
    {
        // TODO: use single persistent staging buffer?
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        m_device->createBuffer(size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        mapMemory(stagingBufferMemory, memcpyFunc);

        m_device->createBuffer(size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            buffer, bufferMemory);

        m_device->copyBuffer(stagingBuffer, buffer, size);

        vkDestroyBuffer(m_device->getVkDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_device->getVkDevice(), stagingBufferMemory, nullptr);
    }
    else
    {
        m_device->createBuffer(size,
            usage,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            buffer, bufferMemory);

        mapMemory(bufferMemory, memcpyFunc);
    }
}

void VertexBuffer::mapMemory(VkDeviceMemory bufferMemory, const MemcpyFunc& memcpyFunc)
{
    void* mappedMemory;
    VK_CHECK_RESULT(vkMapMemory(m_device->getVkDevice(), bufferMemory, 0, VK_WHOLE_SIZE, 0, &mappedMemory));
    memcpyFunc(mappedMemory);
    vkUnmapMemory(m_device->getVkDevice(), bufferMemory);
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

    createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size, m_indexBuffer, m_indexBufferMemory, memcpyFunc);
}

void VertexBuffer::draw(VkCommandBuffer commandBuffer) const
{
    const VkDeviceSize offset = 0;

    for (auto i = 0u; i < m_bindingDescriptions.size(); i++)
    {
        vkCmdBindVertexBuffers(commandBuffer, i, 1, &m_vertexBuffer, &offset);
    }

    if (m_indexBuffer != VK_NULL_HANDLE)
    {
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, m_indexType);
        vkCmdDrawIndexed(commandBuffer, m_numIndices, 1, 0, 0, 0);
    }
    else
    {
        vkCmdDraw(commandBuffer, m_numVertices, 1, 0, 0);
    }
}

const std::vector<VkVertexInputAttributeDescription>& VertexBuffer::getAttributeDescriptions() const
{
    return m_attributesDescriptions;
}

const std::vector<VkVertexInputBindingDescription>& VertexBuffer::getBindingDescriptions() const
{
    return m_bindingDescriptions;
}

void VertexBuffer::destroy()
{
    vkDestroyBuffer(m_device->getVkDevice(), m_vertexBuffer, nullptr);
    m_vertexBuffer = VK_NULL_HANDLE;
    vkFreeMemory(m_device->getVkDevice(), m_vertexBufferMemory, nullptr);
    m_vertexBufferMemory = VK_NULL_HANDLE;

    if (m_indexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device->getVkDevice(), m_indexBuffer, nullptr);
        m_indexBuffer = VK_NULL_HANDLE;
        vkFreeMemory(m_device->getVkDevice(), m_indexBufferMemory, nullptr);
        m_indexBufferMemory = VK_NULL_HANDLE;
    }
}
