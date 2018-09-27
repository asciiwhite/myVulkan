#include "texturearraymesh.h"
#include "device.h"
#include "shader.h"
#include "texture.h"
#include "../utils/scopedtimelog.h"
#include "../utils/hasher.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <algorithm>
#include <iostream>

const uint32_t SET_ID_CAMERA = 0;
const uint32_t BINDING_ID_CAMERA = 0;
const uint32_t SET_ID_MATERIAL = 1;
const uint32_t BINDING_ID_MATERIAL = 0;
const uint32_t BINDING_ID_TEXTURE_DIFFUSE = 1;
const uint32_t SET_ID_TEXTURES = 2;
const uint32_t BINDING_ID_SAMPLER = 0;
const uint32_t BINDING_ID_TEXTURES = 1;

void TextureArrayMesh::destroy()
{
    m_shapeDescs.clear();
    
    for (auto& desc : m_materialDescs)
    {
        m_device->destroyBuffer(desc.material);
        GraphicsPipeline::Release(*m_device, desc.pipeline);
        ShaderManager::Release(*m_device, desc.shader);
        if (desc.diffuseTexture)
            TextureManager::Release(*m_device, desc.diffuseTexture);
    }
    m_materials.clear();
    m_cameraDescriptorSetLayout.destroy(*m_device);
    m_materialDescriptorSetLayout.destroy(*m_device);
    m_texturesDescriptorSetLayout.destroy(*m_device);
    m_mainDescriptorPool.destroy(*m_device);
    m_device->destroyPipelineLayout(m_pipelineLayout);

    m_vertexBuffer.destroy();
    m_device->destroySampler(m_sampler);
    m_device = nullptr;

    assert(m_indices.empty());
    assert(m_vertexData.empty());
}

void TextureArrayMesh::clearFileData()
{
    m_attrib.vertices.clear();
    m_attrib.normals.clear();
    m_attrib.colors.clear();
    m_attrib.texcoords.clear();
    m_shapes.clear();
    m_materials.clear();
}

