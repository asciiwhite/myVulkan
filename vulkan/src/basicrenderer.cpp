#include "basicrenderer.h"
#include "vulkanhelper.h"
#include "debug.h"

#include "../utils/glm.h"
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
    m_swapChainDepthBuffer.createDepthBuffer(&m_device, m_swapChain.getImageExtent(), m_swapChainDepthBufferFormat);

    m_renderPass = m_device.createRenderPass(m_swapChain.getImageFormat(), m_swapChainDepthBufferFormat);
    createSwapChainFramebuffers();

    m_cameraUniformBuffer = m_device.createBuffer(sizeof(glm::mat4),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    createFrameResources(2u);

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
        instanceCreateInfo.enabledLayerCount = debug::validationLayerCount;
        instanceCreateInfo.ppEnabledLayerNames = debug::validationLayerNames;
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
    m_swapChain.init(m_instance, m_surface, m_device);
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

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_device.getGraphicsCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& resource : m_frameResources)
    {
        VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device, &allocInfo, &resource.graphicsCommandBuffer));
        VK_CHECK_RESULT(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &resource.frameCompleteFence));
    }

    return true;
}

bool BasicRenderer::createSwapChainFramebuffers()
{
    m_framebuffers.resize(m_swapChain.getImageCount());

    for (size_t i = 0; i < m_framebuffers.size(); i++)
    {
        m_framebuffers[i].init(
            m_device,
            m_renderPass,
            { m_swapChain.getImageView(static_cast<uint32_t>(i)), m_swapChainDepthBuffer.getImageView() },
            m_swapChain.getImageExtent());
    }

    return true;
}

void BasicRenderer::destroy()
{
    // wait to avoid destruction of still used resources
    vkDeviceWaitIdle(m_device);

    m_device.destroyBuffer(m_cameraUniformBuffer);
    m_swapChainDepthBuffer.destroy();
    m_device.destroyRenderPass(m_renderPass);
    destroyFramebuffers();
    destroyFrameResources();
    m_swapChain.destroy();

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
        m_swapChainDepthBuffer.destroy();
        destroyFramebuffers();
        m_swapChainDepthBuffer.createDepthBuffer(&m_device, m_swapChain.getImageExtent(), m_swapChainDepthBufferFormat);
        createSwapChainFramebuffers();

        updateMVPUniform();

        return true;
    }
    return false;
}

void BasicRenderer::destroyFramebuffers()
{
    for (auto& framebuffer : m_framebuffers)
        framebuffer.destroy();
}

void BasicRenderer::destroyFrameResources()
{
    for (const auto& resource : m_frameResources)
    {
        vkFreeCommandBuffers(m_device, m_device.getGraphicsCommandPool(), 1, &resource.graphicsCommandBuffer);
        vkDestroyFence(m_device, resource.frameCompleteFence, nullptr);
    }
    m_frameResources.clear();
}

void BasicRenderer::draw()
{
    m_stats.startFrame();

    m_frameResourceId = (m_frameResourceId + 1) % m_frameResourceCount;

    vkWaitForFences(m_device, 1, &m_frameResources[m_frameResourceId].frameCompleteFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_frameResources[m_frameResourceId].frameCompleteFence);

    uint32_t swapChainImageId(0);
    if (!m_swapChain.acquireNextImage(swapChainImageId))
        resize(m_swapChain.getImageExtent().width, m_swapChain.getImageExtent().height);
    
    const FrameData frameData {
        m_frameResources[m_frameResourceId], m_framebuffers[swapChainImageId]
    };

    render(frameData);

    if (!m_swapChain.present(swapChainImageId))
        resize(m_swapChain.getImageExtent().width, m_swapChain.getImageExtent().height);

    if (m_stats.endFrame())
    {
        std::cout << "Min/Max/Avg: " << m_stats.getMin() << " " << m_stats.getMax() << " " << m_stats.getAvg() << std::endl;
        m_stats.reset();
    }
}

void BasicRenderer::submitCommandBuffer(VkCommandBuffer commandBuffer)
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = m_swapChain.getImageAvailableSemaphore();
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = m_swapChain.getRenderFinishedSemaphore();

    VK_CHECK_RESULT(vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, m_frameResources[m_frameResourceId].frameCompleteFence));
}

