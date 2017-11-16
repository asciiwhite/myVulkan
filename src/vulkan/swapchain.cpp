#include "swapchain.h"
#include "device.h"
#include "vulkanhelper.h"

#include <vector>
#include <iostream>

void SwapChain::init(VkInstance instance, VkSurfaceKHR surface, Device& device)
{
    m_instance = instance;
    m_surface = surface;
    m_device = &device;
}

uint32_t SwapChain::getSwapChainNumImages(VkSurfaceCapabilitiesKHR &surfaceCaps)
{
    // Set of images defined in a swap chain may not always be available for application to render to:
    // One may be displayed and one may wait in a queue to be presented
    // If application wants to use more images at the same time it must ask for more images
    uint32_t image_count = surfaceCaps.minImageCount + 1;
    if ((surfaceCaps.maxImageCount > 0) &&
        (image_count > surfaceCaps.maxImageCount))
    {
        image_count = surfaceCaps.maxImageCount;
    }
    return image_count;
}

VkSurfaceFormatKHR SwapChain::getSwapChainFormat(std::vector<VkSurfaceFormatKHR> &surfaceFormats)
{
    // If the list contains only one entry with undefined format
    // it means that there are no preferred surface formats and any can be chosen
    if ((surfaceFormats.size() == 1) &&
        (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
    {
        return{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    }

    // Check if list contains most widely used R8 G8 B8 A8 format
    // with nonlinear color space
    for (VkSurfaceFormatKHR &surface_format : surfaceFormats)
    {
        if (surface_format.format == VK_FORMAT_R8G8B8A8_UNORM)
        {
            return surface_format;
        }
    }

    // Return the first format from the list
    return surfaceFormats[0];
}

VkExtent2D SwapChain::getSwapChainExtent(VkSurfaceCapabilitiesKHR &surfaceCaps)
{
    // Special value of surface extent is width == height == -1
    // If this is so we define the size by ourselves but it must fit within defined confines
    if (surfaceCaps.currentExtent.width == ~0u)
    {
        VkExtent2D swap_chain_extent = { 640, 480 };
        if (swap_chain_extent.width < surfaceCaps.minImageExtent.width) {
            swap_chain_extent.width = surfaceCaps.minImageExtent.width;
        }
        if (swap_chain_extent.height < surfaceCaps.minImageExtent.height) {
            swap_chain_extent.height = surfaceCaps.minImageExtent.height;
        }
        if (swap_chain_extent.width > surfaceCaps.maxImageExtent.width) {
            swap_chain_extent.width = surfaceCaps.maxImageExtent.width;
        }
        if (swap_chain_extent.height > surfaceCaps.maxImageExtent.height) {
            swap_chain_extent.height = surfaceCaps.maxImageExtent.height;
        }
        return swap_chain_extent;
    }

    // Most of the cases we define size of the swap_chain images equal to current window's size
    return surfaceCaps.currentExtent;
}

VkImageUsageFlags SwapChain::getSwapChainUsageFlags(VkSurfaceCapabilitiesKHR &surfaceCaps)
{
    // Color attachment flag must always be supported
    // We can define other usage flags but we always need to check if they are supported
    if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    std::cout << "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by the swap chain!" << std::endl
        << "Supported swap chain's image usages include:" << std::endl
        << (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT ? "    VK_IMAGE_USAGE_TRANSFER_SRC\n" : "")
        << (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT ? "    VK_IMAGE_USAGE_TRANSFER_DST\n" : "")
        << (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT ? "    VK_IMAGE_USAGE_SAMPLED\n" : "")
        << (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT ? "    VK_IMAGE_USAGE_STORAGE\n" : "")
        << (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_COLOR_ATTACHMENT\n" : "")
        << (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n" : "")
        << (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n" : "")
        << (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_INPUT_ATTACHMENT" : "")
        << std::endl;
    return static_cast<VkImageUsageFlags>(-1);
}

VkSurfaceTransformFlagBitsKHR SwapChain::getSwapChainTransform(VkSurfaceCapabilitiesKHR &surfaceCaps)
{
    // Sometimes images must be transformed before they are presented (i.e. due to device's orientation
    // being other than default orientation)
    // If the specified transform is other than current transform, presentation engine will transform image
    // during presentation operation; this operation may hit performance on some platforms
    // Here we don't want any transformations to occur so if the identity transform is supported use it
    // otherwise just use the same transform as current transform
    if (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        return surfaceCaps.currentTransform;
    }
}

VkPresentModeKHR SwapChain::getSwapChainPresentMode(std::vector<VkPresentModeKHR> &presentModes, bool vsync)
{
    // FIFO present mode is always available per spec
    // This mode waits for the vertical blank ("v-sync")
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    // If v-sync is not requested, try to find a mailbox mode
    // It's the lowest latency non-tearing present mode available
    if (!vsync)
    {
        for (size_t i = 0; i < presentModes.size(); i++)
        {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
            {
                swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }
    return swapchainPresentMode;
}

VkCompositeAlphaFlagBitsKHR SwapChain::getCompositeAlphaFlags(VkSurfaceCapabilitiesKHR &surfaceCaps)
{
    // Find a supported composite alpha format (not all devices support alpha opaque)
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // Simply select the first composite alpha format available
    std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (auto& compositeAlphaFlag : compositeAlphaFlags)
    {
        if (surfaceCaps.supportedCompositeAlpha & compositeAlphaFlag)
        {
            compositeAlpha = compositeAlphaFlag;
            break;
        };
    }

    return compositeAlpha;
}

bool SwapChain::create(bool vsync)
{
    VkSwapchainKHR oldSwapchain = m_swapChain;

    // Get physical m_device surface properties and formats
    VkSurfaceCapabilitiesKHR surfCaps;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device->getVkPysicalDevice(), m_surface, &surfCaps));

    // Get available present modes
    uint32_t presentModeCount;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_device->getVkPysicalDevice(), m_surface, &presentModeCount, NULL));
    assert(presentModeCount > 0);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_device->getVkPysicalDevice(), m_surface, &presentModeCount, presentModes.data()));

    // Get list of supported surface formats
    uint32_t formatCount;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_device->getVkPysicalDevice(), m_surface, &formatCount, NULL));
    assert(formatCount > 0);

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_device->getVkPysicalDevice(), m_surface, &formatCount, surfaceFormats.data()));

    m_surfaceFormat = getSwapChainFormat(surfaceFormats);
    m_extent        = getSwapChainExtent(surfCaps);

    if (m_extent.width * m_extent.height == 0)
        return false;

    uint32_t                      imageCount = getSwapChainNumImages(surfCaps);
    VkImageUsageFlags             usage = getSwapChainUsageFlags(surfCaps);
    VkSurfaceTransformFlagBitsKHR transform = getSwapChainTransform(surfCaps);
    VkPresentModeKHR              presentMode = getSwapChainPresentMode(presentModes, vsync);
    VkCompositeAlphaFlagBitsKHR   compositeAlpha = getCompositeAlphaFlags(surfCaps);

    VkSwapchainCreateInfoKHR swapchainCI = {};
    swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.pNext = NULL;
    swapchainCI.surface = m_surface;
    swapchainCI.minImageCount = imageCount;
    swapchainCI.imageFormat = m_surfaceFormat.format;
    swapchainCI.imageColorSpace = m_surfaceFormat.colorSpace;
    swapchainCI.imageExtent = { m_extent.width, m_extent.height };
    swapchainCI.imageUsage = usage;
    swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)transform;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCI.queueFamilyIndexCount = 0;
    swapchainCI.pQueueFamilyIndices = NULL;
    swapchainCI.presentMode = presentMode;
    swapchainCI.oldSwapchain = oldSwapchain;
    // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
    swapchainCI.clipped = VK_TRUE;
    swapchainCI.compositeAlpha = compositeAlpha;

    // Set additional usage flag for blitting from the swapchain images if supported
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(m_device->getVkPysicalDevice(), m_surfaceFormat.format, &formatProps);
    if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)
    {
        swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    if (oldSwapchain)
    {
        vkDeviceWaitIdle(m_device->getVkDevice());
    }

    VK_CHECK_RESULT(vkCreateSwapchainKHR(m_device->getVkDevice(), &swapchainCI, nullptr, &m_swapChain));

    // If an existing swap chain is re-created, destroy the old swap chain
    // This also cleans up all the presentable images
    destroySwapChain(oldSwapchain);

    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(m_device->getVkDevice(), m_swapChain, &imageCount, NULL));

    // Get the swap chain images
    m_images.resize(imageCount);
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(m_device->getVkDevice(), m_swapChain, &imageCount, m_images.data()));

    createImageViews(imageCount);

    destroySemaphores();
    createSemaphores(imageCount);

    return true;
}

