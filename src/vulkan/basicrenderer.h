#pragma once

#include "swapchain.h"
#include "device.h"
#include "framebuffer.h"
#include "renderpass.h"
#include "texture.h"

#include "../utils/statistics.h"
#include "../utils/glm.h"

#include <vulkan/vulkan.h>

struct GLFWwindow;

class BasicRenderer
{
public:
    bool init(GLFWwindow* window);
    void destroy();

    bool resize(uint32_t width, uint32_t height);
    void mouseButton(int button, int action, int mods);
    void mouseMove(double x, double y);

    virtual void update();
    void draw();

protected:
    void setCameraFromBoundingBox(const glm::vec3& min, const glm::vec3& max);

private:
    bool createInstance();
    bool createDevice();
    bool createSwapChain();
    bool createCommandBuffers();
    bool createSwapChainFramebuffers();

    void destroyFramebuffers();
    void destroyCommandBuffers();

    bool checkPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, uint32_t &graphicsQueueNodeIndex);
    void submitCommandBuffer(VkCommandBuffer commandBuffer);

    void updateMVPUniform();

    virtual bool setup() = 0;
    virtual void shutdown() = 0;
    virtual void fillCommandBuffers() = 0;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

protected:
    Device m_device;
    SwapChain m_swapChain;
    Texture m_swapChainDepthBuffer;
    VkFormat m_swapChainDepthBufferFormat = VK_FORMAT_UNDEFINED;
    RenderPass m_renderPass;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<Framebuffer> m_framebuffers;

    Statistics m_stats;

    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uniformBufferMemory = VK_NULL_HANDLE;

    float m_sceneBoundingBoxDiameter = 0.f;
    bool m_observerCameraMode = false;

    glm::vec3 m_cameraPosition;
    glm::vec3 m_cameraTarget;
    glm::vec3 m_cameraLook;
    glm::vec3 m_cameraUp;

    bool m_leftMouseButtonDown = false;
    bool m_middleMouseButtonDown = false;
    bool m_rightMouseButtonDown = false;
    double m_mousePositionX = 0.0;
    double m_mousePositionY = 0.0;
    double m_lastMousePositionX = 0.0;
    double m_lastMousePositionY = 0.0;
};
