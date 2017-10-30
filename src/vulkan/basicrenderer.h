#pragma once

#include "swapchain.h"
#include "device.h"
#include "framebuffer.h"
#include "renderpass.h"

#include "../utils/statistics.h"

#include <vulkan/vulkan.h>

struct SDL_Window;

class BasicRenderer
{
public:
    bool init(SDL_Window* window);
    void destroy();

    bool resize(uint32_t width, uint32_t height);

    void draw();

private:
    bool createInstance(SDL_Window* window);
    bool createDevice();
    bool createSwapChain();
    bool createCommandBuffers();
    bool createSwapChainFramebuffers();

    void destroyFramebuffers();
    void destroyCommandBuffers();

    bool checkPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, uint32_t &graphicsQueueNodeIndex);
    void submitCommandBuffer(VkCommandBuffer commandBuffer);

    virtual bool setup() = 0;
    virtual void shutdown() = 0;
    virtual void fillCommandBuffers() = 0;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

protected:
    Device m_device;
    SwapChain m_swapChain;
    RenderPass m_renderPass;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<Framebuffer> m_framebuffers;

    Statistics m_stats;
};