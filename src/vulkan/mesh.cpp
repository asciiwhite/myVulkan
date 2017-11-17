#include "mesh.h"
#include "device.h"
#include "shader.h"
#include "renderpass.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <unordered_map>
#include <algorithm>

void Mesh::destroy()
{
    m_pipeline.destroy();
    m_descriptorSet.destroy(m_device->getVkDevice());
    m_pipelineLayout.destroy();
    m_vertexBuffer.destroy();
    Shader::release(m_shader);
    m_device = nullptr;
}

bool Mesh::loadFromObj(Device& device, const std::string& filename)
{
    m_device = &device;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    std::cout << "Loading mesh: " << filename << std::endl;

    const auto materialBaseDir = filename.substr(0, filename.find_last_of("/\\") + 1);
    const auto res = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str(), materialBaseDir.c_str());
    if (!err.empty())
    {
        std::cerr << err << std::endl;
    }
    if (!res)
    {
        std::cerr << "Failed to load " << filename << std::endl;
        return false;
    }

    if ((attrib.normals.size()   > 0 && attrib.normals.size()   != attrib.vertices.size()) ||
        (attrib.colors.size()    > 0 && attrib.colors.size()    != attrib.vertices.size()) ||
        (attrib.texcoords.size() > 0 && attrib.texcoords.size() != attrib.vertices.size()))
    {
        createInterleavedVertexAttributes(attrib, shapes);
    }
    else
    {
        createSeparateVertexAttributes(attrib, shapes);
    }

    m_shader = selectShaderFromAttributes(attrib);

    return m_shader != nullptr;
}

void Mesh::createInterleavedVertexAttributes(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes)
{
    std::vector<uint32_t> indices;
    uint32_t numIndices = 0;
    std::for_each(shapes.begin(), shapes.end(), [&](const auto& shape) { numIndices += static_cast<uint32_t>(shape.mesh.indices.size()); });
    indices.reserve(numIndices);

    uint32_t vertexSize = 3;
    const uint32_t normalSize = attrib.normals.size() > 0 ? 3 : 0;
    const uint32_t colorSize = attrib.colors.size() == attrib.vertices.size() ? 3 : 0;
    uint32_t texcoordSize = 0;// attrib.texcoords.size() > 0 ? 2 : 0;

    vertexSize += normalSize;
    vertexSize += colorSize;
    vertexSize += texcoordSize;

    std::unordered_map<size_t, uint32_t> uniqueVertices;

    uint32_t vertexOffset = 0;
    std::vector<float> vertexData(vertexSize);

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            uint32_t currentSize = 0;
            assert(index.vertex_index >= 0);
            memcpy(&vertexData[vertexOffset + currentSize], &attrib.vertices[3 * index.vertex_index], 3 * sizeof(float));
            currentSize += 3;
            if (normalSize > 0)
            {
                assert(index.normal_index >= 0);
                memcpy(&vertexData[vertexOffset + currentSize], &attrib.normals[3 * index.normal_index], 3 * sizeof(float));
                currentSize += normalSize;
            }
            if (colorSize > 0)
            {
                memcpy(&vertexData[vertexOffset + currentSize], &attrib.colors[3 * index.vertex_index], 3 * sizeof(float));
                currentSize += colorSize;
            }
            if (texcoordSize > 0)
            {
                assert(index.texcoord_index >= 0);
                memcpy(&vertexData[vertexOffset + currentSize], &attrib.texcoords[2 * index.texcoord_index], 2 * sizeof(float));
                currentSize += texcoordSize;
            }

            assert(currentSize == vertexSize);
            const auto vertexHash = std::_Hash_seq(reinterpret_cast<const unsigned char*>(&vertexData[vertexOffset]), vertexSize * sizeof(float));
            if (uniqueVertices.count(vertexHash) == 0)
            {
                uniqueVertices[vertexHash] = static_cast<uint32_t>(uniqueVertices.size());
                vertexData.resize(vertexData.size() + vertexSize);
                vertexOffset += vertexSize;
            }

            indices.push_back(uniqueVertices[vertexHash]);
        }
    }

    const auto uniqueVertexCount = static_cast<uint32_t>(uniqueVertices.size());

    uint32_t interleavedOffset = 0;
    std::vector<VertexBuffer::AttributeDescription> vertexDesc(1, { 0, 3, uniqueVertexCount, &vertexData.front() });
    interleavedOffset += 3 * sizeof(float);
    if (normalSize)
    {
        vertexDesc.push_back({ 1, 3, uniqueVertexCount, &vertexData.front(), interleavedOffset });
        interleavedOffset += 3 * sizeof(float);
    }
    if (colorSize)
    {
        vertexDesc.push_back({ 2, 3, uniqueVertexCount, &vertexData.front(), interleavedOffset });
        interleavedOffset += 3 * sizeof(float);
    }
    if (texcoordSize)
    {
        vertexDesc.push_back({ 3, 2, uniqueVertexCount, &vertexData.front(), interleavedOffset });
        interleavedOffset += 2 * sizeof(float);
    }
    assert(interleavedOffset == vertexSize * sizeof(float));

    m_vertexBuffer.createFromInterleavedAttributes(m_device, vertexDesc);

    m_vertexBuffer.setIndices(&indices.front(), static_cast<uint32_t>(indices.size()));

    std::cout << "Triangle count:\t\t " << indices.size() / 3 << std::endl;
    std::cout << "Vertex count from file:\t " << attrib.vertices.size() / 3 << std::endl;
    std::cout << "Unique vertex count:\t " << uniqueVertexCount << std::endl;
}