void BasicRenderer::updateMVPUniform()
{
    const auto lookVec = m_observerCameraMode ? m_cameraTarget : m_cameraPosition + m_cameraLook;

    const glm::mat4 view = glm::lookAt(m_cameraPosition, lookVec, m_cameraUp);
    const glm::mat4 projection = glm::perspective(glm::radians(45.0f), m_swapChain.getImageExtent().width / static_cast<float>(m_swapChain.getImageExtent().height), 0.01f, m_sceneBoundingBoxDiameter * 20.f);
    const glm::mat4 mvp = projection * view;
    const uint32_t bufferSize = sizeof(mvp);

    void* data = m_device.mapBuffer(m_cameraUniformBuffer, bufferSize);
    std::memcpy(data, &mvp, bufferSize);
    m_device.unmapBuffer(m_cameraUniformBuffer);
}

void BasicRenderer::setCameraFromBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& lookDir)
{
    const auto size = max - min;
    const auto center = (min + max) / 2.f;
    m_sceneBoundingBoxDiameter = std::max(std::max(size.x, size.y), size.z);
    const auto cameraDistance = m_sceneBoundingBoxDiameter * 1.5f;

    m_cameraPosition = glm::vec3(cameraDistance) * glm::normalize(lookDir) - center;
    m_cameraTarget = center;
    m_cameraLook = glm::normalize(m_cameraTarget - m_cameraPosition);
    m_cameraUp = glm::vec3(0, -1, 0);

    updateMVPUniform();
}

void BasicRenderer::update()
{
    if ((m_leftMouseButtonDown ||
        m_middleMouseButtonDown ||
        m_rightMouseButtonDown) && 
        !m_observerCameraMode)
    {
        const auto stepSize = 0.05f * m_sceneBoundingBoxDiameter;

        const auto deltaX = static_cast<int>(m_mousePositionX - m_lastMousePositionX);
        const auto deltaY = static_cast<int>(m_mousePositionY - m_lastMousePositionY);

        flythrough_camera_update(
            glm::value_ptr(m_cameraPosition),
            glm::value_ptr(m_cameraLook),
            glm::value_ptr(m_cameraUp),
            nullptr,
            0.01f,
            stepSize,
            0.5f,
            80.0f,
            deltaX, deltaY,
            m_rightMouseButtonDown,
            m_middleMouseButtonDown,
            0,
            0,
            0, 0, 0);

        m_cameraTarget += m_cameraLook;
        
        updateMVPUniform();

        m_lastMousePositionX = m_mousePositionX;
        m_lastMousePositionY = m_mousePositionY;
    }
}

void BasicRenderer::mouseButton(int button, int action, int mods)
{
    m_observerCameraMode = (mods & GLFW_MOD_CONTROL) == 0;

    if (button == GLFW_MOUSE_BUTTON_1)
        m_leftMouseButtonDown = action == GLFW_PRESS;
    else if (button == GLFW_MOUSE_BUTTON_3)
        m_middleMouseButtonDown = action == GLFW_PRESS;
    else if (button == GLFW_MOUSE_BUTTON_2)
        m_rightMouseButtonDown = action == GLFW_PRESS;
}

void BasicRenderer::mouseMove(double x, double y)
{
    m_lastMousePositionX = m_mousePositionX;
    m_lastMousePositionY = m_mousePositionY;
    m_mousePositionX = x;
    m_mousePositionY = y;

    if ((m_leftMouseButtonDown ||
        m_middleMouseButtonDown ||
        m_rightMouseButtonDown) && 
        m_observerCameraMode)
    {
            const auto stepSize = 0.005f * m_sceneBoundingBoxDiameter;

            arcball_camera_update(
                glm::value_ptr(m_cameraPosition),
                glm::value_ptr(m_cameraTarget),
                glm::value_ptr(m_cameraUp),
                nullptr,
                0.01f,
                stepSize, 100.0f, 5.0f,
                m_swapChain.getImageExtent().width, m_swapChain.getImageExtent().height,
                static_cast<int>(m_lastMousePositionX), static_cast<int>(m_mousePositionX),
                static_cast<int>(m_mousePositionY), static_cast<int>(m_lastMousePositionY),  // inverse axis for correct rotation direction
                m_rightMouseButtonDown,
                m_leftMouseButtonDown,
                m_middleMouseButtonDown ? static_cast<int>(m_mousePositionY - m_lastMousePositionY) : 0,
                0);

            m_cameraLook = glm::normalize(m_cameraTarget - m_cameraPosition);
            
            updateMVPUniform();
    }    
}