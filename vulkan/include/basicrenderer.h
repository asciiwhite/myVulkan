#pragma once

#include "swapchain.h"
#include "device.h"
#include "framebuffer.h"
#include "texture.h"
#include "buffer.h"

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
    void submitCommandBuffer(VkCommandBuffer commandBuffer);
    void setCameraFromBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& lookDir);
    void updateMVPUniform();

    struct BaseFrameResources
    {
        VkCommandBuffer graphicsCommandBuffer;
        VkFence frameCompleteFence;
    };

    struct FrameData
    {
        BaseFrameResources& resources;
        VkFramebuffer framebuffer;
    };

    virtual void render(const FrameData& frameData) = 0;

    uint32_t m_frameResourceId = 0;
    uint32_t m_frameResourceCount = 0;
    
    Device m_device;
    SwapChain m_swapChain;
    VkRenderPass m_renderPass;
    Buffer m_cameraUniformBuffer;
    Statistics m_stats;

private:
    std::vector<BaseFrameResources> m_frameResources;
    std::vector<Framebuffer> m_framebuffers;

    bool createInstance();
    bool createDevice();
    bool createSwapChain();
    bool createFrameResources(uint32_t numFrames);
    bool createSwapChainFramebuffers();

    void destroyFramebuffers();
    void destroyFrameResources();

    virtual bool setup() = 0;
    virtual void shutdown() = 0;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    Texture m_swapChainDepthBuffer;
    VkFormat m_swapChainDepthBufferFormat = VK_FORMAT_UNDEFINED;

    // camera stuff
protected:
    float m_sceneBoundingBoxDiameter = 0.f;

    glm::vec3 m_cameraPosition;
    glm::vec3 m_cameraTarget;
    glm::vec3 m_cameraLook;
    glm::vec3 m_cameraUp;

private:
    bool m_observerCameraMode = true;
    bool m_leftMouseButtonDown = false;
    bool m_middleMouseButtonDown = false;
    bool m_rightMouseButtonDown = false;
    double m_mousePositionX = 0.0;
    double m_mousePositionY = 0.0;
    double m_lastMousePositionX = 0.0;
    double m_lastMousePositionY = 0.0;
};
