#include "basicrenderer.h"
#include "vulkanhelper.h"
#include "debug.h"

#include "../utils/glm.h"

#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <algorithm>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const glm::vec3 CameraUpVector(0.f, -1.f, 0.f);

bool BasicRenderer::init(GLFWwindow* window)
{
    createInstance();

    VK_CHECK_RESULT(glfwCreateWindowSurface(m_instance, window, NULL, &m_surface));

    createDevice();
    createSwapChain();

    m_swapChainDepthBufferFormat = m_device.findSupportedFormat(
    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    m_swapChainDepthBuffer.createDepthBuffer(&m_device, m_swapChain.getImageExtent(), m_swapChainDepthBufferFormat);

    m_renderPass.init(&m_device, m_swapChain.getImageFormat(), m_swapChainDepthBufferFormat);
    createSwapChainFramebuffers();

    m_device.createBuffer(sizeof(glm::mat4),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_uniformBuffer, m_uniformBufferMemory);

    if (!setup())
        return false;

    createCommandBuffers();
    fillCommandBuffers();
    
    return true;
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

bool BasicRenderer::createCommandBuffers()
{
    m_commandBuffers.resize(m_swapChain.getImageCount());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_device.getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device.getVkDevice(), &allocInfo, m_commandBuffers.data()));

    return true;
}

bool BasicRenderer::createSwapChainFramebuffers()
{
    m_framebuffers.resize(m_swapChain.getImageCount());

    for (size_t i = 0; i < m_framebuffers.size(); i++)
    {
        m_framebuffers[i].init(
            m_device.getVkDevice(),
            m_renderPass.getVkRenderPass(),
            { m_swapChain.getImageView(static_cast<uint32_t>(i)), m_swapChainDepthBuffer.getImageView() },
            m_swapChain.getImageExtent());
    }

    return true;
}

void BasicRenderer::destroy()
{
    // wait to avoid destruction of still used resources
    vkDeviceWaitIdle(m_device.getVkDevice());

    vkDestroyBuffer(m_device.getVkDevice(), m_uniformBuffer, nullptr);
    m_uniformBuffer = VK_NULL_HANDLE;
    vkFreeMemory(m_device.getVkDevice(), m_uniformBufferMemory, nullptr);
    m_uniformBufferMemory = VK_NULL_HANDLE;

    m_swapChainDepthBuffer.destroy();
    m_renderPass.destroy();
    destroyFramebuffers();
    destroyCommandBuffers();
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
    vkDeviceWaitIdle(m_device.getVkDevice());

    if (m_swapChain.create())
    {
        m_swapChainDepthBuffer.destroy();
        destroyFramebuffers();
        destroyCommandBuffers();
        createCommandBuffers();
        m_swapChainDepthBuffer.createDepthBuffer(&m_device, m_swapChain.getImageExtent(), m_swapChainDepthBufferFormat);
        createSwapChainFramebuffers();

        fillCommandBuffers();
        
        updateMVPUniform();

        return true;
    }
    return false;
}

void BasicRenderer::destroyFramebuffers()
{
    for (auto& framebuffer : m_framebuffers)
    {
        framebuffer.destroy();
    }
}

