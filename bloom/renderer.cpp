#include "renderer.h"
#include "vulkanhelper.h"
#include "shader.h"
#include "barrier.h"
#include "imgui.h"
#include "objfileloader.h"
#include "imagepool.h" 

const uint32_t SET_ID_CAMERA = 0;
const uint32_t BINDING_ID_CAMERA = 0;

bool Renderer::setup()
{
    meshFilename = "data/meshes/holodeck/holodeck.obj";

    MeshDescription meshDesc;
    if (ObjFileLoader::read(meshFilename, meshDesc))
    {
        m_mesh.reset(new Mesh(m_device));
        if (!m_mesh->init(meshDesc, m_cameraUniformBuffer, m_swapchainRenderPass))
            m_mesh.reset();
        else
            setCameraFromBoundingBox(meshDesc.boundingBox.min, meshDesc.boundingBox.max, glm::vec3(1.f, 0.5f, 0.f));
    }

    if (!m_mesh)
        return false;

    const uint32_t numDescriptors = static_cast<uint32_t>( 5 * m_maxDownsampleLoops + 2);    
    m_descriptorPool = m_device.createDescriptorPool(numDescriptors,
        { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, numDescriptors - 1},
          { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numDescriptors } },
        true);
 
    setupCameraDescriptorSet();
    setClearColor({ 0.0f, 0.0f, 0.0f, 0.0f });

    const std::vector<RenderPassAttachmentData> sceneAttachmentData{ {
        { VK_FORMAT_B8G8R8A8_UNORM, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { VK_FORMAT_D32_SFLOAT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL } } };
    m_sceneRenderPass = m_device.createRenderPass(sceneAttachmentData);

    const std::vector<RenderPassAttachmentData> blitAttachmentData{ {
        { VK_FORMAT_B8G8R8A8_UNORM, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } } };
    m_blitRenderPass = m_device.createRenderPass(blitAttachmentData);

    m_clampToEdgeSampler = m_device.createSampler(true);

    m_bloomParameterUB = UniformBuffer(m_device, &m_bloomParameter);

    createPlitPasses();
    setupBlitPipelines();

    return true;
}

void Renderer::createPlitPasses()
{ 
    createBlitPass(m_blitPasses[eBlitTechnique::COPY],           m_blitRenderPass,      "data/shaders/passthrough.frag.spv");
    createBlitPass(m_blitPasses[eBlitTechnique::COPY_SWAPCHAIN], m_swapchainRenderPass, "data/shaders/passthrough.frag.spv");
    createBlitPass(m_blitPasses[eBlitTechnique::BOX_4x4],        m_blitRenderPass,      "data/shaders/box_filter_4x4.frag.spv");

    createBlitPass(m_blitPasses[eBlitTechnique::BOX_3x3],        m_blitRenderPass,       "data/shaders/box_filter_3x3.frag.spv", 
        { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT } });

    createBlitPass(m_blitPasses[eBlitTechnique::BOX_3x3_ADD],   m_swapchainRenderPass,  "data/shaders/box_filter_3x3_add.frag.spv",
        { { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, 
        {   2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT } }, true);

    createBlitPass(m_blitPasses[eBlitTechnique::PREFILTER],     m_blitRenderPass,       "data/shaders/prefilter.frag.spv",
        { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT } });
}

bool Renderer::createBlitPass(BlitPass& pass, VkRenderPass renderPass, const char* fragmentShaderFilename, const std::vector<VkDescriptorSetLayoutBinding>& additionalBindings, bool alphaBlend)
{
    const ShaderResourceHandler::ShaderModulesDescription shaderDesc(
        { { VK_SHADER_STAGE_VERTEX_BIT, "data/shaders/fullscreen.vert.spv" },
          { VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderFilename } });

    pass.shader = ShaderManager::Acquire(m_device, shaderDesc);
    if (!pass.shader)
        return false;

    std::vector<VkDescriptorSetLayoutBinding> descriptorLayoutBinding({ { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT } });
    descriptorLayoutBinding.insert(descriptorLayoutBinding.end(), additionalBindings.begin(), additionalBindings.end());
    pass.descriptorSetLayout = m_device.createDescriptorSetLayout(descriptorLayoutBinding);
    pass.pipelineLayout = m_device.createPipelineLayout({ pass.descriptorSetLayout });

    GraphicsPipelineSettings blitSettings;
    blitSettings.setDepthTesting(false);
    if (alphaBlend)
        blitSettings.setAlphaBlending(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE);

    pass.pipeline = GraphicsPipeline::Acquire(m_device,
        renderPass,
        pass.pipelineLayout,
        blitSettings,
        pass.shader.shaderStageCreateInfos,
        std::vector<VkVertexInputAttributeDescription>{},
        std::vector<VkVertexInputBindingDescription>{});

    return pass.pipeline;
}

