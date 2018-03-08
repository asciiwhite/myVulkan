#include "renderer.h"
#include "vulkanhelper.h"
#include "shader.h"

#include <array>

const uint32_t SET_ID_CAMERA = 0;
const uint32_t BINDING_ID_CAMERA = 0;

const uint32_t SET_ID_GROUND = 1;
const uint32_t BINDING_ID_GROUND = 0;

const uint32_t BINDING_ID_COMPUTE_HEIGHT = 0;
const uint32_t BINDING_ID_COMPUTE_INPUT = 1;

const uint32_t TILES_PER_DIM = 80;
const uint32_t NUM_TILES = TILES_PER_DIM * TILES_PER_DIM;

const uint32_t HEIGHT_OFFSET = 3 * 4 * 8;
const uint32_t HEIGHT_SIZE = NUM_TILES * 4;

const uint32_t WORKGROUP_SIZE = 32;
const uint32_t GROUP_COUNT_X = static_cast<uint32_t>(ceil(static_cast<float>(TILES_PER_DIM) / WORKGROUP_SIZE));
const uint32_t GROUP_COUNT_Y = GROUP_COUNT_X;

bool Renderer::setup()
{
    m_shader = Shader::getShader(m_device.getVkDevice(), { { VK_SHADER_STAGE_VERTEX_BIT, "data/shaders/base.vert.spv"} ,
                                                           { VK_SHADER_STAGE_FRAGMENT_BIT, "data/shaders/color.frag.spv"} });
    if (m_shader == nullptr)
        return false;

    m_computeShader = Shader::getShader(m_device.getVkDevice(), { { VK_SHADER_STAGE_COMPUTE_BIT, "data/shaders/height.comp.spv"} });
    if (m_computeShader == nullptr)
        return false;

    m_descriptorPool.init(m_device.getVkDevice(), 2,
        { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
          { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } });

    setupCameraDescriptorSet();
    setupCubeVertexBuffer();
    setupGraphicsPipeline();
    setupComputePipeline();
    buildComputeCommandBuffer();

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

    std::vector<float> groundHeight(NUM_TILES, 1.f);
 
    const std::vector<VertexBuffer::AttributeDescription> vertexDesc{
        { 0, 3, 8, &vertices.front(), 0 },
        { 1, 1, NUM_TILES, &groundHeight.front(), 0, true }
    };

    m_vertexBuffer.createFromSeparateAttributes(&m_device, vertexDesc, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
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

void Renderer::setupComputePipeline()
{
    m_computeInputBuffer.create(m_device, sizeof(ComputeInput), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    m_computeMappedInputBuffer = static_cast<ComputeInput*>(m_computeInputBuffer.map(m_device));
    m_computeMappedInputBuffer->tilesPerDim = TILES_PER_DIM;
    m_computeMappedInputBuffer->time = 0.f;

    m_computeDescriptorSetLayout.init(m_device.getVkDevice(),
        { { BINDING_ID_COMPUTE_HEIGHT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
          { BINDING_ID_COMPUTE_INPUT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT } });

    m_computeDescriptorSet.addStorageBuffer(BINDING_ID_COMPUTE_HEIGHT, m_vertexBuffer.getVkBuffer(), HEIGHT_OFFSET, HEIGHT_SIZE);
    m_computeDescriptorSet.addUniformBuffer(BINDING_ID_COMPUTE_INPUT, m_computeInputBuffer.getVkBuffer());
    m_computeDescriptorSet.finalize(m_device.getVkDevice(), m_computeDescriptorSetLayout, m_descriptorPool);

    m_computePipelineLayout.init(m_device.getVkDevice(), { m_computeDescriptorSetLayout.getVkLayout() });

    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.flags = 0;
    pipelineInfo.stage = m_computeShader->getShaderStages().front();
    pipelineInfo.layout = m_computePipelineLayout.getVkPipelineLayout();
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = 0;

    VK_CHECK_RESULT(vkCreateComputePipelines(m_device.getVkDevice(), nullptr, 1, &pipelineInfo, nullptr, &m_computePipeline));

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK_RESULT(vkCreateFence(m_device.getVkDevice(), &fenceCreateInfo, nullptr, &m_computeFence));
}

void Renderer::shutdown()
{
    m_vertexBuffer.destroy();
    m_graphicsPipelineLayout.destroy();

    m_cameraDescriptorSetLayout.destroy(m_device.getVkDevice());
    m_descriptorPool.destroy(m_device.getVkDevice());

    vkFreeCommandBuffers(m_device.getVkDevice(), m_device.getComputeCommandPool(), static_cast<uint32_t>(m_computeCommandBuffers.size()), m_computeCommandBuffers.data());

    m_computeInputBuffer.destroy(m_device);
    m_computeDescriptorSetLayout.destroy(m_device.getVkDevice());
    vkDestroyPipeline(m_device.getVkDevice(), m_computePipeline, nullptr);
    m_computePipelineLayout.destroy();

    vkDestroyFence(m_device.getVkDevice(), m_computeFence, nullptr);

    GraphicsPipeline::release(m_graphicsPipeline);
    Shader::release(m_shader);
    Shader::release(m_computeShader);
}

void Renderer::renderGround(VkCommandBuffer commandBuffer) const
{
    m_cameraUniformDescriptorSet.bind(commandBuffer, m_graphicsPipelineLayout.getVkPipelineLayout(), SET_ID_CAMERA);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->getVkPipeline());

    m_vertexBuffer.draw(commandBuffer, NUM_TILES);
}

void Renderer::buildComputeCommandBuffer()
{
    // Create a command buffer for compute operations
    m_computeCommandBuffers.resize(m_swapChain.getImageCount());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_device.getComputeCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_computeCommandBuffers.size());
    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device.getVkDevice(), &allocInfo, m_computeCommandBuffers.data()));

    for (size_t i = 0; i < m_computeCommandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VK_CHECK_RESULT(vkBeginCommandBuffer(m_computeCommandBuffers[i], &beginInfo));

        // Add memory barrier to ensure that the (graphics) vertex shader has fetched attributes before compute starts to write to the buffer
        VkBufferMemoryBarrier bufferBarrier = {};
        bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufferBarrier.buffer = m_vertexBuffer.getVkBuffer();
        bufferBarrier.size = HEIGHT_SIZE;
        bufferBarrier.offset = HEIGHT_OFFSET;
        bufferBarrier.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;						// Vertex shader invocations have finished reading from the buffer
        bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;								// Compute shader wants to write to the buffer
                                                                                                // Compute and graphics queue may have different queue families (see VulkanDevice::createLogicalDevice)
                                                                                                // For the barrier to work across different queues, we need to set their family indices
        bufferBarrier.srcQueueFamilyIndex = m_device.getGraphicsQueueFamilyId();		        // Required as compute and graphics queue may have different families
        bufferBarrier.dstQueueFamilyIndex = m_device.getComputeQueueFamilyId();	            	// Required as compute and graphics queue may have different families

        vkCmdPipelineBarrier(
            m_computeCommandBuffers[i],
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0,
            0, nullptr,
            1, &bufferBarrier,
            0, nullptr);

        vkCmdBindPipeline(m_computeCommandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
        VkDescriptorSet descriptorSets{ m_computeDescriptorSet.getVkDescriptorSet() };
        vkCmdBindDescriptorSets(m_computeCommandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout.getVkPipelineLayout(), 0, 1, &descriptorSets, 0, 0);

        vkCmdDispatch(m_computeCommandBuffers[i], GROUP_COUNT_X, GROUP_COUNT_Y, 1);

        // Add memory barrier to ensure that compute shader has finished writing to the buffer
        // Without this the (rendering) vertex shader may display incomplete results (partial data from last frame) 
        bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;								// Compute shader has finished writes to the buffer
        bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;						// Vertex shader invocations want to read from the buffer
        // Compute and graphics queue may have different queue families (see VulkanDevice::createLogicalDevice)
        // For the barrier to work across different queues, we need to set their family indices
        bufferBarrier.srcQueueFamilyIndex = m_device.getComputeQueueFamilyId();				    // Required as compute and graphics queue may have different families
        bufferBarrier.dstQueueFamilyIndex = m_device.getGraphicsQueueFamilyId();     		    // Required as compute and graphics queue may have different families

        vkCmdPipelineBarrier(
            m_computeCommandBuffers[i],
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            0,
            0, nullptr,
            1, &bufferBarrier,
            0, nullptr);

        vkEndCommandBuffer(m_computeCommandBuffers[i]);
    }
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
    m_computeMappedInputBuffer->time += 2.f * static_cast<float>(m_stats.getLastFrameTime()) / 1000000;

    // Submit graphics commands
    submitCommandBuffer(m_commandBuffers[imageId]);

    // Submit compute commands
    vkWaitForFences(m_device.getVkDevice(), 1, &m_computeFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device.getVkDevice(), 1, &m_computeFence);

    VkSubmitInfo computeSubmitInfo = {};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &m_computeCommandBuffers[imageId];

    VK_CHECK_RESULT(vkQueueSubmit(m_device.getComputeQueue(), 1, &computeSubmitInfo, m_computeFence));
}