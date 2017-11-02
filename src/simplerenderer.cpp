#include "simplerenderer.h"
#include "utils/timer.h"
#include "vulkan/vulkanhelper.h"

#include <cstring>
#include <cmath>

bool SimpleRenderer::setup()
{
    m_shader.createFromFiles(m_device.getVkDevice(), "data/shaders/simple.vert.spv", "data/shaders/simple.frag.spv");
    m_texture.loadFromFile(&m_device, "data/textures/vulkan.jpg");

    m_device.createSampler(m_sampler);

    const float scaling = 2.0f;
    const uint32_t bufferSize = sizeof(scaling);

    m_device.createBuffer(bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_uniformBuffer, m_uniformBufferMemory);

    void* data;
    vkMapMemory(m_device.getVkDevice(), m_uniformBufferMemory, 0, bufferSize, 0, &data);
    std::memcpy(data, &scaling, bufferSize);
    vkUnmapMemory(m_device.getVkDevice(), m_uniformBufferMemory);

    m_descriptorSet.addSampler(m_texture.getImageView(), m_sampler);
    m_descriptorSet.addUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, m_uniformBuffer);
    m_descriptorSet.finalize(m_device.getVkDevice());

    m_pipelineLayout.init(m_device.getVkDevice(), { m_descriptorSet.getLayout() });
    
    const float vertices[] = {
       -0.5, -0.5,
        0.5, -0.5,
        0.5,  0.5,
       -0.5,  0.5
    };

    const float texCoords[] = {
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0,
        0.0, 1.0
    };

    const float colors[] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f
    };

    const std::vector<VertexBuffer::AttributeDescription> attribDesc =
    {
        { 0, 2, 4, vertices },
        { 1, 2, 4, texCoords },
        { 2, 3, 4, colors }
    };

    const uint16_t indices[] = { 0, 1, 2, 2, 3, 0 };

    m_vertexBuffer.init(&m_device, attribDesc);
    m_vertexBuffer.setIndices(indices, 6);

    PipelineSettings settings;

    m_pipeline.init(m_device.getVkDevice(),
        m_renderPass.getVkRenderPass(),
        m_pipelineLayout.getVkPipelineLayout(),
        settings,
        m_shader.getShaderStages(),
        &m_vertexBuffer);

    return true;
}

void SimpleRenderer::shutdown()
{
    m_descriptorSet.destroy(m_device.getVkDevice());
    m_shader.destory();
    m_vertexBuffer.destroy();
    m_pipeline.destroy();
    m_pipelineLayout.destroy();
    m_texture.destroy();
    vkDestroySampler(m_device.getVkDevice(), m_sampler, nullptr);

    vkDestroyBuffer(m_device.getVkDevice(), m_uniformBuffer, nullptr);
    m_uniformBuffer = VK_NULL_HANDLE;
    vkFreeMemory(m_device.getVkDevice(), m_uniformBufferMemory, nullptr);
    m_uniformBufferMemory = VK_NULL_HANDLE;
}

void SimpleRenderer::fillCommandBuffers()
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
        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(m_swapChain.getImageExtent().width), static_cast<float>(m_swapChain.getImageExtent().height), 0.0f, 1.0f };
        vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);

        VkRect2D scissor = { {0, 0}, m_swapChain.getImageExtent() };
        vkCmdSetScissor(m_commandBuffers[i], 0, 1, &scissor);

        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.getVkPipeline());

        m_descriptorSet.bind(m_commandBuffers[i], m_pipelineLayout.getVkPipelineLayout());

        m_vertexBuffer.draw(m_commandBuffers[i]);

        vkCmdEndRenderPass(m_commandBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffers[i]));
    }
}

void SimpleRenderer::update()
{
    const auto time = static_cast<float>(Timer::getMicroseconds()) / 1000000;
    const auto scaling = cos(time) + 2.0f;
    const auto bufferSize(sizeof(scaling));
    void* data;
    vkMapMemory(m_device.getVkDevice(), m_uniformBufferMemory, 0, bufferSize, 0, &data);
    std::memcpy(data, &scaling, bufferSize);
    vkUnmapMemory(m_device.getVkDevice(), m_uniformBufferMemory);
}