bool TextureArrayMesh::loadFromObj(Device& device, const std::string& filename)
{
    m_device = &device;
    m_fileName = filename;

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

    if (m_shapes.empty())
    {
        std::cout << "No shapes to load from" << filename << std::endl;
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

    std::cout << "Created " << m_shapeDescs.size() << " shapes" << std::endl;

    loadMaterials();

    sortShapesByMaterialTransparency();

    clearFileData();

    return true;
}

bool TextureArrayMesh::hasUniqueVertexAttributes() const
{
    return !(((m_attrib.normals.empty() != m_attrib.vertices.empty()) ||
        (!m_attrib.texcoords.empty() && m_attrib.texcoords.size() != m_attrib.vertices.size())));
}

void TextureArrayMesh::createInterleavedVertexAttributes()
{
    {
        ScopedTimeLog log("Creating interleaved vertex data");

        uint32_t numIndices = 0;
        std::for_each(m_shapes.begin(), m_shapes.end(), [&](const tinyobj::shape_t& shape) { numIndices += static_cast<uint32_t>(shape.mesh.indices.size()); });
        m_indices.reserve(numIndices);

        std::unordered_map<size_t, uint32_t> uniqueVertices;

        uint32_t shapeStartIndex = 0;
        assert(!m_shapes[0].mesh.material_ids.empty());
        int currentMaterialId = m_shapes[0].mesh.material_ids.front();

        m_vertexSize = 6 + (m_attrib.texcoords.empty() ? 0 : 2); // vertex + normals + texcoords
        m_vertexOffset = 0;
        m_vertexData.resize(m_vertexSize);
        for (const auto& shape : m_shapes)
        {
            assert(!shape.mesh.material_ids.empty());

            for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++)
            {
                if (shape.mesh.material_ids[f] != currentMaterialId)
                {
                    shapeStartIndex = addShape(shapeStartIndex, currentMaterialId);
                    currentMaterialId = shape.mesh.material_ids[f];
                }

                const auto idx0 = shape.mesh.indices[3 * f + 0];
                const auto idx1 = shape.mesh.indices[3 * f + 1];
                const auto idx2 = shape.mesh.indices[3 * f + 2];

                if (m_attrib.normals.empty())
                {
                    const auto faceNormal = calculateFaceNormal(idx0, idx1, idx2);
                    addVertex(idx0, uniqueVertices, faceNormal);
                    addVertex(idx1, uniqueVertices, faceNormal);
                    addVertex(idx2, uniqueVertices, faceNormal);
                }
                else
                {
                    const auto vertexNormal0 = glm::make_vec3(&m_attrib.normals[3 * idx0.normal_index]);
                    const auto vertexNormal1 = glm::make_vec3(&m_attrib.normals[3 * idx1.normal_index]);
                    const auto vertexNormal2 = glm::make_vec3(&m_attrib.normals[3 * idx2.normal_index]);
                    addVertex(idx0, uniqueVertices, vertexNormal0);
                    addVertex(idx1, uniqueVertices, vertexNormal1);
                    addVertex(idx2, uniqueVertices, vertexNormal2);
                }
            }
        }
        addShape(shapeStartIndex, currentMaterialId);

        const auto uniqueVertexCount = static_cast<uint32_t>(uniqueVertices.size());
        auto vertexAttributeSize = (3 + 3) * 4;

        std::vector<VertexBuffer::InterleavedAttributeDescription> vertexDesc{
            { 0, 3, 0 }, // vertices
            { 1, 3, 3 * sizeof(float) } }; // normals

        if (!m_attrib.texcoords.empty())
        {
            vertexDesc.emplace_back( 2, 2, static_cast<uint32_t>(6 * sizeof(float)));
            vertexAttributeSize += 2 * 4;
        }

       m_vertexBuffer.createFromInterleavedAttributes(m_device, uniqueVertexCount, vertexAttributeSize, &m_vertexData.front(), vertexDesc);
        m_vertexData.clear();

        std::cout << "Triangle count:\t\t " << m_indices.size() / 3 << std::endl;
        std::cout << "Vertex count from file:\t " << m_attrib.vertices.size() / 3 << std::endl;
        std::cout << "Unique vertex count:\t " << uniqueVertexCount << std::endl;
    }

    mergeShapesByMaterial();

    m_vertexBuffer.setIndices(&m_indices.front(), static_cast<uint32_t>(m_indices.size()));
    m_indices.clear();
}

uint32_t TextureArrayMesh::addShape(uint32_t startIndex, int materialId)
{
    const auto stopIndex = static_cast<uint32_t>(m_indices.size());
    const auto indexCount = stopIndex - startIndex;
    m_shapeDescs.push_back({ startIndex, indexCount, materialId == -1 ? static_cast<uint32_t>(m_materials.size()) : static_cast<uint32_t>(materialId) });
    return stopIndex;
}

glm::vec3 TextureArrayMesh::calculateFaceNormal(const tinyobj::index_t idx0, const tinyobj::index_t idx1, const tinyobj::index_t idx2) const
{
    const auto v0 = glm::make_vec3(&m_attrib.vertices[3 * idx0.vertex_index]);
    const auto v1 = glm::make_vec3(&m_attrib.vertices[3 * idx1.vertex_index]);
    const auto v2 = glm::make_vec3(&m_attrib.vertices[3 * idx2.vertex_index]);
    const auto v10 = v1 - v0;
    const auto v20 = v2 - v0;
    return glm::normalize(glm::cross(v10, v20));
}