void SwapChain::createImageViews(uint32_t imageCount)
{
    // Get the swap chain buffers containing the image and imageview
    m_imageViews.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        m_device->createImageView(m_images[i], m_surfaceFormat.format, m_imageViews[i], VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void SwapChain::createSemaphores(uint32_t imageCount)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    m_semaphores.resize(imageCount);
    for (auto& sems : m_semaphores)
    {
        VK_CHECK_RESULT(vkCreateSemaphore(m_device->getVkDevice(), &semaphoreInfo, nullptr, &sems.first));
        VK_CHECK_RESULT(vkCreateSemaphore(m_device->getVkDevice(), &semaphoreInfo, nullptr, &sems.second));
    }
}

bool SwapChain::acquireNextImage(uint32_t& imageId)
{
    // By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is thrown
    // With that we don't have to handle VK_NOT_READY
    auto imageAcquiredSemaphore = m_semaphores[m_currentImageId].first;
    auto result = vkAcquireNextImageKHR(m_device->getVkDevice(), m_swapChain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &imageId);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return false;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    return true;
}

bool SwapChain::present(uint32_t imageId)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_semaphores[m_currentImageId].second;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapChain;
    presentInfo.pImageIndices = &imageId;

    auto result = vkQueuePresentKHR(m_device->getPresentationQueue(), &presentInfo);

    m_currentImageId = ++m_currentImageId % m_images.size();

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        return false;
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    return true;
}

void SwapChain::destroy()
{
    destroySwapChain(m_swapChain);
    destroySemaphores();
}

void SwapChain::destroySemaphores()
{
    for (auto& sems : m_semaphores)
    {
        vkDestroySemaphore(m_device->getVkDevice(), sems.first, nullptr);
        vkDestroySemaphore(m_device->getVkDevice(), sems.second, nullptr);
    }
    m_semaphores.clear();
}

void SwapChain::destroySwapChain(VkSwapchainKHR& swapChain)
{
    if (swapChain != VK_NULL_HANDLE)
    {
        for (uint32_t i = 0; i < m_imageViews.size(); i++)
        {
            vkDestroyImageView(m_device->getVkDevice(), m_imageViews[i], nullptr);
        }
        vkDestroySwapchainKHR(m_device->getVkDevice(), swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;
    }
}
