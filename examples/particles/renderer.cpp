#include "renderer.h"
#include "vulkanhelper.h"
#include "shader.h"
#include "barrier.h"
#include "imgui.h"

#include <random>
#include <array>

const uint32_t SET_ID_CAMERA = 0;
const uint32_t BINDING_ID_CAMERA = 0;

const uint32_t SET_ID_GROUND = 1;
const uint32_t BINDING_ID_GROUND = 0;

const uint32_t BINDING_ID_COMPUTE_PARTICLES = 0;
const uint32_t BINDING_ID_COMPUTE_INPUT = 1;

const uint32_t WORKGROUP_SIZE = 512;

bool Renderer::setup()
{
    m_shader = ShaderManager::Acquire(m_device, ShaderResourceHandler::ShaderModulesDescription
        { { VK_SHADER_STAGE_VERTEX_BIT, "data/shaders/base.vert.spv"} ,
          { VK_SHADER_STAGE_FRAGMENT_BIT, "data/shaders/color.frag.spv"} });
    if (!m_shader)
        return false;

    m_computeShader = ShaderManager::Acquire(m_device, ShaderResourceHandler::ShaderModulesDescription
        { { VK_SHADER_STAGE_COMPUTE_BIT, "data/shaders/particles.comp.spv"} });
    if (!m_computeShader)
        return false;

    m_descriptorPool = m_device.createDescriptorPool(2, 
        { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
          { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } });
    
    m_particleCount = static_cast<int>(m_particlesPerSecond * m_particleLifetimeInSeconds);
    m_groupCount = static_cast<uint32_t>(std::ceil(static_cast<float>(m_particleCount) / WORKGROUP_SIZE));

    setupCameraDescriptorSet();
    setupParticleVertexBuffer();
    setupGraphicsPipeline();
    setupComputePipeline();
    createComputeCommandBuffer();

    const auto size = 10.f;
    glm::vec3 min(-size, -size, 0.f);
    glm::vec3 max( size,  size, 0.f);
    setCameraFromBoundingBox(min, max, glm::vec3(0,0,1));

    return true;
}

void Renderer::setupCameraDescriptorSet()
{
    m_cameraDescriptorSetLayout = m_device.createDescriptorSetLayout({ { BINDING_ID_CAMERA, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT } });

    m_cameraUniformDescriptorSet.setUniformBuffer(BINDING_ID_CAMERA, m_cameraUniformBuffer);
    m_cameraUniformDescriptorSet.allocateAndUpdate(m_device, m_cameraDescriptorSetLayout, m_descriptorPool);
}

void Renderer::setupParticleVertexBuffer()
{
    struct ParticleData
    {
        glm::vec2 pos;
        glm::vec2 vel;
        glm::vec4 age;
    };

    std::vector<ParticleData> particles(m_particleCount);

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> ageDist(-1.0f, 0.0f);
    std::uniform_real_distribution<float> velDist(-1.0f, 1.0f);

    std::generate(particles.begin(), particles.end(), [&] { return ParticleData{ m_emitterPosition,{ velDist(mt),velDist(mt) },{ ageDist(mt),0,0,0 } }; });

    const std::vector<VertexBuffer::InterleavedAttributeDescription> vertexDesc{
        { 0, 2, offsetof(ParticleData, pos) },
        { 1, 4, offsetof(ParticleData, age) }
    }; 

    m_vertexBuffer.reset(new VertexBuffer(m_device));
    m_vertexBuffer->createFromInterleavedAttributes(m_particleCount, sizeof(ParticleData), &particles.front().pos.x, vertexDesc);
}

void Renderer::setupGraphicsPipeline()
{
    m_graphicsPipelineLayout = m_device.createPipelineLayout({ m_cameraDescriptorSetLayout });

    GraphicsPipelineSettings settings;
    settings.setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST).setDepthTesting(false);
     settings.setAlphaBlending(
            VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE);

    m_graphicsPipeline = GraphicsPipeline::Acquire(m_device,
        m_swapchainRenderPass,
        m_graphicsPipelineLayout,
        settings,
        m_shader.shaderStageCreateInfos,
        m_vertexBuffer->getAttributeDescriptions(),
        m_vertexBuffer->getBindingDescriptions());
}

void Renderer::setupComputePipeline()
{
    m_computeInputBuffer = UniformBuffer(m_device, sizeof(ComputeInput));
    m_computeMappedInputBuffer = static_cast<ComputeInput*>(m_computeInputBuffer.map());
    m_computeMappedInputBuffer->particleCount = m_particleCount;
    m_computeMappedInputBuffer->particleLifetimeInSeconds = m_particleLifetimeInSeconds;
    m_computeMappedInputBuffer->particleSpeed = m_particleSpeed;
    m_computeMappedInputBuffer->gravityForce = -0.0005f;
    m_computeMappedInputBuffer->collisionDamping = 0.7f;
    m_computeMappedInputBuffer->emitterPos = m_emitterPosition;
    m_computeMappedInputBuffer->timeDeltaInSeconds = 0.f;

    m_computeDescriptorSetLayout = m_device.createDescriptorSetLayout(
        { { BINDING_ID_COMPUTE_PARTICLES, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT },
          { BINDING_ID_COMPUTE_INPUT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT } });

    m_computeDescriptorSet.allocate(m_device, m_computeDescriptorSetLayout, m_descriptorPool);
    m_computeDescriptorSet.setStorageBuffer(BINDING_ID_COMPUTE_PARTICLES, *m_vertexBuffer);
    m_computeDescriptorSet.setBuffer(BINDING_ID_COMPUTE_INPUT, m_computeInputBuffer);
    m_computeDescriptorSet.update(m_device);

    m_computePipelineLayout = m_device.createPipelineLayout({ m_computeDescriptorSetLayout });

    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.flags = 0;
    pipelineInfo.stage = m_computeShader.shaderStageCreateInfos.front();
    pipelineInfo.layout = m_computePipelineLayout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = 0;

    VK_CHECK_RESULT(vkCreateComputePipelines(m_device, nullptr, 1, &pipelineInfo, nullptr, &m_computePipeline));
}

