#include "gui.h"
#include "device.h"
#include "graphicspipeline.h"
#include "statistics.h"
#include "mouseinputhandler.h"
#include "vulkanhelper.h"

#include <imgui.h>
#include <array>

const static uint32_t GUI_PARAMETER_SET_ID = 0;
const static uint32_t GUI_PARAMETER_BINDING_ID = 0;

GUI::GUI(Device &device)
: m_device(device)
{
}

GUI::~GUI()
{
    if (ImGui::GetCurrentContext())
        ImGui::DestroyContext();

    m_device.destroySampler(m_resources.sampler);
    m_device.destroyTexture(m_resources.image);
    m_device.destroyRenderPass(m_resources.renderPass);
    m_device.destroyPipelineLayout(m_resources.pipelineLayout);
    m_resources.descriptorPool.destroy(m_device);
    m_resources.descriptorSetLayout.destroy(m_device);

    GraphicsPipeline::Release(m_device, m_resources.pipeline);
    ShaderManager::Release(m_device, m_resources.shader);
    
    for (auto& resources : m_resources.frameResources)
    {
        m_device.destroyBuffer(resources.indices.buffer);
        m_device.destroyBuffer(resources.vertices.buffer);
    }
}

void GUI::setup(size_t resource_count, uint32_t width, uint32_t height, VkFormat colorAttachmentFormat, VkFormat swapChainDepthBufferFormat)
{
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiStyle &gui_style = ImGui::GetStyle();
    gui_style.Colors[ImGuiCol_TitleBg] = ImVec4( 0.16f, 0.29f, 0.48f, 0.9f );
    gui_style.Colors[ImGuiCol_TitleBgActive] = ImVec4( 0.16f, 0.29f, 0.48f, 0.9f );
    gui_style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.06f, 0.07f, 0.08f, 0.8f );
    gui_style.Colors[ImGuiCol_PlotHistogram] = ImVec4( 0.20f, 0.40f, 0.60f, 1.0f );
    gui_style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 0.20f, 0.45f, 0.90f, 1.0f );

    onResize(width, height);

    m_resources.frameResources.resize(resource_count);

    static const std::array<RenderPassAttachmentData, 2> attachmentData{ {
        { colorAttachmentFormat, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },
        { swapChainDepthBufferFormat, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL } } };

    m_resources.renderPass = m_device.createRenderPass(attachmentData);

    createTexture();
    createDescriptorResources();
    createGraphicsPipeline();
}

void GUI::onResize(uint32_t width, uint32_t height)
{
    ImGui::GetIO().DisplaySize.x = static_cast<float>(width);
    ImGui::GetIO().DisplaySize.y = static_cast<float>(height);
}

void GUI::startFrame(const Statistics& stats, const MouseInputState& mouseState)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = stats.getDeltaTime();
    io.Framerate = stats.getAverageFPS();
    io.MouseDown[0] = mouseState.buttonIsPressed[0];
    io.MouseDown[1] = mouseState.buttonIsPressed[2];
    io.MousePos = ImVec2(mouseState.position[0], mouseState.position[1]);

    ImGui::NewFrame();

    ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x - 120.0f, 20.0f ) );
    ImGui::SetNextWindowSize( ImVec2( 100.0f, 100.0 ) );
    ImGui::Begin( "Stats", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar );

    static bool showFps = false;

    if( ImGui::RadioButton( "FPS", showFps ) )
        showFps = true;
    ImGui::SameLine();
    if( ImGui::RadioButton( "ms", !showFps ) )
        showFps = false;

    if (showFps)
    {
        ImGui::SetCursorPosX( 20.0f );
        ImGui::Text( "%7.1f", stats.getAverageFPS() );

        auto & histogram = stats.getFPSHistogram();
        ImGui::PlotHistogram( "", histogram.data(), static_cast<int>(histogram.size()), 0, nullptr, 0.0f, FLT_MAX, ImVec2( 85.0f, 30.0f ) );
    }
    else
    {
        ImGui::SetCursorPosX( 20.0f );
        ImGui::Text( "%9.3f", stats.getAverageDeltaTime() );

        auto & histogram = stats.getDeltaTimeHistogram();
        ImGui::PlotHistogram( "", histogram.data(), static_cast<int>(histogram.size()), 0, nullptr, 0.0f, FLT_MAX, ImVec2( 85.0f, 30.0f ) );
    }

    ImGui::End();
}

void GUI::draw(uint32_t resource_index, VkCommandBuffer commandBuffer, VkFramebuffer framebuffer)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_resources.renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { static_cast<uint32_t>(ImGui::GetIO().DisplaySize.x), static_cast<uint32_t>(ImGui::GetIO().DisplaySize.y) };
    renderPassInfo.clearValueCount = 0;
    renderPassInfo.pClearValues = nullptr;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = { 0.0f, 0.0f, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    m_resources.descriptorSet.bind(commandBuffer, m_resources.pipelineLayout, GUI_PARAMETER_SET_ID);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resources.pipeline);

    drawFrameData(commandBuffer, m_resources.frameResources[resource_index]);

    vkCmdEndRenderPass(commandBuffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}