void TextureArrayMesh::addVertex(const tinyobj::index_t index, UniqueVertexMap& uniqueVertices, const glm::vec3& faceNormal)
{
    assert(index.vertex_index >= 0);

    memcpy(&m_vertexData[m_vertexOffset + 0], &m_attrib.vertices[3 * index.vertex_index], 3 * sizeof(float));
    memcpy(&m_vertexData[m_vertexOffset + 3], &faceNormal.x, 3 * sizeof(float));

    uint32_t currentSize = 6;
    if (!m_attrib.texcoords.empty())
    {
        assert(index.texcoord_index >= 0);
        m_vertexData[m_vertexOffset + 6] = m_attrib.texcoords[2 * index.texcoord_index + 0];
        m_vertexData[m_vertexOffset + 7] = 1.0f - m_attrib.texcoords[2 * index.texcoord_index + 1];
        currentSize += 2;
    }

    assert(m_vertexSize == currentSize);
    Hasher hasher;
    hasher.add(reinterpret_cast<const unsigned char*>(&m_vertexData[m_vertexOffset]), currentSize * sizeof(float));
    const auto vertexHash = hasher.get();
    if (uniqueVertices.count(vertexHash) == 0)
    {
        uniqueVertices[vertexHash] = static_cast<uint32_t>(uniqueVertices.size());
        m_vertexData.resize(m_vertexData.size() + currentSize);
        m_vertexOffset += currentSize;
    }

    m_indices.push_back(uniqueVertices[vertexHash]);
}