void Renderer::shutdown()
{
    m_vertexBuffer.reset();
    m_device.destroy(m_graphicsPipelineLayout);

    m_device.destroy(m_cameraDescriptorSetLayout);
    m_device.destroy(m_descriptorPool);

    m_computeCommandBuffers.clear();
    m_computeInputBuffer = UniformBuffer();
    m_device.destroy(m_computeDescriptorSetLayout);
    m_device.destroy(m_computePipeline);
    m_device.destroy(m_computePipelineLayout);

    GraphicsPipeline::Release(m_device, m_graphicsPipeline);
    ShaderManager::Release(m_device, m_shader);
    ShaderManager::Release(m_device, m_computeShader);
}

void Renderer::renderParticles(CommandBuffer& commandBuffer) const
{
    m_cameraUniformDescriptorSet.bind(commandBuffer, m_graphicsPipelineLayout, SET_ID_CAMERA);

    commandBuffer.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    m_vertexBuffer->bind(commandBuffer);
    m_vertexBuffer->draw(commandBuffer);
}

bool Renderer::createComputeCommandBuffer()
{
    // Create a command buffer for compute operations
    m_computeCommandBuffers.resize(m_frameResourceCount);
    for (auto& buffer : m_computeCommandBuffers)
        buffer = m_device.createComputeCommandBuffer();

    return true;
}
 
void Renderer::buildComputeCommandBuffer(CommandBuffer& commandBuffer)
{
    commandBuffer.begin();

    // Add memory barrier to ensure that the (graphics) vertex shader has fetched attributes before compute starts to write to the buffer
    auto bufferBarrier = createBufferMemoryBarrier(*m_vertexBuffer,
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,    // Vertex shader invocations have finished reading from the buffer
        VK_ACCESS_SHADER_WRITE_BIT,             // Compute shader wants to write to the buffer
        m_device.graphicsQueue().familyId(),
        m_device.computeQueue().familyId());
  
    commandBuffer.pipelineBarrier(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, bufferBarrier);

    commandBuffer.bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
    VkDescriptorSet descriptorSets{ m_computeDescriptorSet };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1, &descriptorSets, 0, 0);

    vkCmdDispatch(commandBuffer, m_groupCount, 1, 1);

    // Add memory barrier to ensure that compute shader has finished writing to the buffer
    // Without this the (rendering) vertex shader may display incomplete results (partial data from last frame) 
    bufferBarrier = createBufferMemoryBarrier(*m_vertexBuffer,
        VK_ACCESS_SHADER_WRITE_BIT,             // Compute shader has finished writes to the buffer
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,    // Vertex shader invocations want to read from the buffer
        m_device.computeQueue().familyId(),
        m_device.graphicsQueue().familyId());

    commandBuffer.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, bufferBarrier);

    commandBuffer.end();
}

void Renderer::render(const FrameData& frameData)
{
    // compute part
    m_computeMappedInputBuffer->timeDeltaInSeconds = m_stats.getDeltaTime();

    buildComputeCommandBuffer(*m_computeCommandBuffers[m_frameResourceId]);
    m_device.computeQueue().submitAsync(*m_computeCommandBuffers[m_frameResourceId]);

    // graphics part
    fillCommandBuffer(*frameData.resources.graphicsCommandBuffer, m_swapchainRenderPass, frameData.framebuffer, [this](auto& commandBuffer) { renderParticles(commandBuffer); });
}

void Renderer::createGUIContent()
{
    ImGui::Begin("GPU particle collision", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

    bool updatePartices = false;
    updatePartices |= ImGui::SliderInt("particles/s", &m_particlesPerSecond, 1, 10000);
    updatePartices |= ImGui::SliderFloat("particle lifetime/s", &m_particleLifetimeInSeconds, 1.f, 10.f, "%.1f");
    
    ImGui::SliderFloat("particle speed", &m_computeMappedInputBuffer->particleSpeed, 0.f, 20.f, "%.1f");
    ImGui::SliderFloat("gravity force", &m_computeMappedInputBuffer->gravityForce, -0.01f, 0.01f, "%.4f");
    ImGui::SliderFloat("collision damping", &m_computeMappedInputBuffer->collisionDamping, 0.0f, 2.0f, "%.1f");
    ImGui::SliderFloat2("emitter position", &m_computeMappedInputBuffer->emitterPos.x, -9.0f, 8.0f, "%.1f");
        
    if (updatePartices)
        updateParticleCount();

    ImGui::End();
}

void Renderer::updateParticleCount()
{
    // finish all frames so vertexbuffer is not not used anymore and we can update it
    waitForAllFrames();

    m_particleCount = static_cast<int>(m_particlesPerSecond * m_particleLifetimeInSeconds);
    m_computeMappedInputBuffer->particleCount = m_particleCount;
    m_computeMappedInputBuffer->particleLifetimeInSeconds = m_particleLifetimeInSeconds;
    m_groupCount = static_cast<uint32_t>(std::ceil(static_cast<float>(m_particleCount) / WORKGROUP_SIZE));

    setupParticleVertexBuffer();

    m_computeDescriptorSet.setStorageBuffer(BINDING_ID_COMPUTE_PARTICLES, *m_vertexBuffer);
    m_computeDescriptorSet.update(m_device);
}
