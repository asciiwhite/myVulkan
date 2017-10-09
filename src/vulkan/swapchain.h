#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class Device;

class SwapChain
{
public:
    void init(VkInstance instance, VkSurfaceKHR surface, const Device& device);
    bool create(uint32_t width, uint32_t height, bool vsync = false);
    void destroy();

    uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }
    VkExtent2D getImageExtent() const { return m_extent; }
    VkImageView getImageView(uint32_t imageViewId) const { return m_imageViews[imageViewId]; }
    VkFormat getImageFormat() const { return m_surfaceFormat.format; }

    const VkSemaphore* getImageAvailableSemaphore() const { return &m_imageAvailableSemaphore; }
    const VkSemaphore* getRenderFinishedSemaphore() const { return &m_renderFinishedSemaphore; }

    bool acquireNextImage(uint32_t& imageId);
    bool present(uint32_t imageId);

private:
    void createImageViews(uint32_t imageCount);
    void createSemaphores();
    void destroySwapChain(VkSwapchainKHR& swapChain);

    uint32_t                        getSwapChainNumImages(VkSurfaceCapabilitiesKHR &surfaceCaps);
    VkImageUsageFlags               getSwapChainUsageFlags(VkSurfaceCapabilitiesKHR &surfaceCaps);
    VkSurfaceTransformFlagBitsKHR   getSwapChainTransform(VkSurfaceCapabilitiesKHR &surfaceCaps);
    VkExtent2D                      getSwapChainExtent(VkSurfaceCapabilitiesKHR &surfaceCaps);
    VkCompositeAlphaFlagBitsKHR     getCompositeAlphaFlags(VkSurfaceCapabilitiesKHR &surfaceCaps);
    VkSurfaceFormatKHR              getSwapChainFormat(std::vector<VkSurfaceFormatKHR> &surfaceFormats);
    VkPresentModeKHR                getSwapChainPresentMode(std::vector<VkPresentModeKHR> &presentModes, bool vsync);

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
    VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore m_renderFinishedSemaphore = VK_NULL_HANDLE;
    VkExtent2D m_extent = { 0, 0 };
    VkSurfaceFormatKHR m_surfaceFormat = { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
};