void GUI::resizeBufferIfNecessary(GUIResources::FrameResources::BufferParam& bufferParam, VkBufferUsageFlagBits usageFlags, int entryCount, int sizeOfEntry) const
{
    if (bufferParam.count < entryCount)
    {
        m_device.destroyBuffer(bufferParam.buffer);

        const auto bufferSize = sizeOfEntry * entryCount;
        bufferParam.buffer = m_device.createBuffer(static_cast<uint32_t>(bufferSize), usageFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        bufferParam.data = m_device.mapBuffer(bufferParam.buffer, bufferSize, 0);
        bufferParam.count = entryCount;
    }
}

void GUI::drawFrameData(VkCommandBuffer commandBuffer, GUIResources::FrameResources& frameResources)
{
    ImGui::Render();

    const auto drawData = ImGui::GetDrawData();
    if( drawData->TotalVtxCount == 0 )
        return;

    resizeBufferIfNecessary(frameResources.vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, drawData->TotalVtxCount, sizeof(ImDrawVert));
    resizeBufferIfNecessary(frameResources.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, drawData->TotalIdxCount, sizeof(ImDrawIdx));

    auto vertexPointer = reinterpret_cast<ImDrawVert*>(frameResources.vertices.data);
    auto indexPointer = reinterpret_cast<ImDrawIdx*>(frameResources.indices.data);

    for( int i = 0; i < drawData->CmdListsCount; i++ )
    {
        const auto cmdList = drawData->CmdLists[i];

        memcpy(vertexPointer, cmdList->VtxBuffer.Data, sizeof(ImDrawVert) * cmdList->VtxBuffer.Size );
        memcpy(indexPointer, cmdList->IdxBuffer.Data, sizeof(ImDrawIdx) * cmdList->IdxBuffer.Size );

        vertexPointer += cmdList->VtxBuffer.Size;
        indexPointer += cmdList->IdxBuffer.Size;
    }

    // only flush the copied area
    const std::vector<VkMappedMemoryRange> memoryRanges {
        { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, frameResources.vertices.buffer.memory, 0, VK_WHOLE_SIZE },
        { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, frameResources.indices.buffer.memory, 0, VK_WHOLE_SIZE }
    };

    vkFlushMappedMemoryRanges(m_device, 2, memoryRanges.data());

    // Bind vertex and index buffers
    VkBuffer buffer{ frameResources.vertices.buffer };
    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, frameResources.indices.buffer, 0, VK_INDEX_TYPE_UINT16);

    // Setup scale and translation: xy scale, xy translation
    const std::vector<float> scale_and_translation{ 2.0f / ImGui::GetIO().DisplaySize.x, 2.0f / ImGui::GetIO().DisplaySize.y, -1.0f, -1.0f };
    vkCmdPushConstants(commandBuffer, m_resources.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * static_cast<uint32_t>(scale_and_translation.size()), scale_and_translation.data());

    // Render GUI
    int vertexOffset = 0;
    int indexOffset = 0;
    for( int list = 0; list < drawData->CmdListsCount; list++ )
    {
        const auto cmdList = drawData->CmdLists[list];
        for (int command_index = 0; command_index < cmdList->CmdBuffer.Size; command_index++)
        {
            const auto drawCommand = &cmdList->CmdBuffer[command_index];

            const VkRect2D scissor{
                { drawCommand->ClipRect.x > 0 ? static_cast<int32_t>(drawCommand->ClipRect.x) : 0,
                  drawCommand->ClipRect.y > 0 ? static_cast<int32_t>(drawCommand->ClipRect.y) : 0 },
                { static_cast<uint32_t>(drawCommand->ClipRect.z - drawCommand->ClipRect.x),
                  static_cast<uint32_t>(drawCommand->ClipRect.w - drawCommand->ClipRect.y) } };

            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            vkCmdDrawIndexed(commandBuffer, drawCommand->ElemCount, 1, indexOffset, vertexOffset, 0 );
            indexOffset += drawCommand->ElemCount;
        }
        vertexOffset += cmdList->VtxBuffer.Size;
    }
}

void GUI::createTexture()
{
    int w = 0, h = 0;
    unsigned char* pixels = nullptr;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32( &pixels, &w, &h );

    m_resources.image = m_device.createImageFromData(static_cast<uint32_t>(w), static_cast<uint32_t>(h), pixels, VK_FORMAT_R8G8B8A8_UNORM);
    m_resources.sampler = m_device.createSampler(); // maybe use clamp to edge 
}

void GUI::createDescriptorResources()
{
    m_resources.descriptorPool.init(m_device,
        1, { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 } });

    m_resources.descriptorSetLayout.init(m_device,
        { { GUI_PARAMETER_BINDING_ID, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT } });

    m_resources.descriptorSet.setImageSampler(GUI_PARAMETER_BINDING_ID, m_resources.image.imageView, m_resources.sampler);
    m_resources.descriptorSet.allocateAndUpdate(m_device, m_resources.descriptorSetLayout, m_resources.descriptorPool);
}

bool GUI::createGraphicsPipeline()
{
    const auto shaderDesc = ShaderResourceHandler::ShaderModulesDescription{
        { VK_SHADER_STAGE_VERTEX_BIT,  "data/shaders/gui.vert.spv" },
        { VK_SHADER_STAGE_FRAGMENT_BIT, "data/shaders/gui.frag.spv" } };

    m_resources.shader = ShaderManager::Acquire(m_device, shaderDesc);
    if (!m_resources.shader)
        return false;

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 4;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    m_resources.pipelineLayout = m_device.createPipelineLayout({ m_resources.descriptorSetLayout }, { pushConstantRange });

    GraphicsPipelineSettings settings;
    settings.setAlphaBlending(true).setDepthTesting(false);

    const std::vector<VkVertexInputBindingDescription> vertexBindingDesc{ {0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX } };
    const std::vector<VkVertexInputAttributeDescription> vertexAttributeDesc{
        { 0, vertexBindingDesc[0].binding, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos) },
        { 1, vertexBindingDesc[0].binding, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv) },
        { 2, vertexBindingDesc[0].binding, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col) } };

    m_resources.pipeline = GraphicsPipeline::Acquire(m_device,
        m_resources.renderPass,
        m_resources.pipelineLayout,
        settings,
        m_resources.shader.shaderStageCreateInfos,
        vertexAttributeDesc,
        vertexBindingDesc);

    return true;
}
