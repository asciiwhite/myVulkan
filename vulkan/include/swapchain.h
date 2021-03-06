#pragma once

#include "deviceref.h"
#include "imageview.h"

#include <vulkan/vulkan.h>
#include <vector>

class Device;

class SwapChain : public DeviceRef
{
public:
    SwapChain(Device& device);

    void init(VkSurfaceKHR surface);
    bool create(bool vsync = false);
    void destroy();

    uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }
    VkExtent2D getImageExtent() const { return m_extent; }
    VkImageView getImageView(uint32_t imageViewId) const { return m_imageViews[imageViewId].imageView(); }
    VkFormat getImageFormat() const { return m_surfaceFormat.format; }

    VkSemaphore getImageAvailableSemaphore() const { return m_semaphores[m_currentImageId].first; }
    VkSemaphore getRenderFinishedSemaphore() const { return m_semaphores[m_currentImageId].second; }

    bool acquireNextImage(uint32_t& imageId);
    bool present(uint32_t imageId);

private:
    void createImageViews(uint32_t imageCount);
    void createSemaphores(uint32_t imageCount);
    void destroySwapChain(VkSwapchainKHR& swapChain);
    void destroySemaphores();

    uint32_t                        getSwapChainNumImages(VkSurfaceCapabilitiesKHR &surfaceCaps);
    VkImageUsageFlags               getSwapChainUsageFlags(VkSurfaceCapabilitiesKHR &surfaceCaps);
    VkSurfaceTransformFlagBitsKHR   getSwapChainTransform(VkSurfaceCapabilitiesKHR &surfaceCaps);
    VkExtent2D                      getSwapChainExtent(VkSurfaceCapabilitiesKHR &surfaceCaps);
    VkCompositeAlphaFlagBitsKHR     getCompositeAlphaFlags(VkSurfaceCapabilitiesKHR &surfaceCaps);
    VkSurfaceFormatKHR              getSwapChainFormat(std::vector<VkSurfaceFormatKHR> &surfaceFormats);
    VkPresentModeKHR                getSwapChainPresentMode(std::vector<VkPresentModeKHR> &presentModes, bool vsync);

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> m_images;
    std::vector<ImageView> m_imageViews;
    std::vector<std::pair<VkSemaphore, VkSemaphore>> m_semaphores;
    VkExtent2D m_extent = { 0, 0 };
    VkSurfaceFormatKHR m_surfaceFormat = { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    uint32_t m_currentImageId = 0;
};
