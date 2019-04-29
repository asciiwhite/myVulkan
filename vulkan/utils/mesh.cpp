#include "mesh.h"
#include "device.h"
#include "shader.h"
#include "image.h"
#include "imageloader.h"
#include "../utils/scopedtimelog.h"

#include <algorithm>
#include <iostream>

const uint32_t SET_ID_CAMERA = 0;
const uint32_t BINDING_ID_CAMERA = 0;
const uint32_t SET_ID_MATERIAL = 1;
const uint32_t BINDING_ID_MATERIAL = 0;
const uint32_t BINDING_ID_TEXTURE_DIFFUSE = 1;


Mesh::Mesh(Device& device)
    : DeviceRef(device)
    , m_vertexBuffer(device)
{
}

Mesh::~Mesh()
{
    m_shapes.clear();

    for (auto& desc : m_materials)
    {
        GraphicsPipeline::Release(device(), desc.pipeline);
        ShaderManager::Release(device(), desc.shader);
    }
    m_materials.clear();

    destroy(m_cameraDescriptorSetLayout);
    destroy(m_materialDescriptorSetLayout);
    destroy(m_pipelineLayout);
    destroy(m_materialDescriptorPool);
    destroy(m_cameraDescriptorPool);
    destroy(m_sampler);  
}

bool Mesh::init(const MeshDescription& meshDesc, VkBuffer cameraUniformBuffer, VkRenderPass renderPass)
{
    m_shapes = meshDesc.shapes;
    m_sampler = device().createSampler();

    if (!loadMaterials(meshDesc.materials))
        return false;

    createVertexBuffer(meshDesc.geometry);
    createDescriptors(cameraUniformBuffer);
    if (!createPipelines(renderPass))
        return false;

    return true;
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

        desc.material = UniformBuffer(device(), uboSize);
        desc.material.fill([&](auto data) {
            std::memcpy(data, &material.ambient, 3 * sizeof(float));
            std::memcpy(static_cast<char*>(data) + 4 * sizeof(float), &material.diffuse, 3 * sizeof(float));
            std::memcpy(static_cast<char*>(data) + 8 * sizeof(float), &material.emission, 3 * sizeof(float));
        });

        desc.descriptorSet.setUniformBuffer(BINDING_ID_MATERIAL, desc.material);

        if (!material.textureFilename.empty())
        {
            desc.diffuseTexture = ImageLoader::load(device(), material.textureFilename);
            if (desc.diffuseTexture)
            {
                desc.descriptorSet.setImageSampler(BINDING_ID_TEXTURE_DIFFUSE, desc.diffuseTexture.imageView(), m_sampler);
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

    return ShaderManager::Acquire(device(), ShaderResourceHandler::ShaderModulesDescription
        { { VK_SHADER_STAGE_VERTEX_BIT, vertexShaderFilename },
          { VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderFilename} });
}

void Mesh::createVertexBuffer(const MeshDescription::Geometry& geometry)
{
    assert(!geometry.vertexAttribs.empty() || !geometry.interleavedVertexAttribs.empty());

    if (!geometry.vertexAttribs.empty())
    {
        m_vertexBuffer.createFromSeparateAttributes(geometry.vertexAttribs);
    }
    else
    {
        m_vertexBuffer.createFromInterleavedAttributes(geometry.vertexCount, geometry.vertexSize * sizeof(float), const_cast<float*>(geometry.vertices.data()), geometry.interleavedVertexAttribs);
    }

    m_vertexBuffer.setIndices(geometry.indices.data(), static_cast<uint32_t>(geometry.indices.size()));
}

void Mesh::createDescriptors(VkBuffer cameraUniformBuffer)
{
    m_cameraUniformDescriptorSet.setUniformBuffer(BINDING_ID_CAMERA, cameraUniformBuffer);

    m_cameraDescriptorPool = device().createDescriptorPool(1, { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 } });

    m_cameraDescriptorSetLayout = device().createDescriptorSetLayout({ { BINDING_ID_CAMERA, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT } });

    m_cameraUniformDescriptorSet.allocateAndUpdate(device(), m_cameraDescriptorSetLayout, m_cameraDescriptorPool);

    const auto materialDescriptorCount = static_cast<uint32_t>(m_materials.size());

    m_materialDescriptorPool = device().createDescriptorPool(materialDescriptorCount,
        { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, materialDescriptorCount },
          { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, materialDescriptorCount } });

    m_materialDescriptorSetLayout = device().createDescriptorSetLayout( 
        { { BINDING_ID_MATERIAL, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },
          { BINDING_ID_TEXTURE_DIFFUSE, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT } });
}

bool Mesh::createPipelines(VkRenderPass renderPass)
{
    m_pipelineLayout = device().createPipelineLayout({ m_cameraDescriptorSetLayout, m_materialDescriptorSetLayout });

    for (auto& desc : m_materials)
    {
        desc.descriptorSet.allocateAndUpdate(device(), m_materialDescriptorSetLayout, m_materialDescriptorPool);

        auto isTransparent = desc.diffuseTexture && desc.diffuseTexture.transpareny();

        GraphicsPipelineSettings settings;
        settings.setCullMode(isTransparent ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT);
        if (isTransparent)
            settings.setAlphaBlending(
                VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ZERO);

        desc.pipeline = GraphicsPipeline::Acquire(device(),
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

    m_vertexBuffer.bind(commandBuffer);

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

        m_vertexBuffer.drawIndexed(commandBuffer, 1, shape.startIndex, shape.indexCount);
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

uint32_t Mesh::numShapes() const
{
    return static_cast<uint32_t>(m_shapes.size());
}