void TextureArrayMesh::createSeparateVertexAttributes()
{
    ScopedTimeLog log("Creating separate vertex data");

    std::vector<uint32_t> indices;
    uint32_t numIndices = 0;
    std::for_each(m_shapes.begin(), m_shapes.end(), [&](const tinyobj::shape_t& shape) { numIndices += static_cast<uint32_t>(shape.mesh.indices.size()); });
    indices.reserve(numIndices);

    m_shapeDescs.push_back({ 0u, numIndices, 0u });

    const auto vertexCount =  static_cast<uint32_t>(m_attrib.vertices.size() / 3);

    std::vector<VertexBuffer::AttributeDescription> vertexDesc{ { 0, 3, vertexCount, &m_attrib.vertices.front() } };

    if (!m_attrib.normals.empty())
    {
        vertexDesc.emplace_back( 1, 3, vertexCount, &m_attrib.normals.front() );
    }
    if (!m_attrib.texcoords.empty())
    {
        vertexDesc.emplace_back( 2, 2, vertexCount, &m_attrib.texcoords.front() );
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

Shader TextureArrayMesh::selectShaderFromAttributes(bool useTexture)
{
    const std::string shaderPath = "data/shaders/";
    std::string vertexShaderName = "color_normal";
    std::string fragmemtShaderName = "color";
    
    if (!m_attrib.texcoords.empty() && useTexture)
    {
        vertexShaderName += "_texture";
        fragmemtShaderName += "_texture_array";
    }
    
    const auto vertexShaderFilename = shaderPath + vertexShaderName + ".vert.spv";
    const auto fragmentShaderFilename = shaderPath + fragmemtShaderName + ".frag.spv";

    return ShaderManager::Acquire(*m_device, ShaderResourceHandler::ShaderModulesDescription
        { { VK_SHADER_STAGE_VERTEX_BIT, vertexShaderFilename },
          { VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderFilename} });
}

void TextureArrayMesh::loadMaterials()
{
    if (m_shapes.empty())
        return;

    std::cout << "Found " << m_materials.size() << " materials" << std::endl;

    ScopedTimeLog log("Loading materials");

    createDefaultMaterial();

    m_sampler = m_device->createSampler();

    // TODO: only load used materials
    m_materialDescs.resize(m_materials.size());
    auto materialDescIter = m_materialDescs.begin();
    
    for (auto& material : m_materials)
    {
        std::string textureFilename;
        if (!material.diffuse_texname.empty())
        {
            textureFilename = material.diffuse_texname;
        }
        else if (!material.emissive_texname.empty())
        {
            textureFilename = material.emissive_texname;
        }

        auto& desc = *materialDescIter++;

        const uint32_t uboSize = sizeof(glm::vec4) + sizeof(glm::vec4) + sizeof(glm::vec4);
        desc.material = m_device->createBuffer(uboSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data = m_device->mapBuffer(desc.material, uboSize, 0);
        std::memcpy(data, &material.ambient, 3 * sizeof(float));
        std::memcpy(static_cast<char*>(data) + 4 * sizeof(float), &material.diffuse, 3 * sizeof(float));
        std::memcpy(static_cast<char*>(data) + 8 * sizeof(float), &material.emission, 3 * sizeof(float));
        m_device->unmapBuffer(desc.material);

        desc.descriptorSet.setUniformBuffer(BINDING_ID_MATERIAL, desc.material);

        if (!textureFilename.empty())
        {
            const auto textureFullname = m_materialBaseDir + textureFilename;
            auto texture = TextureManager::Acquire(*m_device, textureFullname);
            if (texture)
            {
                desc.diffuseTexture = texture;
                desc.imageIdx = static_cast<uint32_t>(m_imageViews.size());
                m_imageViews.push_back(texture.imageView);
            }
        }

        desc.shader = selectShaderFromAttributes(desc.diffuseTexture);
    }
}

void TextureArrayMesh::createDefaultMaterial()
{
    tinyobj::material_t defaultMaterial;
    defaultMaterial.name = "default";
    defaultMaterial.ambient[0] = defaultMaterial.ambient[1] = defaultMaterial.ambient[2] = 0.1f;
    defaultMaterial.diffuse[0] = defaultMaterial.diffuse[1] = defaultMaterial.diffuse[2] = 1.0f;
    defaultMaterial.emission[0] = defaultMaterial.emission[1] = defaultMaterial.emission[2] = 0.0f;
    m_materials.push_back(defaultMaterial);
}

void TextureArrayMesh::mergeShapesByMaterial()
{
    ScopedTimeLog log("Merging shapes by material");

    std::vector<uint32_t> mergesIndicies(m_indices.size());
    std::vector<ShapeDesc> mergesShapes;

    std::sort(m_shapeDescs.begin(), m_shapeDescs.end(), [](const ShapeDesc& a, const ShapeDesc& b) { return a.materialId < b.materialId; });

    auto currentMaterialId = m_shapeDescs.front().materialId;
    mergesShapes.push_back({ 0, 0, currentMaterialId });
    uint32_t mergedShapeIndex = 0;
    for (const auto& shapeDesc : m_shapeDescs)
    {
        const auto startId = mergesShapes[mergedShapeIndex].startIndex + mergesShapes[mergedShapeIndex].indexCount;

        if (currentMaterialId != shapeDesc.materialId)
        {
            currentMaterialId = shapeDesc.materialId;
            mergesShapes.push_back({ startId, 0, currentMaterialId });
            mergedShapeIndex++;
        }

        std::memcpy(&mergesIndicies[startId], &m_indices[shapeDesc.startIndex], shapeDesc.indexCount * sizeof(uint32_t));
        mergesShapes[mergedShapeIndex].indexCount += shapeDesc.indexCount;
    }

    std::cout << "Merged " << m_shapeDescs.size() << " shapes into " << mergesShapes.size() << " shapes" << std::endl;

    m_indices = mergesIndicies;
    m_shapeDescs = mergesShapes;
}

bool TextureArrayMesh::isTransparentMaterial(uint32_t id) const
{
    assert(id < m_materialDescs.size());
    return m_materialDescs[id].diffuseTexture && m_materialDescs[id].diffuseTexture.hasTranspareny();
}

void TextureArrayMesh::sortShapesByMaterialTransparency()
{
    std::sort(m_shapeDescs.begin(), m_shapeDescs.end(), [=](const ShapeDesc& a, const ShapeDesc& b) {
        auto transparencyPenaltyA = isTransparentMaterial(a.materialId) ? 10 : 1;
        auto transparencyPenaltyB = isTransparentMaterial(b.materialId) ? 10 : 1;
        return  (a.materialId * transparencyPenaltyA) <
                (b.materialId * transparencyPenaltyB); });
}

bool TextureArrayMesh::finalize(VkRenderPass renderPass, VkBuffer cameraUniformBuffer)
{
    m_cameraUniformDescriptorSet.setUniformBuffer(BINDING_ID_CAMERA, cameraUniformBuffer);

    const auto descriptorSetCount = static_cast<uint32_t>(m_materialDescs.size()) + 1 + 1; // camera set + texture set
    const auto uniformBufferCount = static_cast<uint32_t>(m_materialDescs.size()) + 1; // + camera set
    const auto samplerCount = 1u;
    const auto sampledImageCount = static_cast<uint32_t>(m_imageViews.size());

    m_mainDescriptorPool.init(*m_device, descriptorSetCount,
    { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBufferCount },
      { VK_DESCRIPTOR_TYPE_SAMPLER, samplerCount },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, sampledImageCount } });

    m_cameraDescriptorSetLayout.init(*m_device,
    { { BINDING_ID_CAMERA, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT } });

    m_cameraUniformDescriptorSet.allocateAndUpdate(*m_device, m_cameraDescriptorSetLayout, m_mainDescriptorPool);
    
    m_texturesDescriptorSetLayout.init(*m_device,
    { { BINDING_ID_SAMPLER, 1, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
      { BINDING_ID_TEXTURES, sampledImageCount, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT } });

    m_texturesDescriptorSet.setSampler(BINDING_ID_SAMPLER, m_sampler);
    m_texturesDescriptorSet.setImageArray(BINDING_ID_TEXTURES, m_imageViews);
    m_texturesDescriptorSet.allocateAndUpdate(*m_device, m_texturesDescriptorSetLayout, m_mainDescriptorPool);

    m_materialDescriptorSetLayout.init(*m_device,
        { { BINDING_ID_MATERIAL, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT } });

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(uint32_t);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // keep order of descriptor sets here regarding set id
    m_pipelineLayout = m_device->createPipelineLayout({ m_cameraDescriptorSetLayout, m_materialDescriptorSetLayout, m_texturesDescriptorSetLayout }, { pushConstantRange });

    for (auto& desc : m_materialDescs)
    {
        desc.descriptorSet.allocateAndUpdate(*m_device, m_materialDescriptorSetLayout, m_mainDescriptorPool);

        auto isTransparent = desc.diffuseTexture && desc.diffuseTexture.hasTranspareny();

        GraphicsPipelineSettings settings;
        settings.setAlphaBlending(isTransparent).setCullMode(isTransparent ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT);

        desc.pipeline = GraphicsPipeline::Acquire(*m_device,
            renderPass,
            m_pipelineLayout,
            settings,
            desc.shader.shaderStageCreateInfos,
            m_vertexBuffer.getAttributeDescriptions(),
            m_vertexBuffer.getBindingDescriptions());

        if (!desc.pipeline)
        {
            return false;
        }
    }

    return true;
}

void TextureArrayMesh::render(VkCommandBuffer commandBuffer) const
{
    VkPipeline currentPipeline = VK_NULL_HANDLE;

    m_cameraUniformDescriptorSet.bind(commandBuffer, m_pipelineLayout, SET_ID_CAMERA);
    m_texturesDescriptorSet.bind(commandBuffer, m_pipelineLayout, SET_ID_TEXTURES);

    for (const auto& shapeDesc : m_shapeDescs)
    {
        assert(shapeDesc.materialId <= m_materialDescs.size());
        const auto& materialDesc = m_materialDescs[shapeDesc.materialId];

        if (currentPipeline != materialDesc.pipeline)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, materialDesc.pipeline);
            currentPipeline = materialDesc.pipeline;
        }

        if (materialDesc.imageIdx != ~0u)
        {
            vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), static_cast<const void*>(&materialDesc.imageIdx));
        }

        materialDesc.descriptorSet.bind(commandBuffer, m_pipelineLayout, SET_ID_MATERIAL);

        m_vertexBuffer.draw(commandBuffer, 1, shapeDesc.startIndex, shapeDesc.indexCount);
    }
}

void TextureArrayMesh::calculateBoundingBox()
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

void TextureArrayMesh::getBoundingbox(glm::vec3& min, glm::vec3& max) const
{
    min = m_minBB;
    max = m_maxBB;
}

uint32_t TextureArrayMesh::numVertices() const
{
    return m_vertexBuffer.numVertices();
}

uint32_t TextureArrayMesh::numIndices() const
{
    return m_vertexBuffer.numIndices();
}

uint32_t TextureArrayMesh::numShapes() const
{
    return static_cast<uint32_t>(m_shapeDescs.size());
}

const std::string& TextureArrayMesh::fileName() const
{
    return m_fileName;
}