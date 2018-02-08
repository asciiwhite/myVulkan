#include "renderer.h"
#include "vulkanhelper.h"
#include "shader.h"

#include <array>

const uint32_t SET_ID_CAMERA = 0;
const uint32_t BINDING_ID_CAMERA = 0;

const uint32_t SET_ID_GROUND = 1;
const uint32_t BINDING_ID_GROUND = 0;

const uint32_t TILES_PER_DIM = 10;
const uint32_t NUM_TILES = TILES_PER_DIM * TILES_PER_DIM;

bool Renderer::setup()
{
    m_shader = Shader::getShader(m_device.getVkDevice(), "data/shaders/base.vert.spv", "data/shaders/color.frag.spv");
    if (m_shader == nullptr)
        return false;

    m_descriptorPool.init(m_device.getVkDevice(), 2, { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 } });

    setupCamera();
    setupGroundHeightUniform();

    const std::vector<float> vertices{
        -1.f, 0.f, -1.f,
         1.f, 0.f, -1.f,
        -1.f, 0.f,  1.f,
         1.f, 0.f,  1.f,
    };

    const std::vector<uint16_t> indices { 0, 1, 2, 3 };

    const std::vector<VertexBuffer::AttributeDescription> vertexDesc{
        { 0, 3, 4, &vertices.front(), 0 }
    };

    m_vertexBuffer.createFromSeparateAttributes(&m_device, vertexDesc);
    m_vertexBuffer.setIndices(indices.data(), static_cast<uint32_t>(indices.size()));

    m_pipelineLayout.init(m_device.getVkDevice(), { m_cameraDescriptorSetLayout.getVkLayout(), m_groundDescriptorSetLayout.getVkLayout() });

    PipelineSettings settings;
    settings.setCullMode(VK_CULL_MODE_NONE).setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

    m_pipeline = Pipeline::getPipeline(m_device.getVkDevice(),
        m_renderPass.getVkRenderPass(),
        m_pipelineLayout.getVkPipelineLayout(),
        settings,
        m_shader->getShaderStages(),
        &m_vertexBuffer);

    glm::vec3 min(-10.f, 0.f, -10.f);
    glm::vec3 max( 10.f, 0.f,  10.f);
    setCameraFromBoundingBox(min, max);

    return true;
}

void Renderer::setupCamera()
{
    m_cameraDescriptorSetLayout.init(m_device.getVkDevice(),
        { { BINDING_ID_CAMERA, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT } });

    m_cameraUniformDescriptorSet.addUniformBuffer(BINDING_ID_CAMERA, m_cameraUniformBuffer.getVkBuffer());
    m_cameraUniformDescriptorSet.finalize(m_device.getVkDevice(), m_cameraDescriptorSetLayout, m_descriptorPool);
}

void Renderer::setupGroundHeightUniform()
{
    const uint32_t bufferSize = NUM_TILES * 4 * 4;

    m_groundUniformBuffer.create(m_device,
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data = m_groundUniformBuffer.map(m_device);
    auto height = static_cast<float*>(data);
    for (uint32_t i = 0; i < NUM_TILES; i++)
    {
        auto x = static_cast<float>(i) / TILES_PER_DIM;
        auto y = static_cast<float>(i % TILES_PER_DIM);
        auto d = abs(x - (TILES_PER_DIM / 2)) + abs(y - (TILES_PER_DIM / 2));
        *height = static_cast<float>(d*0.15f);
        height += 4;
    }
    m_groundUniformBuffer.unmap(m_device);

    m_groundDescriptorSetLayout.init(m_device.getVkDevice(),
        { { BINDING_ID_GROUND, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT } });

    m_groundUniformDescriptorSet.addUniformBuffer(BINDING_ID_GROUND, m_groundUniformBuffer.getVkBuffer());
    m_groundUniformDescriptorSet.finalize(m_device.getVkDevice(), m_groundDescriptorSetLayout, m_descriptorPool);
}

void Renderer::shutdown()
{
    m_vertexBuffer.destroy();
    m_pipelineLayout.destroy();

    m_groundUniformBuffer.destroy(m_device);
    m_groundDescriptorSetLayout.destroy(m_device.getVkDevice());
    m_cameraDescriptorSetLayout.destroy(m_device.getVkDevice());
    m_descriptorPool.destroy(m_device.getVkDevice());

    Pipeline::release(m_pipeline);
    Shader::release(m_shader);
}

void Renderer::renderGround(VkCommandBuffer commandBuffer) const
{
    m_cameraUniformDescriptorSet.bind(commandBuffer, m_pipelineLayout.getVkPipelineLayout(), SET_ID_CAMERA);
    m_groundUniformDescriptorSet.bind(commandBuffer, m_pipelineLayout.getVkPipelineLayout(), SET_ID_GROUND);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->getVkPipeline());

    m_vertexBuffer.draw(commandBuffer, NUM_TILES);
}

void Renderer::fillCommandBuffers()
{
    for (size_t i = 0; i < m_commandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VK_CHECK_RESULT(vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo));

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass.getVkRenderPass();
        renderPassInfo.framebuffer = m_framebuffers[i].getVkFramebuffer();
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapChain.getImageExtent();
        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = { 0.5f, 0.5f, 0.5f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(m_swapChain.getImageExtent().width), static_cast<float>(m_swapChain.getImageExtent().height), 0.0f, 1.0f };
        vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);

        VkRect2D scissor = { {0, 0}, m_swapChain.getImageExtent() };
        vkCmdSetScissor(m_commandBuffers[i], 0, 1, &scissor);

        renderGround(m_commandBuffers[i]);

        vkCmdEndRenderPass(m_commandBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffers[i]));
    }
}