void BasicRenderer::destroyCommandBuffers()
{
    vkFreeCommandBuffers(m_device.getVkDevice(), m_device.getCommandPool(), static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
}

void BasicRenderer::draw()
{
    m_stats.startFrame();

    if (enableValidationLayers)
    {
        // to avoid memory leak from validation layers
        vkQueueWaitIdle(m_device.getPresentationQueue());
    }

    uint32_t imageId(0);
    if (!m_swapChain.acquireNextImage(imageId))
        resize(m_swapChain.getImageExtent().width, m_swapChain.getImageExtent().height);

    submitCommandBuffer(m_commandBuffers[imageId]);

    if (!m_swapChain.present(imageId))
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

    VK_CHECK_RESULT(vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
}

void BasicRenderer::updateMVPUniform()
{
    const glm::mat4 view = glm::lookAt(m_cameraPosition, m_cameraTarget, CameraUpVector);
    const glm::mat4 projection = glm::perspective(glm::radians(45.0f), m_swapChain.getImageExtent().width / static_cast<float>(m_swapChain.getImageExtent().height), 0.01f, m_sceneBoundingBoxDiameter * 20.f);
    const glm::mat4 mvp = projection * view;
    const uint32_t bufferSize = sizeof(mvp);

    void* data;
    vkMapMemory(m_device.getVkDevice(), m_uniformBufferMemory, 0, bufferSize, 0, &data);
    std::memcpy(data, &mvp, bufferSize);
    vkUnmapMemory(m_device.getVkDevice(), m_uniformBufferMemory);
}

void BasicRenderer::setCameraFromBoundingBox(const glm::vec3& min, const glm::vec3& max)
{
    const auto size = max - min;
    const auto center = (min + max) / 2.f;
    m_sceneBoundingBoxDiameter = std::max(std::max(size.x, size.y), size.z);
    const auto cameraDistance = m_sceneBoundingBoxDiameter * 1.5f;

    m_cameraPosition = glm::vec3(0, 2.f * center.y, cameraDistance) - center;
    m_cameraTarget = center;

    updateMVPUniform();
}

void BasicRenderer::mouseButton(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_1)
    {
        m_leftMouseButtonDown = action == GLFW_PRESS;
        m_observerCameraMode = (mods & GLFW_MOD_CONTROL) != 0;
    }
    else if (button == GLFW_MOUSE_BUTTON_3)
        m_middleMouseButtonDown = action == GLFW_PRESS;
    else if (button == GLFW_MOUSE_BUTTON_2)
        m_rightMouseButtonDown = action == GLFW_PRESS;
}

void BasicRenderer::mouseMove(double x, double y)
{
    if (m_leftMouseButtonDown ||
        m_middleMouseButtonDown ||
        m_rightMouseButtonDown)
    {
        const static auto rotationSize = 0.005f;
        const auto stepSize = 0.005f * m_sceneBoundingBoxDiameter;
        const auto deltaX = static_cast<float>(x - m_mousePositionX);
        const auto deltaY = static_cast<float>(y - m_mousePositionY);

        if (m_leftMouseButtonDown)
        {
            const auto cameraDirection = m_cameraTarget - m_cameraPosition;
            auto newCameraDirection = glm::rotate(cameraDirection, deltaX * rotationSize, glm::vec3(0.0f, 1.0f, 0.0f));
            newCameraDirection = glm::rotate(newCameraDirection, deltaY * rotationSize, glm::vec3(-1.0f, 0.0f, 0.0f));
            if (m_observerCameraMode)
            {
                m_cameraTarget = m_cameraPosition + newCameraDirection;
            }
            else
            {
                m_cameraPosition = m_cameraTarget - newCameraDirection;
            }
            updateMVPUniform();
        }
        if (m_middleMouseButtonDown)
        {
            const auto cameraDirection = glm::normalize(m_cameraTarget - m_cameraPosition);
            const auto offset = glm::vec3(deltaY * stepSize) * cameraDirection;
            m_cameraPosition += offset;
            m_cameraTarget += offset;
            updateMVPUniform();
        }
        if (m_rightMouseButtonDown)
        {
            const auto cameraDirection = glm::normalize(m_cameraTarget - m_cameraPosition);
            const auto cameraUp = glm::normalize(glm::cross(cameraDirection, CameraUpVector));
            const auto cameraRight = glm::normalize(glm::cross(cameraDirection, cameraUp));
            const auto cameraOffset = stepSize * ((cameraRight * deltaY) + (cameraUp * -deltaX));
            m_cameraPosition += cameraOffset;
            m_cameraTarget += cameraOffset;
            updateMVPUniform();
        }
    }

    m_mousePositionX = x;
    m_mousePositionY = y;
}