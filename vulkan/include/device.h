#pragma once

#include "devicedestroy.h"
#include "deviceref.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <array>

struct GraphicsPipelineSettings;
class VertexBuffer;

struct RenderPassAttachmentData
{
    VkFormat            format;
    VkAttachmentLoadOp  loadOp;
    VkAttachmentStoreOp storeOp;
    VkImageLayout       initialLayout;
    VkImageLayout       finalLayout;
};

class Device
{
public:
    bool init(VkInstance instance, VkSurfaceKHR surface, bool enableValidationLayers);
    void destroy();

    VkRenderPass createRenderPass(const std::array<RenderPassAttachmentData, 2>& attachmentData) const;

    VkFramebuffer createFramebuffer(VkRenderPass renderPass, const std::vector<VkImageView>& attachments, VkExtent2D extent) const;

    VkPipelineLayout createPipelineLayout(const std::vector<VkDescriptorSetLayout>& layouts = {}, const std::vector<VkPushConstantRange>& pushConstants = {}) const;

    VkPipeline createPipeline(VkRenderPass renderPass, VkPipelineLayout layout, const GraphicsPipelineSettings& settings,
        const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
        const std::vector<VkVertexInputAttributeDescription>& attributeDesc,
        const std::vector<VkVertexInputBindingDescription>& bindingDesc) const;

    VkSampler createSampler() const;

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;

    VkDescriptorSetLayout createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings) const;

    VkDescriptorPool createDescriptorPool(uint32_t count, const std::vector<VkDescriptorPoolSize>& sizes) const;

    void transitionImageLayout(VkImage image, VkFormat imageFormat, VkImageLayout oldLayout, VkImageLayout newLayout) const;

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    operator VkDevice() const { return m_device; }

    VkPhysicalDevice getVkPysicalDevice() const { return m_physicalDevice; };
    VkQueue getPresentationQueue() const { return m_presentQueue; };
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; };
    VkQueue getComputeQueue() const { return m_computeQueue; };

    uint32_t getGraphicsQueueFamilyId() const { return m_graphicsQueueFamilyIndex; };
    uint32_t getComputeQueueFamilyId() const { return m_computeQueueFamilyIndex; };

    VkCommandPool getGraphicsCommandPool() const { return m_graphicsCommandPool; };
    VkCommandPool getComputeCommandPool() const { return m_computeCommandPool; };

    template<typename T>
    void destroy(T t) const
    {
        detail::destroy(*this, t);
    }

private:
    bool checkPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    void createCommandPools();

    VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

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

template<typename T>
void DeviceRef::destroy(T t) const
{
    if (m_device != nullptr)
        device().destroy(t);
}
