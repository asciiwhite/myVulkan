#include "mesh.h"
#include "device.h"
#include "shader.h"
#include "renderpass.h"
#include "texture.h"
#include "../utils/scopedtimelog.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <algorithm>
#include <iostream>

void Mesh::destroy()
{
    m_pipeline.destroy();
    m_descriptorSet.destroy(m_device->getVkDevice());
    m_pipelineLayout.destroy();
    m_vertexBuffer.destroy();
    Shader::release(m_shader);
    if (m_texture)
        m_texture->destroy();
    vkDestroySampler(m_device->getVkDevice(), m_sampler, nullptr);
    m_sampler = VK_NULL_HANDLE;
    m_device = nullptr;

    assert(m_indices.empty());
    assert(m_vertexData.empty());
}

void Mesh::clearFileData()
{
    m_attrib.vertices.clear();
    m_attrib.normals.clear();
    m_attrib.colors.clear();
    m_attrib.texcoords.clear();
    m_shapes.clear();
    m_materials.clear();
}

bool Mesh::loadFromObj(Device& device, const std::string& filename)
{
    m_device = &device;

    std::string err;
    bool result;
    {
        ScopedTimeLog log(("Loading mesh ") + filename);
        m_materialBaseDir = filename.substr(0, filename.find_last_of("/\\") + 1);
        result = tinyobj::LoadObj(&m_attrib, &m_shapes, &m_materials, &err, filename.c_str(), m_materialBaseDir.c_str());
    }

    if (!err.empty())
    {
        std::cerr << err << std::endl;
    }
    if (!result)
    {
        std::cerr << "Failed to load " << filename;
        return false;
    }

    if (m_attrib.vertices.size() == 0)
    {
        std::cout << "No vertices to load " << filename << std::endl;
        return false;
    }

    calculateBoundingBox();

    if (hasUniqueVertexAttributes())
    {
        
        createSeparateVertexAttributes();
    }
    else
    {
        createInterleavedVertexAttributes();
    }

    loadMaterials();

    m_shader = selectShaderFromAttributes();

    clearFileData();

    return m_shader != nullptr;
}

bool Mesh::hasUniqueVertexAttributes() const
{
    return !(((m_attrib.normals.size() != m_attrib.vertices.size()) ||
        (m_attrib.colors.size() > 0 && m_attrib.colors.size() != m_attrib.vertices.size()) ||
        (m_attrib.texcoords.size() > 0 && m_attrib.texcoords.size() != m_attrib.vertices.size())));
}

void Mesh::createInterleavedVertexAttributes()
{
    ScopedTimeLog log("Creating interleaved vertex data");

    uint32_t numIndices = 0;
    std::for_each(m_shapes.begin(), m_shapes.end(), [&](const auto& shape) { numIndices += static_cast<uint32_t>(shape.mesh.indices.size()); });
    m_indices.reserve(numIndices);

    std::unordered_map<size_t, uint32_t> uniqueVertices;

    m_vertexSize = 9 + (m_attrib.texcoords.empty() ? 0 : 2);
    m_vertexOffset = 0;
    m_vertexData.resize(m_vertexSize);
    for (const auto& shape : m_shapes)
    {
        for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++)
        {
            const auto idx0 = shape.mesh.indices[3 * f + 0];
            const auto idx1 = shape.mesh.indices[3 * f + 1];
            const auto idx2 = shape.mesh.indices[3 * f + 2];

            glm::vec3 diffuseColor(1.0f);
            auto current_material_id = shape.mesh.material_ids[f];
            if (current_material_id >= 0)
            {
                diffuseColor = glm::make_vec3(&m_materials[current_material_id].diffuse[0]);
            };

            if (m_attrib.normals.empty())
            {
                const auto faceNormal = calculateFaceNormal(idx0, idx1, idx2);
                addVertex(idx0, uniqueVertices, faceNormal, diffuseColor);
                addVertex(idx1, uniqueVertices, faceNormal, diffuseColor);
                addVertex(idx2, uniqueVertices, faceNormal, diffuseColor);
            }
            else
            {
                const auto vertexNormal0 = glm::make_vec3(&m_attrib.normals[3 * idx0.normal_index]);
                const auto vertexNormal1 = glm::make_vec3(&m_attrib.normals[3 * idx1.normal_index]);
                const auto vertexNormal2 = glm::make_vec3(&m_attrib.normals[3 * idx2.normal_index]);
                addVertex(idx0, uniqueVertices, vertexNormal0, diffuseColor);
                addVertex(idx1, uniqueVertices, vertexNormal1, diffuseColor);
                addVertex(idx2, uniqueVertices, vertexNormal2, diffuseColor);
            }
        }
    }

    const auto uniqueVertexCount = static_cast<uint32_t>(uniqueVertices.size());

    uint32_t interleavedOffset = 0;
    std::vector<VertexBuffer::AttributeDescription> vertexDesc;
    vertexDesc.push_back({ 0, 3, uniqueVertexCount, &m_vertexData.front() });
    interleavedOffset += 3 * sizeof(float);

    vertexDesc.push_back({ 1, 3, uniqueVertexCount, &m_vertexData.front(), interleavedOffset });
    interleavedOffset += 3 * sizeof(float);

    vertexDesc.push_back({ 2, 3, uniqueVertexCount, &m_vertexData.front(), interleavedOffset });
    interleavedOffset += 3 * sizeof(float);

    if (!m_attrib.texcoords.empty())
    {
        vertexDesc.push_back({ 3, 2, uniqueVertexCount, &m_vertexData.front(), interleavedOffset });
        interleavedOffset += 2 * sizeof(float);
    }
    assert(interleavedOffset == m_vertexSize * sizeof(float));

    m_vertexBuffer.createFromInterleavedAttributes(m_device, vertexDesc);
    m_vertexData.clear();

    m_vertexBuffer.setIndices(&m_indices.front(), static_cast<uint32_t>(m_indices.size()));
    m_indices.clear();

    std::cout << "Triangle count:\t\t " << m_indices.size() / 3 << std::endl;
    std::cout << "Vertex count from file:\t " << m_attrib.vertices.size() / 3 << std::endl;
    std::cout << "Unique vertex count:\t " << uniqueVertexCount << std::endl;
}

