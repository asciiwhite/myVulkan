#include "texturearraymesh.h"
#include "device.h"
#include "../utils/scopedtimelog.h"

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
    m_materialDescsImageIds.clear();
    m_texturesDescriptorSetLayout.destroy(*m_device);
    m_mainDescriptorPool.destroy(*m_device);
    m_imageViews.clear();

    Mesh::destroy();
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
    m_materialDescsImageIds.resize(m_materials.size(), ~0u);

    m_materialDescs.resize(m_materials.size());
    auto materialDescIter = m_materialDescs.begin();
    
    uint32_t materialId = 0;
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
                m_materialDescsImageIds[materialId] = static_cast<uint32_t>(m_imageViews.size());
                m_imageViews.push_back(texture.imageView);
            }
        }

        desc.shader = selectShaderFromAttributes(desc.diffuseTexture);

        materialId++;
    }
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

        const auto imageId = m_materialDescsImageIds[shapeDesc.materialId];
        if (imageId != ~0u)
        {
            vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), static_cast<const void*>(&imageId));
        }

        materialDesc.descriptorSet.bind(commandBuffer, m_pipelineLayout, SET_ID_MATERIAL);

        m_vertexBuffer.draw(commandBuffer, 1, shapeDesc.startIndex, shapeDesc.indexCount);
    }
}
