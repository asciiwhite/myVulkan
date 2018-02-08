#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <functional>

class Device;


class VertexBuffer
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

    void createFromSeparateAttributes(Device* device, const std::vector<AttributeDescription>& descriptions);
    void createFromInterleavedAttributes(Device* device, const std::vector<AttributeDescription>& descriptions);
    void destroy();

    void setIndices(const uint16_t *indices, uint32_t numIndices);
    void setIndices(const uint32_t *indices, uint32_t numIndices);

    void draw(VkCommandBuffer commandBuffer, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t indexCount = 0) const;

    const std::vector<VkVertexInputBindingDescription>& getBindingDescriptions() const;
    const std::vector<VkVertexInputAttributeDescription>& getAttributeDescriptions() const;

private:
    using MemcpyFunc = std::function<void(void*)>;
    void createBuffer(VkBufferUsageFlags usage, uint32_t size, VkBuffer& buffer, VkDeviceMemory& bufferMemory, const MemcpyFunc& memcpyFunc);
    void createIndexBuffer(const void *indices, uint32_t numIndices, VkIndexType indexType);
    void mapMemory(VkDeviceMemory bufferMemory, const MemcpyFunc& memcpyFunc);

    Device* m_device = nullptr;
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
    VkIndexType m_indexType = VK_INDEX_TYPE_UINT16;

    uint32_t m_numVertices = 0;
    uint32_t m_numIndices = 0;
    std::vector<VkVertexInputAttributeDescription> m_attributesDescriptions;
    std::vector<VkVertexInputBindingDescription> m_bindingDescriptions;
    std::vector<VkDeviceSize> m_bindingOffsets;
};