void Renderer::destroyPlitPasses()
{
    for (auto& pass : m_blitPasses)
    {
        m_device.destroy(pass.pipelineLayout);
        m_device.destroy(pass.descriptorSetLayout);
        GraphicsPipeline::Release(m_device, pass.pipeline);
        ShaderManager::Release(m_device, pass.shader);
    }
}

void Renderer::setupBlitPipelines()
{
    if (m_enableBloom)
    {
        VkExtent2D resolution = m_swapChain.getImageExtent();
        int step;
        for (step = 0; step < m_numDownsampleLoops; step++)
        {
            resolution.width  /= 2;
            resolution.height /= 2;
            if (resolution.width < 2 || resolution.height < 2)
            {
                if (m_useDownsampling)
                    break;
                else
                    step = m_numDownsampleLoops - 1;
            }
            if (m_useDownsampling || step == m_numDownsampleLoops - 1)
                addBlitPipeline(resolution, step == 0 ? eBlitTechnique::PREFILTER : m_useBoxFilter ? eBlitTechnique::BOX_4x4 : eBlitTechnique::COPY);
        }

        if (m_useUpsampling)
        {
            for (step -= 2; step >= 0; step--)
            {
                resolution.width  *= 2;
                resolution.height *= 2;
                addBlitPipeline(resolution, m_useBoxFilter ? eBlitTechnique::BOX_3x3 : eBlitTechnique::COPY);
            }
        }
    }
    if (m_showDebug)
        addBlitPipeline(m_swapChain.getImageExtent(), eBlitTechnique::COPY_SWAPCHAIN);
    else
        addBlitPipeline(m_swapChain.getImageExtent(), m_enableBloom ? eBlitTechnique::BOX_3x3_ADD : eBlitTechnique::COPY_SWAPCHAIN);
}

bool Renderer::postResize()
{
    recreateBlitPipeline();
    return true;
}

void Renderer::recreateBlitPipeline()
{
    // finish all frames so we can update
    waitForAllFrames();

    destroyBlitPipelines();
    setupBlitPipelines();

    m_device.destroy(m_sceneFrameBuffer);
    m_sceneFrameBuffer = VK_NULL_HANDLE;
}

void Renderer::addBlitPipeline(VkExtent2D extent, eBlitTechnique blitTechnique)
{
    BlitPassDescription passDescr;
    passDescr.frameBufferExtent = extent;
    passDescr.blitPass = &m_blitPasses[blitTechnique];
    passDescr.destriptorSet.allocate(m_device, passDescr.blitPass->descriptorSetLayout, m_descriptorPool);

    switch (blitTechnique)
    {
    case eBlitTechnique::BOX_3x3:
    case eBlitTechnique::PREFILTER:
        passDescr.destriptorSet.setUniformBuffer(1, m_bloomParameterUB);
        break;
    case eBlitTechnique::BOX_3x3_ADD:
        passDescr.destriptorSet.setUniformBuffer(2, m_bloomParameterUB);
        break;
    default:
        break;
    }

    m_blitPassDescriptions.push_back(passDescr);
}

void Renderer::destroyBlitPipelines()
{
    for (auto& descr : m_blitPassDescriptions)
    {
        m_device.destroy(descr.frameBuffer);
        descr.destriptorSet.free(m_device, m_descriptorPool);
    }
    m_blitPassDescriptions.clear();
}

void Renderer::setupCameraDescriptorSet()
{
    m_cameraDescriptorSetLayout = m_device.createDescriptorSetLayout({ { BINDING_ID_CAMERA, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT } });

    m_cameraUniformDescriptorSet.setUniformBuffer(BINDING_ID_CAMERA, m_cameraUniformBuffer);
    m_cameraUniformDescriptorSet.allocateAndUpdate(m_device, m_cameraDescriptorSetLayout, m_descriptorPool);
}

void Renderer::shutdown()
{
    m_mesh.reset();

    m_bloomParameterUB = UniformBuffer();
    destroyBlitPipelines();
    destroyPlitPasses();
    m_device.destroy(m_cameraDescriptorSetLayout);
    m_device.destroy(m_descriptorPool);
    m_device.destroy(m_blitRenderPass);
    m_device.destroy(m_sceneRenderPass);    
    m_device.destroy(m_sceneFrameBuffer);
    m_device.destroy(m_clampToEdgeSampler);
}

