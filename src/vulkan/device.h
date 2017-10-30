#pragma once

#include <vulkan/vulkan.h>

class Device
{
public:
    bool init(VkInstance instance, VkSurfaceKHR surface, bool enableValidationLayers);
    void destroy();

    void createSampler(VkSampler& sampler);
    void createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void createImageView(VkImage image, VkFormat format, VkImageView& imageView);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

    VkDevice getVkDevice() const { return m_device; };
    VkPhysicalDevice getVkPysicalDevice() const { return m_physicalDevice; };
    VkQueue getPresentationQueue() const { return m_presentQueue; };
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; };
    VkCommandPool getCommandPool() const { return m_commandPool; };

private:
    bool checkPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    void createCommandPool();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    uint32_t m_presentQueueFamilyIndex = UINT32_MAX;
    uint32_t m_graphicsQueueFamilyIndex = UINT32_MAX;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
};