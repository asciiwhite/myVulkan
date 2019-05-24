#include "renderer.h"
#include "vulkanhelper.h"
#include "shader.h"
#include "barrier.h"
#include "imgui.h"
#include "objfileloader.h"

const uint32_t SET_ID_CAMERA = 0;
const uint32_t BINDING_ID_CAMERA = 0;

bool Renderer::setup()
{
    meshFilename = "data/meshes/kejim/kejim.obj";

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

    const uint32_t numDescriptors = static_cast<uint32_t>(20);    
    m_descriptorPool = m_device.createDescriptorPool(numDescriptors,
        { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, numDescriptors },
          { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numDescriptors } },
        true);
 
    setupCameraDescriptorSet();
    setClearColor({ 0.0f, 0.0f, 0.0f, 0.0f });

    const std::vector<RenderPassAttachmentDescription> sceneAttachmentData{ {
        { VK_FORMAT_B8G8R8A8_UNORM, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { VK_FORMAT_D32_SFLOAT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } } };
    m_sceneRenderPass = m_device.createRenderPass(sceneAttachmentData);

    const std::vector<RenderPassAttachmentDescription> colorBlitAttachmentData{ {
        { VK_FORMAT_B8G8R8A8_UNORM, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } } };
    m_colorBlitRenderPass = m_device.createRenderPass(colorBlitAttachmentData);

    const std::vector<RenderPassAttachmentDescription> cocBlitAttachmentData{ {
        { m_cocBufferFormat, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } } };
    m_cocBlitRenderPass = m_device.createRenderPass(cocBlitAttachmentData);

    const std::vector<RenderPassAttachmentDescription> combineBlitAttachmentData{ {
      { VK_FORMAT_R16G16B16A16_SFLOAT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } } };
    m_combineBlitRenderPass = m_device.createRenderPass(combineBlitAttachmentData);

    m_clampToEdgeSampler = m_device.createSampler(true);

    m_doFParameter.nearPlane = m_cameraHandler.m_nearPlane;
    m_doFParameter.farPlane = m_cameraHandler.m_farPlane;
//    m_doFParameter.focusDistance = m_cameraHandler.m_farPlane / 20.f;
//    m_doFParameter.focusRange = m_cameraHandler.m_farPlane / 100.f;
    m_doFParameterUB = UniformBuffer(m_device, &m_doFParameter);

    if (!createPlitPasses())
        return false;

    setupBlitPipelines();

    return true;
}

bool Renderer::createPlitPasses()
{ 
    if (!createBlitPass(m_blitPasses[eBlitTechnique::COPY_SWAPCHAIN], m_swapchainRenderPass, "data/shaders/passthrough.frag.spv"))
        return false;

    if (!createBlitPass(m_blitPasses[eBlitTechnique::DOWNSAMPLE], m_colorBlitRenderPass, "data/shaders/box_filter_3x3.frag.spv"))
        return false;

    if (!createBlitPass(m_blitPasses[eBlitTechnique::COMBINE_COC], m_combineBlitRenderPass, "data/shaders/combineCoc.frag.spv",
         { { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT } }))
        return false;

    if (!createBlitPass(m_blitPasses[eBlitTechnique::COMBINE_DOF], m_colorBlitRenderPass, "data/shaders/combineDof.frag.spv",
        { { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
          { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT } }))
        return false;

    if (!createBlitPass(m_blitPasses[eBlitTechnique::BOKEH], m_colorBlitRenderPass, "data/shaders/bokeh.frag.spv",
         { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT } }))
        return false;

    if (!createBlitPass(m_blitPasses[eBlitTechnique::COC], m_cocBlitRenderPass, "data/shaders/coc.frag.spv",
        { { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT } }))
        return false;

    return true;
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

    pass.renderPass = renderPass;

    return pass.pipeline;
}

void Renderer::destroyPlitPasses()
{
    for (auto& pass : m_blitPasses)
    {
        m_device.destroy(pass.pipelineLayout);
        m_device.destroy(pass.descriptorSetLayout);
        if (pass.pipeline)
            GraphicsPipeline::Release(m_device, pass.pipeline);
        if (pass.shader)
            ShaderManager::Release(m_device, pass.shader);
    }
}

void Renderer::setupBlitPipelines()
{
    VkExtent2D fullRes = m_swapChain.getImageExtent();
    VkExtent2D halfRes = { fullRes.width / 2, fullRes.height / 2 };

    addBlitPipeline(fullRes, m_cocBufferFormat, eBlitTechnique::COC);
    addBlitPipeline(halfRes, VK_FORMAT_R16G16B16A16_SFLOAT, eBlitTechnique::COMBINE_COC);
    addBlitPipeline(halfRes, VK_FORMAT_B8G8R8A8_UNORM, eBlitTechnique::BOKEH);
    addBlitPipeline(halfRes, VK_FORMAT_B8G8R8A8_UNORM, eBlitTechnique::DOWNSAMPLE);
    addBlitPipeline(fullRes, VK_FORMAT_B8G8R8A8_UNORM, eBlitTechnique::COMBINE_DOF);
    addBlitPipeline(fullRes, VK_FORMAT_B8G8R8A8_UNORM, eBlitTechnique::COPY_SWAPCHAIN);
}

bool Renderer::postResize()
{
    recreateDoFPipeline();
    return true;
}

void Renderer::recreateDoFPipeline()
{
    // finish all frames so we can update
    waitForAllFrames();

    destroyBlitPipelines();
    setupBlitPipelines();
    m_imagePool.clear();

    m_device.destroy(m_sceneFrameBuffer);
    m_sceneFrameBuffer = VK_NULL_HANDLE;
}