void Renderer::render(const FrameData& frameData)
{
    const VkCommandBuffer commandBuffer = frameData.resources.graphicsCommandBuffer;
    VkExtent2D res = m_swapChain.getImageExtent();
    
    auto sceneColor = ImagePool::Aquire<ColorAttachment>(m_device, res, VK_FORMAT_B8G8R8A8_UNORM);
    auto sceneDepth = ImagePool::Aquire<DepthStencilAttachment>(m_device, res, VK_FORMAT_D32_SFLOAT);
    if (!m_sceneFrameBuffer)
        m_sceneFrameBuffer = m_device.createFramebuffer(m_sceneRenderPass, { sceneColor->imageView(), sceneDepth->imageView() }, res);

    beginCommandBuffer(commandBuffer);
    beginRenderPass(commandBuffer, m_sceneRenderPass, m_sceneFrameBuffer, res, true);
    m_mesh->render(commandBuffer);
    vkCmdEndRenderPass(commandBuffer);
    ImagePool::Release(std::move(sceneDepth));

    ImagePool::ImageHandle<ColorAttachment> srcImage;
    ImagePool::ImageHandle<ColorAttachment> dstImage;

    if (m_enableBloom || m_showDebug)
    {
        auto& firstPassDescr = m_blitPassDescriptions[0];
        res = firstPassDescr.frameBufferExtent;
        dstImage = ImagePool::Aquire<ColorAttachment>(m_device, res, VK_FORMAT_B8G8R8A8_UNORM);
        if (!firstPassDescr.frameBuffer)
            firstPassDescr.frameBuffer = m_device.createFramebuffer(m_blitRenderPass, { dstImage->imageView() }, res);

        beginRenderPass(commandBuffer, m_blitRenderPass, firstPassDescr.frameBuffer, res, false);
        blitAttachment(commandBuffer, { sceneColor->imageView() }, firstPassDescr);
        vkCmdEndRenderPass(commandBuffer);

        srcImage = std::move(dstImage);

        for (auto i = 1; i < m_blitPassDescriptions.size() - 1; i++)
        {
            auto& passDescr = m_blitPassDescriptions[i];
            res = passDescr.frameBufferExtent;

            dstImage = ImagePool::Aquire<ColorAttachment>(m_device, res, VK_FORMAT_B8G8R8A8_UNORM);
            if (!passDescr.frameBuffer)
                passDescr.frameBuffer = m_device.createFramebuffer(m_blitRenderPass, { dstImage->imageView() }, res);

            beginRenderPass(commandBuffer, m_blitRenderPass, passDescr.frameBuffer, res, false);
            blitAttachment(commandBuffer, { srcImage->imageView() }, passDescr);
            vkCmdEndRenderPass(commandBuffer);

            ImagePool::Release(std::move(srcImage));
            srcImage = std::move(dstImage);
        }
    }

    auto& passDescr = m_blitPassDescriptions.back();
    beginRenderPass(commandBuffer, m_swapchainRenderPass, frameData.framebuffer, m_swapChain.getImageExtent(), true);
    if (m_showDebug)
    {
        blitAttachment(commandBuffer, { srcImage->imageView() }, passDescr);
    }
    else
    {
        if (m_enableBloom)
        {
            blitAttachment(commandBuffer, { srcImage->imageView(), sceneColor->imageView() }, passDescr);
        }
        else
        {
            blitAttachment(commandBuffer, { sceneColor->imageView() }, passDescr);
        }
    }

    if (m_enableBloom)
        ImagePool::Release(std::move(srcImage));
    ImagePool::Release(std::move(sceneColor));

    // this is done in base class
    //vkCmdEndRenderPass(commandBuffer);
    //VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}

void Renderer::blitAttachment(VkCommandBuffer commandBuffer, const std::vector<VkImageView>& attachments, BlitPassDescription& blitPassDescr)
{
    if (!blitPassDescr.destriptorSet.isValid())
    {
        for (auto i=0; i < attachments.size(); i++)
            blitPassDescr.destriptorSet.setImageSampler(i, attachments[i], m_clampToEdgeSampler);
        blitPassDescr.destriptorSet.update(m_device);
    }
    blitPassDescr.destriptorSet.bind(commandBuffer, blitPassDescr.blitPass->pipelineLayout, 0);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, blitPassDescr.blitPass->pipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void Renderer::createGUIContent()
{
    ImGui::Begin("Bloom", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);
    bool updateBlitPipeline = false;
    bool updatePreFilterParameter = false;
    updateBlitPipeline |= ImGui::Checkbox("Enabled", &m_enableBloom);
    updateBlitPipeline |= ImGui::SliderInt("Resolution reduction steps", &m_numDownsampleLoops, 1, m_maxDownsampleLoops);
    updatePreFilterParameter |= ImGui::SliderFloat("Intensity", &m_bloomParameter.intensity, 0.f, 10.f);
    updatePreFilterParameter |= ImGui::SliderFloat("Brightness threshold", &m_bloomParameter.preFilterThreshold, 0.f, 1.f);
    updateBlitPipeline |= ImGui::Checkbox("With downsampling", &m_useDownsampling);
    updateBlitPipeline |= ImGui::Checkbox("With upsampling", &m_useUpsampling);
    updateBlitPipeline |= ImGui::Checkbox("With box filter", &m_useBoxFilter);
    updateBlitPipeline |= ImGui::Checkbox("Show debug", &m_showDebug);
    m_enableBloom |= m_showDebug;
    m_useDownsampling |= m_showDebug;
    if (updateBlitPipeline)
        recreateBlitPipeline();
    if (updatePreFilterParameter)
        m_bloomParameterUB.assign(&m_bloomParameter, sizeof(m_bloomParameter));
    ImGui::End();
}
