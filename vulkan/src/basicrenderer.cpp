#include "basicrenderer.h"
#include "vulkanhelper.h"
#include "debug.h"
#include "imgui.h"

#include "../utils/arcball_camera.h"
#include "../utils/flythrough_camera.h"

#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <algorithm>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

BasicRenderer::BasicRenderer()
    : m_inputHandler(m_cameraHandler)
    , m_swapChain(m_device)
{
}

bool BasicRenderer::init(GLFWwindow* window)
{
    createInstance();

    VK_CHECK_RESULT(glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface));

    createDevice();
    createSwapChain();

    m_swapChainDepthBufferFormat = m_device.findSupportedFormat(
    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    m_swapChainDepthAttachment = DepthStencilAttachment(m_device, m_swapChain.getImageExtent(), m_swapChainDepthBufferFormat);

    static const std::vector<RenderPassAttachmentDescription> defaultAttachmentData{ {
        { m_swapChain.getImageFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },
        { m_swapChainDepthBufferFormat, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL } } };

    m_swapchainRenderPass = m_device.createRenderPass(defaultAttachmentData);
    createSwapChainFramebuffers();

    m_cameraUniformBuffer = UniformBuffer(m_device, sizeof(CameraParameter));
    m_mappedCameraUniformBuffer = reinterpret_cast<CameraParameter*>(m_cameraUniformBuffer.map());

    const auto frameResourceCount = 2u;

    createFrameResources(frameResourceCount);

    m_gui = std::unique_ptr<GUI>(new GUI(m_device));
    m_gui->setup(frameResourceCount, m_swapChain.getImageExtent().width, m_swapChain.getImageExtent().height, m_swapchainRenderPass);

    return setup();
}

bool BasicRenderer::createInstance()
{
    uint32_t extensionCount(0);
    const char** rawExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    std::vector<const char*> extensions;
    for (unsigned int i = 0; i < extensionCount; i++)
    {
        extensions.push_back(rawExtensions[i]);
    }

    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "myVulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = &extensions[0];
    if (enableValidationLayers)
    {
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(debug::validationLayerNames.size());
        instanceCreateInfo.ppEnabledLayerNames = debug::validationLayerNames.data();
    }

    VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

    if (enableValidationLayers)
    {
        debug::setupDebugCallback(m_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_NULL_HANDLE);
    }

    return true;
}

bool BasicRenderer::createSwapChain()
{
    m_swapChain.init(m_surface);
    return m_swapChain.create();
}

bool BasicRenderer::createDevice()
{
    return m_device.init(m_instance, m_surface, enableValidationLayers);
}

bool BasicRenderer::createFrameResources(uint32_t numFrames)
{
    m_frameResourceCount = numFrames;
    m_frameResources.resize(m_frameResourceCount);

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& resource : m_frameResources)
    {
        resource.graphicsCommandBuffer = m_device.createCommandBuffer();
        VK_CHECK_RESULT(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &resource.frameCompleteFence));
    }

    return true;
}

bool BasicRenderer::createSwapChainFramebuffers()
{
    m_framebuffers.resize(m_swapChain.getImageCount());

    for (size_t i = 0; i < m_framebuffers.size(); i++)
    {
        m_framebuffers[i] = m_device.createFramebuffer(
            m_swapchainRenderPass,
            { m_swapChain.getImageView(static_cast<uint32_t>(i)), m_swapChainDepthAttachment.imageView() },
            m_swapChain.getImageExtent());
    }

    return true;
}

void BasicRenderer::destroy()
{
    // wait to avoid destruction of still used resources
    vkDeviceWaitIdle(m_device);
    
    m_gui.reset();

    m_cameraUniformBuffer = UniformBuffer();
    m_swapChainDepthAttachment = DepthStencilAttachment();
    m_device.destroy(m_swapchainRenderPass);
    destroyFramebuffers();
    destroyFrameResources();
    m_swapChain.destroy();
    m_imagePool.clear();
    shutdown();

    m_device.destroy();
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

    if (enableValidationLayers)
    {
        debug::destroyDebugCallback(m_instance);
    }
    vkDestroyInstance(m_instance, nullptr);
}