glm::vec3 Mesh::calculateFaceNormal(const tinyobj::index_t idx0, const tinyobj::index_t idx1, const tinyobj::index_t idx2) const
{
    const auto v0 = glm::make_vec3(&m_attrib.vertices[3 * idx0.vertex_index]);
    const auto v1 = glm::make_vec3(&m_attrib.vertices[3 * idx1.vertex_index]);
    const auto v2 = glm::make_vec3(&m_attrib.vertices[3 * idx2.vertex_index]);
    const auto v10 = v1 - v0;
    const auto v20 = v2 - v0;
    return glm::normalize(glm::cross(v10, v20));
}

void Mesh::addVertex(const tinyobj::index_t index, UniqueVertexMap& uniqueVertices, const glm::vec3& normal, const glm::vec3& diffuseColor)
{
    assert(index.vertex_index >= 0);

    uint32_t currentSize = 0;
    memcpy(&m_vertexData[m_vertexOffset + currentSize], &m_attrib.vertices[3 * index.vertex_index], 3 * sizeof(float));
    currentSize += 3;

    memcpy(&m_vertexData[m_vertexOffset + currentSize], &normal.x, 3 * sizeof(float));
    currentSize += 3;

    m_vertexData[m_vertexOffset + currentSize + 0] = m_attrib.colors[3 * index.vertex_index + 0] * diffuseColor.r;
    m_vertexData[m_vertexOffset + currentSize + 1] = m_attrib.colors[3 * index.vertex_index + 1] * diffuseColor.g;
    m_vertexData[m_vertexOffset + currentSize + 2] = m_attrib.colors[3 * index.vertex_index + 2] * diffuseColor.b;
    currentSize += 3;

    if (m_attrib.texcoords.size() > 0)
    {
        assert(index.texcoord_index >= 0);
        m_vertexData[m_vertexOffset + currentSize] = m_attrib.texcoords[2 * index.texcoord_index];
        m_vertexData[m_vertexOffset + currentSize + 1] = 1.0f - m_attrib.texcoords[2 * index.texcoord_index + 1];
        currentSize += 2;
    }

    assert(m_vertexSize == currentSize);
    const auto vertexHash = std::_Hash_seq(reinterpret_cast<const unsigned char*>(&m_vertexData[m_vertexOffset]), currentSize * sizeof(float));
    if (uniqueVertices.count(vertexHash) == 0)
    {
        uniqueVertices[vertexHash] = static_cast<uint32_t>(uniqueVertices.size());
        m_vertexData.resize(m_vertexData.size() + currentSize);
        m_vertexOffset += currentSize;
    }

    m_indices.push_back(uniqueVertices[vertexHash]);
}