void Mesh::createSeparateVertexAttributes(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes)
{
    std::vector<uint32_t> indices;
    uint32_t numIndices = 0;
    std::for_each(shapes.begin(), shapes.end(), [&](const auto& shape) { numIndices += static_cast<uint32_t>(shape.mesh.indices.size()); });
    indices.reserve(numIndices);

    const auto vertexCount =  static_cast<uint32_t>(attrib.vertices.size() / 3);

    std::vector<VertexBuffer::AttributeDescription> vertexDesc(1, { 0, 3, vertexCount, &attrib.vertices.front() });
    if (!attrib.normals.empty())
    {
        vertexDesc.push_back({ 1, 3, vertexCount, &attrib.normals.front() });
    }
    if (!attrib.colors.empty())
    {
        vertexDesc.push_back({ 2, 3, vertexCount, &attrib.colors.front() });
    }
    if (!attrib.texcoords.empty())
    {
        vertexDesc.push_back({ 3, 2, vertexCount, &attrib.texcoords.front() });
    }

    m_vertexBuffer.createFromSeparateAttributes(m_device, vertexDesc);

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            assert(index.vertex_index >= 0);
            indices.push_back(static_cast<uint32_t>(index.vertex_index));
        }
    }
    m_vertexBuffer.setIndices(&indices.front(), static_cast<uint32_t>(indices.size()));

    std::cout << "Triangle count:\t " << indices.size() / 3 << std::endl;
    std::cout << "Vertex count:\t " << attrib.vertices.size() / 3 << std::endl;
}

std::shared_ptr<Shader> Mesh::selectShaderFromAttributes(const tinyobj::attrib_t& attrib)
{
    assert(!m_shader);

    const std::string shaderPath = "data/shaders/";
    std::string vertexShaderName = "color";
    std::string fragmemtShaderName = vertexShaderName;
    
    if (attrib.normals.size() > 0)
        vertexShaderName += "_normal";
    
    if (attrib.texcoords.size() > 0)
    {
        vertexShaderName += "_texture";
        fragmemtShaderName += "_texture";
    }
    
    const auto vertexShaderFilename = shaderPath + vertexShaderName + ".vert.spv";
    const auto fragmentShaderFilename = shaderPath + fragmemtShaderName + ".frag.spv";

    return Shader::getShader(m_device->getVkDevice(), vertexShaderFilename, fragmentShaderFilename);
}

void Mesh::addUniformBuffer(VkShaderStageFlags shaderStages, VkBuffer uniformBuffer)
{
    m_descriptorSet.addUniformBuffer(shaderStages, uniformBuffer);
}

bool Mesh::finalize(const RenderPass& renderPass)
{
    m_descriptorSet.finalize(m_device->getVkDevice());

    m_pipelineLayout.init(m_device->getVkDevice(), { m_descriptorSet.getLayout() });

    static const PipelineSettings defaultSettings;

    return m_pipeline.init(m_device->getVkDevice(),
        renderPass.getVkRenderPass(),
        m_pipelineLayout.getVkPipelineLayout(),
        defaultSettings,
        m_shader->getShaderStages(),
        &m_vertexBuffer);
}

void Mesh::render(VkCommandBuffer commandBuffer) const
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.getVkPipeline());

    m_descriptorSet.bind(commandBuffer, m_pipelineLayout.getVkPipelineLayout());

    m_vertexBuffer.draw(commandBuffer);
}