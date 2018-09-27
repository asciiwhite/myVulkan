#include "simplerenderer.h"
#include "vulkanhelper.h"
#include "imgui.h"

#include <array>

bool SimpleRenderer::setup()
{
    if (!m_mesh.loadFromObj(m_device, "data/meshes/bunny.obj"))
        return false;

    if (!m_mesh.finalize(m_renderPass, m_cameraUniformBuffer))
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

void SimpleRenderer::fillCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_swapChain.getImageExtent();
    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = { 0.5f, 0.5f, 0.5f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(m_swapChain.getImageExtent().width), static_cast<float>(m_swapChain.getImageExtent().height), 0.0f, 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = { {0, 0}, m_swapChain.getImageExtent() };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    m_mesh.render(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}

void SimpleRenderer::render(const FrameData& frameData)
{
    fillCommandBuffer(frameData.resources.graphicsCommandBuffer, frameData.framebuffer);
    submitCommandBuffer(frameData.resources.graphicsCommandBuffer, m_swapChain.getImageAvailableSemaphore(), nullptr, nullptr);
}

void SimpleRenderer::createGUIContent()
{
    const auto baseFileName = m_mesh.fileName().substr(m_mesh.fileName().find_last_of("/\\") + 1, m_mesh.fileName().size());

    ImGui::Begin("Mesh loader", nullptr, ImGuiWindowFlags_NoResize);
    ImGui::Text("File: %s", baseFileName.c_str());
    ImGui::Text("#vertices: %u", m_mesh.numVertices());
    ImGui::Text("#indices: %u", m_mesh.numIndices());
    ImGui::End();
}