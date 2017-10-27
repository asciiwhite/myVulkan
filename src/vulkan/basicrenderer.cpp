#include "basicrenderer.h"
#include "vulkanhelper.h"
#include "debug.h"

#include <SDL_vulkan.h>

#include <vector>
#include <iostream>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

bool BasicRenderer::init(SDL_Window* window)
{
    createInstance(window);

    SDL_Vulkan_CreateSurface(window, m_instance, &m_surface);

    createDevice();
    createSwapChain(window);
    m_renderPass.init(m_device.getVkDevice(), m_swapChain.getImageFormat());

    if (!setup())
        return false;

    createCommandBuffers();
    createSwapChainFramebuffers();

    fillCommandBuffers();
    
    return true;
}

bool BasicRenderer::createInstance(SDL_Window* window)
{
    uint32_t extensionCount(0);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr))
        return false;

    std::vector<const char*> extensions(extensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, &extensions[0]))
        return false;

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

bool BasicRenderer::createSwapChain(SDL_Window* window)
{
    m_swapChain.init(m_instance, m_surface, m_device);

    int width(0);
    int height(0);
    SDL_Vulkan_GetDrawableSize(window, &width, &height);
    return m_swapChain.create(width, height);
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

    for (size_t i = 0; i < m_commandBuffers.size(); i++)
    {
        m_framebuffers[i].init(
            m_device.getVkDevice(),
            m_renderPass.getVkRenderPass(),
            m_swapChain.getImageView(static_cast<uint32_t>(i)),
            m_swapChain.getImageExtent());
    }

    return true;
}

void BasicRenderer::destroy()
{
    // wait to avoid destruction of still used resources
    vkDeviceWaitIdle(m_device.getVkDevice());

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

bool BasicRenderer::resize(uint32_t width, uint32_t height)
{
    vkDeviceWaitIdle(m_device.getVkDevice());

    if (m_swapChain.create(width, height))
    {
        destroyFramebuffers();
        destroyCommandBuffers();
        createCommandBuffers();
        createSwapChainFramebuffers();

        fillCommandBuffers();
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