void Mesh::createSeparateVertexAttributes()
{
    ScopedTimeLog log("Creating separate vertex data");

    std::vector<uint32_t> indices;
    uint32_t numIndices = 0;
    std::for_each(m_shapes.begin(), m_shapes.end(), [&](const auto& shape) { numIndices += static_cast<uint32_t>(shape.mesh.indices.size()); });
    indices.reserve(numIndices);

    const auto vertexCount =  static_cast<uint32_t>(m_attrib.vertices.size() / 3);

    std::vector<VertexBuffer::AttributeDescription> vertexDesc(1, { 0, 3, vertexCount, &m_attrib.vertices.front() });
    if (!m_attrib.normals.empty())
    {
        vertexDesc.push_back({ 1, 3, vertexCount, &m_attrib.normals.front() });
    }
    if (!m_attrib.colors.empty())
    {
        vertexDesc.push_back({ 2, 3, vertexCount, &m_attrib.colors.front() });
    }
    if (!m_attrib.texcoords.empty())
    {
        vertexDesc.push_back({ 3, 2, vertexCount, &m_attrib.texcoords.front() });
    }

    m_vertexBuffer.createFromSeparateAttributes(m_device, vertexDesc);

    for (const auto& shape : m_shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            assert(index.vertex_index >= 0);
            indices.push_back(static_cast<uint32_t>(index.vertex_index));
        }
    }
    m_vertexBuffer.setIndices(&indices.front(), static_cast<uint32_t>(indices.size()));
    m_indices.clear();

    std::cout << "Triangle count:\t " << indices.size() / 3 << std::endl;
    std::cout << "Vertex count:\t " << m_attrib.vertices.size() / 3 << std::endl;
}

std::shared_ptr<Shader> Mesh::selectShaderFromAttributes()
{
    assert(!m_shader);

    const std::string shaderPath = "data/shaders/";
    std::string vertexShaderName;
    std::string fragmemtShaderName;
    
    if (m_attrib.texcoords.size() > 0 && (m_texture && m_sampler != VK_NULL_HANDLE))
    {
        vertexShaderName = "color_normal_texture";
        fragmemtShaderName = "color_texture";
    }
    else
    {
        vertexShaderName = "color_normal";
        fragmemtShaderName = "color";
    }
    
    const auto vertexShaderFilename = shaderPath + vertexShaderName + ".vert.spv";
    const auto fragmentShaderFilename = shaderPath + fragmemtShaderName + ".frag.spv";

    return Shader::getShader(m_device->getVkDevice(), vertexShaderFilename, fragmentShaderFilename);
}

void Mesh::loadMaterials()
{
    if (m_shapes.empty() || m_materials.empty())
        return;

    ScopedTimeLog log("Loading materials");

    assert(!m_shapes[0].mesh.material_ids.empty());
    const auto materialId = m_shapes[0].mesh.material_ids[0];
    const auto& diffuseTextureFilename = m_materials[materialId].diffuse_texname;

    if (!diffuseTextureFilename.empty())
    {
        auto texture = std::make_shared<Texture>();
        const auto textureFilename = m_materialBaseDir + diffuseTextureFilename;
        if (texture->loadFromFile(m_device, textureFilename))
        {
            m_texture = texture;
            m_device->createSampler(m_sampler); 
        }
    }
}

void Mesh::addUniformBuffer(VkShaderStageFlags shaderStages, VkBuffer uniformBuffer)
{
    m_descriptorSet.addUniformBuffer(shaderStages, uniformBuffer);
}

bool Mesh::finalize(const RenderPass& renderPass)
{
    if (m_texture && m_sampler != VK_NULL_HANDLE)
    {
        m_descriptorSet.addSampler(m_texture->getImageView(), m_sampler);
    }
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

void Mesh::calculateBoundingBox()
{
    m_minBB.x = m_maxBB.x = m_attrib.vertices[0];
    m_minBB.y = m_maxBB.y = m_attrib.vertices[1];
    m_minBB.z = m_maxBB.z = m_attrib.vertices[2];

    for (uint32_t i = 3; i < m_attrib.vertices.size(); i += 3)
    {
        m_minBB.x = std::min(m_minBB.x, m_attrib.vertices[i + 0]);
        m_maxBB.x = std::max(m_maxBB.x, m_attrib.vertices[i + 0]);
        m_minBB.y = std::min(m_minBB.y, m_attrib.vertices[i + 1]);
        m_maxBB.y = std::max(m_maxBB.y, m_attrib.vertices[i + 1]);
        m_minBB.z = std::min(m_minBB.z, m_attrib.vertices[i + 2]);
        m_maxBB.z = std::max(m_maxBB.z, m_attrib.vertices[i + 2]);
    }
}

void Mesh::getBoundingbox(glm::vec3& min, glm::vec3& max) const
{
    min = m_minBB;
    max = m_maxBB;
}