#include "mesh.h"
#include "device.h"
#include "shader.h"
#include "texture.h"
#include "../utils/scopedtimelog.h"

#include <algorithm>
#include <iostream>

const uint32_t SET_ID_CAMERA = 0;
const uint32_t BINDING_ID_CAMERA = 0;
const uint32_t SET_ID_MATERIAL = 1;
const uint32_t BINDING_ID_MATERIAL = 0;
const uint32_t BINDING_ID_TEXTURE_DIFFUSE = 1;

bool Mesh::init(Device& device, const MeshDescription& meshDesc, VkBuffer cameraUniformBuffer, VkRenderPass renderPass)
{
    m_device = &device;
    m_shapes = meshDesc.shapes;
    m_sampler = m_device->createSampler();

    if (!loadMaterials(meshDesc.materials))
        return false;

    createVertexBuffer(meshDesc.geometry);
    createDescriptors(cameraUniformBuffer);
    if (!createPipelines(renderPass))
        return false;

    return true;
}

void Mesh::destroy()
{
    if (!m_device)
        return;

    m_shapes.clear();
    
    for (auto& desc : m_materials)
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
    m_device->destroyPipelineLayout(m_pipelineLayout);
    m_materialDescriptorPool.destroy(*m_device);
    m_cameraUniformDescriptorSet.free(*m_device, m_cameraDescriptorPool);
    m_cameraDescriptorPool.destroy(*m_device);

    m_vertexBuffer.destroy();
    m_device->destroySampler(m_sampler);
    m_device = nullptr;
}

bool Mesh::loadMaterials(const std::vector<MaterialDescription>& materials)
{
    if (m_shapes.empty())
        return false;

    std::cout << "Found " << materials.size() << " materials" << std::endl;

    ScopedTimeLog log("Loading materials");

    // TODO: only load used materials
    m_materials.resize(materials.size());
    auto materialDescIter = m_materials.begin();

    for (auto& material : materials)
    {
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

        if (!material.textureFilename.empty())
        {
            auto texture = TextureManager::Acquire(*m_device, material.textureFilename);
            if (texture)
            {
                desc.diffuseTexture = texture;
                desc.descriptorSet.setImageSampler(BINDING_ID_TEXTURE_DIFFUSE, texture.imageView, m_sampler);
            }
        }

        desc.shader = selectShaderFromAttributes(desc.diffuseTexture);
        if (!desc.shader)
            return false;
    }

    return true;
}

Shader Mesh::selectShaderFromAttributes(bool useTexture)
{
    const static std::string shaderPath = "data/shaders/";
    std::string vertexShaderName = "color_normal";
    std::string fragmemtShaderName = "color";
    
    if (useTexture)
    {
        vertexShaderName += "_texture";
        fragmemtShaderName += "_texture";
    }
    
    const auto vertexShaderFilename = shaderPath + vertexShaderName + ".vert.spv";
    const auto fragmentShaderFilename = shaderPath + fragmemtShaderName + ".frag.spv";

    return ShaderManager::Acquire(*m_device, ShaderResourceHandler::ShaderModulesDescription
        { { VK_SHADER_STAGE_VERTEX_BIT, vertexShaderFilename },
          { VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderFilename} });
}

void Mesh::createVertexBuffer(const MeshDescription::Geometry& geometry)
{
    assert(!geometry.vertexAttribs.empty() || !geometry.interleavedVertexAttribs.empty());

    if (!geometry.vertexAttribs.empty())
    {
        m_vertexBuffer.createFromSeparateAttributes(m_device, geometry.vertexAttribs);
    }
    else
    {
        m_vertexBuffer.createFromInterleavedAttributes(m_device, geometry.vertexCount, geometry.vertexSize * sizeof(float), const_cast<float*>(geometry.vertices.data()), geometry.interleavedVertexAttribs);
    }

    m_vertexBuffer.setIndices(geometry.indices.data(), static_cast<uint32_t>(geometry.indices.size()));
}

void Mesh::createDescriptors(VkBuffer cameraUniformBuffer)
{
    m_cameraUniformDescriptorSet.setUniformBuffer(BINDING_ID_CAMERA, cameraUniformBuffer);

    m_cameraDescriptorPool.init(*m_device, 1,
        { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 } });

    m_cameraDescriptorSetLayout.init(*m_device,
        { { BINDING_ID_CAMERA, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT } });

    m_cameraUniformDescriptorSet.allocateAndUpdate(*m_device, m_cameraDescriptorSetLayout, m_cameraDescriptorPool);

    const auto materialDescriptorCount = static_cast<uint32_t>(m_materials.size());

    m_materialDescriptorPool.init(*m_device, materialDescriptorCount,
        { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, materialDescriptorCount },
          { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, materialDescriptorCount } });

    m_materialDescriptorSetLayout.init(*m_device,
        { { BINDING_ID_MATERIAL, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
          { BINDING_ID_TEXTURE_DIFFUSE, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT } });
}

bool Mesh::createPipelines(VkRenderPass renderPass)
{
    m_pipelineLayout = m_device->createPipelineLayout({ m_cameraDescriptorSetLayout, m_materialDescriptorSetLayout });

    for (auto& desc : m_materials)
    {
        desc.descriptorSet.allocateAndUpdate(*m_device, m_materialDescriptorSetLayout, m_materialDescriptorPool);

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

void Mesh::render(VkCommandBuffer commandBuffer) const
{
    VkPipeline currentPipeline = VK_NULL_HANDLE;

    m_cameraUniformDescriptorSet.bind(commandBuffer, m_pipelineLayout, SET_ID_CAMERA);

    for (const auto& shape : m_shapes)
    {
        assert(shape.materialId <= m_materials.size());
        const auto& materialDesc = m_materials[shape.materialId];

        if (currentPipeline != materialDesc.pipeline)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, materialDesc.pipeline);
            currentPipeline = materialDesc.pipeline;
        }

        materialDesc.descriptorSet.bind(commandBuffer, m_pipelineLayout, SET_ID_MATERIAL);

        m_vertexBuffer.draw(commandBuffer, 1, shape.startIndex, shape.indexCount);
    }
}

uint32_t Mesh::numVertices() const
{
    return m_vertexBuffer.numVertices();
}

uint32_t Mesh::numTriangles() const
{
    return m_vertexBuffer.numIndices() / 3;
}