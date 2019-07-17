#pragma once

#include "swapchain.h"
#include "device.h"
#include "image.h"
#include "buffer.h"
#include "imagepool.h"
#include "commandbuffer.h" 

#include "../utils/camerainputhandler.h"
#include "../utils/statistics.h"
#include "../utils/glm.h"
#include "../utils/gui.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <functional>

struct GLFWwindow;

class BasicRenderer
{
public:
    BasicRenderer();

    bool init(GLFWwindow* window);
    void destroy();

    bool resize(uint32_t width, uint32_t height);
    void mouseButton(int button, int action, int mods);
    void mouseMove(double x, double y);

    virtual void update();
    void draw();

protected:
    using DrawFunc = std::function<void(CommandBuffer&)>;
    void fillCommandBuffer(CommandBuffer& commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, const DrawFunc&);
    void setCameraFromBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& lookDir);
    void setClearColor(VkClearColorValue clearColor);
    const VkClearColorValue& clearColor() const;
    void updateMVPUniform();
    void waitForAllFrames() const;
    virtual void createGUIContent() {};

    struct BaseFrameResources
    {
        CommandBufferPtr graphicsCommandBuffer;
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
    VkRenderPass m_swapchainRenderPass;
    Statistics m_stats;
    ImagePool m_imagePool;

    struct CameraParameter
    {
        glm::mat4x4 mvp;
        glm::vec4 pos;
        float pixelsPerRadians;
    };
    UniformBuffer m_cameraUniformBuffer;
    CameraParameter * m_mappedCameraUniformBuffer = nullptr;

private:
    std::vector<BaseFrameResources> m_frameResources;
    std::vector<VkFramebuffer> m_framebuffers;

    bool createInstance();
    bool createDevice();
    bool createSwapChain();
    bool createFrameResources(uint32_t numFrames);
    bool createSwapChainFramebuffers();

    void destroyFramebuffers();
    void destroyFrameResources();

    virtual bool setup() = 0;
    virtual void shutdown() = 0;
    virtual bool postResize() { return true; };

    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    DepthStencilAttachment m_swapChainDepthAttachment;
    VkFormat m_swapChainDepthBufferFormat = VK_FORMAT_UNDEFINED;
    VkClearColorValue m_clearColor = {0.1f, 0.1f, 0.1f, 0.0f};

protected:
    std::unique_ptr<GUI> m_gui;
    CameraInputHandler m_cameraHandler;
    MouseInputHandler m_inputHandler;
};