bool BasicRenderer::resize(uint32_t /*width*/, uint32_t /*height*/)
{
    vkDeviceWaitIdle(m_device);

    if (m_swapChain.create())
    {
        destroyFramebuffers();
        m_swapChainDepthAttachment = DepthStencilAttachment(m_device, m_swapChain.getImageExtent(), m_swapChainDepthBufferFormat);
        createSwapChainFramebuffers();

        m_gui->onResize(m_swapChain.getImageExtent().width, m_swapChain.getImageExtent().height);

        updateMVPUniform();
        postResize();

        return true;
    }
    return false;
}

void BasicRenderer::destroyFramebuffers()
{
    for (auto& framebuffer : m_framebuffers)
        m_device.destroy(framebuffer);
}

void BasicRenderer::destroyFrameResources()
{
    for (const auto& resource : m_frameResources)
    {
        vkDestroyFence(m_device, resource.frameCompleteFence, nullptr);
    }
    m_frameResources.clear();
}

void BasicRenderer::draw()
{
    m_stats.update();

    // gui frame start
    m_gui->startFrame(m_stats, m_inputHandler.getMouseInputState());
    createGUIContent();

    // wait for previous frame completion
    m_frameResourceId = (m_frameResourceId + 1) % m_frameResourceCount;
    vkWaitForFences(m_device, 1, &m_frameResources[m_frameResourceId].frameCompleteFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_frameResources[m_frameResourceId].frameCompleteFence);

    // aquire image for rendering
    uint32_t swapChainImageId(0);
    if (!m_swapChain.acquireNextImage(swapChainImageId))
        resize(m_swapChain.getImageExtent().width, m_swapChain.getImageExtent().height);
    
    auto& commandBuffer = *m_frameResources[m_frameResourceId].graphicsCommandBuffer;

    // scene rendering
    render({ m_frameResources[m_frameResourceId], m_framebuffers[swapChainImageId] });

    // gui rendering
    m_gui->draw(m_frameResourceId, commandBuffer);
    
    // submission
    m_device.graphicsQueue().submitAsync(
        commandBuffer,
        m_swapChain.getImageAvailableSemaphore(),
        m_swapChain.getRenderFinishedSemaphore(),
        m_frameResources[m_frameResourceId].frameCompleteFence,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    // presentation
    if (!m_swapChain.present(swapChainImageId))
        resize(m_swapChain.getImageExtent().width, m_swapChain.getImageExtent().height);
}

void BasicRenderer::fillCommandBuffer(CommandBuffer& commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, const DrawFunc& drawFunc)
{
    commandBuffer.begin();
    commandBuffer.beginRenderPass(renderPass, framebuffer, m_swapChain.getImageExtent());

    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(m_swapChain.getImageExtent().width), static_cast<float>(m_swapChain.getImageExtent().height), 0.0f, 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = { { 0, 0 }, m_swapChain.getImageExtent() };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    drawFunc(commandBuffer);
}

void BasicRenderer::updateMVPUniform()
{
    m_mappedCameraUniformBuffer->mvp = m_cameraHandler.mvp(m_swapChain.getImageExtent().width / static_cast<float>(m_swapChain.getImageExtent().height));
    m_mappedCameraUniformBuffer->pos = glm::make_vec4(m_cameraHandler.cameraPosition());
    m_mappedCameraUniformBuffer->pixelsPerRadians = static_cast<float>(m_swapChain.getImageExtent().height) / m_cameraHandler.m_fovRadians;
}

void BasicRenderer::setCameraFromBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& lookDir)
{
    m_cameraHandler.setCameraFromBoundingBox(min, max, lookDir);
    updateMVPUniform();
}

void BasicRenderer::setClearColor(VkClearColorValue clearColor)
{
    m_clearColor = clearColor;
}

const VkClearColorValue& BasicRenderer::clearColor() const
{
    return m_clearColor;
}

void BasicRenderer::update()
{
    if (m_inputHandler.update())
        updateMVPUniform();
}

void BasicRenderer::mouseButton(int button, int action, int mods)
{
    m_inputHandler.button(button, action, mods);
}

void BasicRenderer::mouseMove(double x, double y)
{
    const auto disableCameraUpdate = ImGui::IsAnyItemActive();
    if (m_inputHandler.move(static_cast<float>(x), static_cast<float>(y), m_stats.getDeltaTime(), m_swapChain.getImageExtent().width, m_swapChain.getImageExtent().height, disableCameraUpdate))
        updateMVPUniform();
}

void BasicRenderer::waitForAllFrames() const
{
    for (const auto& frameResource : m_frameResources)
        vkWaitForFences(m_device, 1, &frameResource.frameCompleteFence, VK_TRUE, UINT64_MAX);
}
