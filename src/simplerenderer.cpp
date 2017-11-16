#include "simplerenderer.h"
#include "utils/timer.h"
#include "vulkan/vulkanhelper.h"

#pragma warning(push)
#pragma warning(disable:4201)
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

#include <cstring>
#include <cmath>
#include <array>

bool SimpleRenderer::setup()
{
    m_shader.createFromFiles(m_device.getVkDevice(), "data/shaders/color.vert.spv", "data/shaders/color.frag.spv");

    m_device.createSampler(m_sampler);

    m_device.createBuffer(sizeof(glm::mat4),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_uniformBuffer, m_uniformBufferMemory);

    updateMVP();

    m_descriptorSet.addUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, m_uniformBuffer);
    m_descriptorSet.finalize(m_device.getVkDevice());

    m_pipelineLayout.init(m_device.getVkDevice(), { m_descriptorSet.getLayout() });
    
    const std::string meshFilename = "data/meshes/bunny.obj";
    const std::string materialBaseDir = meshFilename.substr(0, meshFilename.find_last_of("/\\") + 1);
    if (!m_mesh.loadFromObj(m_device, meshFilename, materialBaseDir))
        return false;

    PipelineSettings settings;

    m_pipeline.init(m_device.getVkDevice(),
        m_renderPass.getVkRenderPass(),
        m_pipelineLayout.getVkPipelineLayout(),
        settings,
        m_shader.getShaderStages(),
        &m_mesh.getVertexBuffer());

    return true;
}

void SimpleRenderer::shutdown()
{
    m_mesh.destroy();

    m_descriptorSet.destroy(m_device.getVkDevice());
    m_shader.destory();
    m_pipeline.destroy();
    m_pipelineLayout.destroy();
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
        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(m_swapChain.getImageExtent().width), static_cast<float>(m_swapChain.getImageExtent().height), 0.0f, 1.0f };
        vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);

        VkRect2D scissor = { {0, 0}, m_swapChain.getImageExtent() };
        vkCmdSetScissor(m_commandBuffers[i], 0, 1, &scissor);

        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.getVkPipeline());

        m_descriptorSet.bind(m_commandBuffers[i], m_pipelineLayout.getVkPipelineLayout());

        m_mesh.getVertexBuffer().draw(m_commandBuffers[i]);

        vkCmdEndRenderPass(m_commandBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffers[i]));
    }
}

void SimpleRenderer::update()
{
}

void SimpleRenderer::resized()
{
    updateMVP();
}

void SimpleRenderer::updateMVP()
{
    const glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 4.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    const glm::mat4 projection = glm::perspective(glm::radians(45.0f), m_swapChain.getImageExtent().width / (float)m_swapChain.getImageExtent().height, 0.1f, 10.0f);
    const glm::mat4 mvp = projection * view;
    const uint32_t bufferSize = sizeof(mvp);

    void* data;
    vkMapMemory(m_device.getVkDevice(), m_uniformBufferMemory, 0, bufferSize, 0, &data);
    std::memcpy(data, &mvp, bufferSize);
    vkUnmapMemory(m_device.getVkDevice(), m_uniformBufferMemory);
}