void Renderer::addBlitPipeline(VkExtent2D extent, VkFormat format, eBlitTechnique blitTechnique)
{
    BlitPassDescription passDescr;
    passDescr.frameBufferExtent = extent;
    passDescr.frameBufferFormat = format;
    passDescr.blitPass = &m_blitPasses[blitTechnique];
    passDescr.destriptorSet.allocate(m_device, passDescr.blitPass->descriptorSetLayout, m_descriptorPool);

    switch (blitTechnique)
    {
    case eBlitTechnique::COC:
    case eBlitTechnique::BOKEH:
        passDescr.destriptorSet.setUniformBuffer(1, m_doFParameterUB);
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

    m_doFParameterUB = UniformBuffer();
    destroyBlitPipelines();
    destroyPlitPasses();
    m_device.destroy(m_cameraDescriptorSetLayout);
    m_device.destroy(m_descriptorPool);
    m_device.destroy(m_colorBlitRenderPass);
    m_device.destroy(m_cocBlitRenderPass);
    m_device.destroy(m_combineBlitRenderPass);
    m_device.destroy(m_sceneRenderPass);    
    m_device.destroy(m_sceneFrameBuffer);
    m_device.destroy(m_clampToEdgeSampler);
}

void Renderer::render(const FrameData& frameData)
{
    const VkCommandBuffer commandBuffer = frameData.resources.graphicsCommandBuffer;
    VkExtent2D fullRes = m_swapChain.getImageExtent();

    auto [sceneColor, sceneDepth] = renderScenePass(commandBuffer, fullRes);

    size_t passId = 0;
    auto cocImage = renderBlitPass(commandBuffer, m_blitPassDescriptions[passId++], { sceneDepth->imageView() });
    auto combinedCoCImage = renderBlitPass(commandBuffer, m_blitPassDescriptions[passId++], { sceneColor->imageView(), cocImage->imageView() });
    auto bokehImage = renderBlitPass(commandBuffer, m_blitPassDescriptions[passId++], { combinedCoCImage->imageView() });
    auto filteredBokehImage = renderBlitPass(commandBuffer, m_blitPassDescriptions[passId++], { bokehImage->imageView() });
    auto combinedDoFCImage = renderBlitPass(commandBuffer, m_blitPassDescriptions[passId++], { sceneColor->imageView(), filteredBokehImage->imageView(), cocImage->imageView() });

    // show final image
    beginRenderPass(commandBuffer, m_swapchainRenderPass, frameData.framebuffer, fullRes, true);
    blitAttachment(commandBuffer, { m_showCoC ? cocImage->imageView() : m_enableDoF ? combinedDoFCImage->imageView() : sceneColor->imageView() }, m_blitPassDescriptions[passId++]);


    // this is done in base class
    //vkCmdEndRenderPass(commandBuffer);
    //VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}

std::pair<Renderer::ColorImageHandle, Renderer::DepthImageHandle> Renderer::renderScenePass(VkCommandBuffer commandBuffer, VkExtent2D extend)
{
    auto sceneColor = m_imagePool.aquire<ColorAttachment>(m_device, extend, VK_FORMAT_B8G8R8A8_UNORM);
    auto sceneDepth = m_imagePool.aquire<DepthStencilAttachment>(m_device, extend, VK_FORMAT_D32_SFLOAT);
    if (!m_sceneFrameBuffer)
        m_sceneFrameBuffer = m_device.createFramebuffer(m_sceneRenderPass, { sceneColor->imageView(), sceneDepth->imageView() }, extend);
    beginCommandBuffer(commandBuffer);
    beginRenderPass(commandBuffer, m_sceneRenderPass, m_sceneFrameBuffer, extend, true);
    m_mesh->render(commandBuffer);
    vkCmdEndRenderPass(commandBuffer);

    return { std::move(sceneColor), std::move(sceneDepth) };
}

Renderer::ColorImageHandle Renderer::renderBlitPass(VkCommandBuffer commandBuffer, BlitPassDescription& passDescr, const std::vector<VkImageView>& attachments)
{
    auto destImage = m_imagePool.aquire<ColorAttachment>(m_device, passDescr.frameBufferExtent, passDescr.frameBufferFormat);
    if (!passDescr.frameBuffer)
        passDescr.frameBuffer = m_device.createFramebuffer(passDescr.blitPass->renderPass, { destImage->imageView() }, passDescr.frameBufferExtent);
    beginRenderPass(commandBuffer, passDescr.blitPass->renderPass, passDescr.frameBuffer, passDescr.frameBufferExtent, false);
    blitAttachment(commandBuffer, attachments, passDescr);
    vkCmdEndRenderPass(commandBuffer);

    return std::move(destImage);
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
    ImGui::Begin("Depth of field", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);
    bool updateDoFPipeline = false;
    bool updateDoFParameter = false;
    updateDoFPipeline |= ImGui::Checkbox("Enable Depth of Field", &m_enableDoF);
    updateDoFPipeline |= ImGui::Checkbox("Show circle of confusion", &m_showCoC);
    updateDoFParameter |= ImGui::SliderFloat("Focus range", &m_doFParameter.focusRange, 0.1f, m_cameraHandler.m_farPlane);
    updateDoFParameter |= ImGui::SliderFloat("Focus distance", &m_doFParameter.focusDistance, 0.1f, m_cameraHandler.m_farPlane);
    updateDoFParameter |= ImGui::SliderFloat("Bokeh radius", &m_doFParameter.bokehRadius, 1.0f, 10.f);
    if (updateDoFPipeline)
        recreateDoFPipeline();
    if (updateDoFParameter)
        m_doFParameterUB.assign(&m_doFParameter, sizeof(m_doFParameter));
    ImGui::End();
}
