#pragma once

#include <vulkan/vulkan.h>
#include <vector>

struct Buffer;

class Device
{
public:
    bool init(VkInstance instance, VkSurfaceKHR surface, bool enableValidationLayers);
    void destroy();

    Buffer createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const;
    void* mapBuffer(const Buffer& buffer, uint64_t size = VK_WHOLE_SIZE, uint64_t offset = 0u) const;
    void unmapBuffer(const Buffer& buffer) const;
    void destroyBuffer(Buffer& buffer) const;

    VkRenderPass createRenderPass(VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat) const;
    void destroyRenderPass(VkRenderPass& renderPass) const;

    VkFramebuffer createFramebuffer(VkRenderPass renderPass, const std::vector<VkImageView>& attachments, VkExtent2D extent) const;
    void destroyFramebuffer(VkFramebuffer& framebuffer) const;

    void createSampler(VkSampler& sampler) const;    
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
    void createImageView(VkImage image, VkFormat format, VkImageView& imageView, VkImageAspectFlags aspectFlags) const;

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;

    void transitionImageLayout(VkImage image, VkFormat imageFormat, VkImageLayout oldLayout, VkImageLayout newLayout) const;

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

    operator VkDevice() const { return m_device; }

    VkPhysicalDevice getVkPysicalDevice() const { return m_physicalDevice; };
    VkQueue getPresentationQueue() const { return m_presentQueue; };
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; };
    VkQueue getComputeQueue() const { return m_computeQueue; };

    uint32_t getGraphicsQueueFamilyId() const { return m_graphicsQueueFamilyIndex; };
    uint32_t getComputeQueueFamilyId() const { return m_computeQueueFamilyIndex; };

    VkCommandPool getGraphicsCommandPool() const { return m_graphicsCommandPool; };
    VkCommandPool getComputeCommandPool() const { return m_computeCommandPool; };

private:
    bool checkPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    void createCommandPools();

    VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    static bool hasStencilComponent(VkFormat format);

    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    uint32_t m_presentQueueFamilyIndex = UINT32_MAX;
    uint32_t m_graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t m_computeQueueFamilyIndex = UINT32_MAX;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;
    VkCommandPool m_computeCommandPool = VK_NULL_HANDLE;
};