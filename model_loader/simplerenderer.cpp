#include "simplerenderer.h"
#include "vulkanhelper.h"

#include <array>

bool SimpleRenderer::setup()
{
    if (!m_mesh.loadFromObj(m_device, "data/meshes/bunny.obj"))
        return false;

    m_mesh.addCameraUniformBuffer(m_cameraUniformBuffer.getVkBuffer());
    if (!m_mesh.finalize(m_renderPass))
        return false;

    glm::vec3 min, max;
    m_mesh.getBoundingbox(min, max);
    setCameraFromBoundingBox(min, max, glm::vec3(0,1,1));

    return true;
}

void SimpleRenderer::shutdown()
{
    m_mesh.destroy();
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
        clearValues[0].color = { 0.5f, 0.5f, 0.5f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(m_swapChain.getImageExtent().width), static_cast<float>(m_swapChain.getImageExtent().height), 0.0f, 1.0f };
        vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);

        VkRect2D scissor = { {0, 0}, m_swapChain.getImageExtent() };
        vkCmdSetScissor(m_commandBuffers[i], 0, 1, &scissor);

        m_mesh.render(m_commandBuffers[i]);

        vkCmdEndRenderPass(m_commandBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffers[i]));
    }
}

void SimpleRenderer::render(uint32_t imageId)
{
    submitCommandBuffer(m_commandBuffers[imageId]);
}
