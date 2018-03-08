#include "renderer.h"
#include "vulkanhelper.h"
#include "shader.h"

#include <array>

const uint32_t SET_ID_CAMERA = 0;
const uint32_t BINDING_ID_CAMERA = 0;

const uint32_t SET_ID_GROUND = 1;
const uint32_t BINDING_ID_GROUND = 0;

const uint32_t TILES_PER_DIM = 80;
const uint32_t NUM_TILES = TILES_PER_DIM * TILES_PER_DIM;

bool Renderer::setup()
{
    m_shader = Shader::getShader(m_device.getVkDevice(), { { VK_SHADER_STAGE_VERTEX_BIT, "data/shaders/base.vert.spv"} ,
                                                           { VK_SHADER_STAGE_FRAGMENT_BIT, "data/shaders/color.frag.spv"} });
    if (m_shader == nullptr)
        return false;

    m_descriptorPool.init(m_device.getVkDevice(), 2, { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 } });

    setupCameraDescriptorSet();
    setupCubeVertexBuffer();
    setupGraphicsPipeline();

    const auto size = static_cast<float>(TILES_PER_DIM / 2);
    glm::vec3 min(-size, 0.f, -size);
    glm::vec3 max( size, 0.f,  size);
    setCameraFromBoundingBox(min, max);

    return true;
}

void Renderer::setupCameraDescriptorSet()
{
    m_cameraDescriptorSetLayout.init(m_device.getVkDevice(),
        { { BINDING_ID_CAMERA, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT } });

    m_cameraUniformDescriptorSet.addUniformBuffer(BINDING_ID_CAMERA, m_cameraUniformBuffer.getVkBuffer());
    m_cameraUniformDescriptorSet.finalize(m_device.getVkDevice(), m_cameraDescriptorSetLayout, m_descriptorPool);
}

void Renderer::setupCubeVertexBuffer()
{
    const std::array<float, 24> vertices{
        // front
        -1.0, -1.0,  1.0,
        1.0, -1.0,  1.0,
        1.0,  1.0,  1.0,
        -1.0,  1.0,  1.0,
        // back
        -1.0, -1.0, -1.0,
        1.0, -1.0, -1.0,
        1.0,  1.0, -1.0,
        -1.0,  1.0, -1.0
    };

    const std::array<uint16_t, 36> indices{
        0, 1, 2,
        2, 3, 0,
        // top
        1, 5, 6,
        6, 2, 1,
        // back
        7, 6, 5,
        5, 4, 7,
        // bottom
        4, 0, 3,
        3, 7, 4,
        // left
        4, 5, 1,
        1, 0, 4,
        // right
        3, 2, 6,
        6, 7, 3
    };

    std::vector<float> groundHeight(NUM_TILES);
    for (uint32_t i = 0; i < NUM_TILES; i++)
    {
        auto x = static_cast<float>(i) / TILES_PER_DIM;
        auto y = static_cast<float>(i % TILES_PER_DIM);
        auto dx = x - (TILES_PER_DIM / 2);
        auto dy = y - (TILES_PER_DIM / 2);
        auto d = sqrt(dx * dx + dy * dy);
        groundHeight[i] = sin(d * 0.25f) / (d * 0.005f);
    }

    const std::vector<VertexBuffer::AttributeDescription> vertexDesc{
        { 0, 3, 8, &vertices.front(), 0 },
        { 1, 1, NUM_TILES, &groundHeight.front(), 0, true }
    };

    m_vertexBuffer.createFromSeparateAttributes(&m_device, vertexDesc);
    m_vertexBuffer.setIndices(indices.data(), static_cast<uint32_t>(indices.size()));
}

void Renderer::setupGraphicsPipeline()
{
    m_graphicsPipelineLayout.init(m_device.getVkDevice(), { m_cameraDescriptorSetLayout.getVkLayout() });

    PipelineSettings settings;
    settings.setCullMode(VK_CULL_MODE_BACK_BIT).setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    m_graphicsPipeline = GraphicsPipeline::getPipeline(m_device.getVkDevice(),
        m_renderPass.getVkRenderPass(),
        m_graphicsPipelineLayout.getVkPipelineLayout(),
        settings,
        m_shader->getShaderStages(),
        &m_vertexBuffer);
}

void Renderer::shutdown()
{
    m_vertexBuffer.destroy();
    m_graphicsPipelineLayout.destroy();

    m_cameraDescriptorSetLayout.destroy(m_device.getVkDevice());
    m_descriptorPool.destroy(m_device.getVkDevice());

    GraphicsPipeline::release(m_graphicsPipeline);
    Shader::release(m_shader);
}

void Renderer::renderGround(VkCommandBuffer commandBuffer) const
{
    m_cameraUniformDescriptorSet.bind(commandBuffer, m_pipelineLayout.getVkPipelineLayout(), SET_ID_CAMERA);

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

void Renderer::render(uint32_t imageId)
{
    // Submit graphics commands
    submitCommandBuffer(m_commandBuffers[imageId]);
}