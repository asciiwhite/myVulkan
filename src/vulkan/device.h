#pragma once

#include <vulkan/vulkan.h>

class Device
{
public:
    bool init(VkInstance instance, VkSurfaceKHR surface, bool enableValidationLayers);
    void destroy();

    VkDevice getVkDevice() const { return m_device; };
    VkPhysicalDevice getVkPysicalDevice() const { return m_physicalDevice; };
    VkQueue getPresentationQueue() const { return m_presentQueue; };
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; };
    VkCommandPool getCommandPool() const { return m_commandPool; };

private:
    bool checkPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    void createCommandPool();

    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    uint32_t m_presentQueueFamilyIndex = UINT32_MAX;
    uint32_t m_graphicsQueueFamilyIndex = UINT32_MAX;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
};