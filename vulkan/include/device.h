#pragma once

#include "devicedestroy.h"
#include "deviceref.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <array>

struct GraphicsPipelineSettings;
class VertexBuffer;

struct RenderPassAttachmentDescription
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

    VkRenderPass createRenderPass(const std::vector<RenderPassAttachmentDescription>& attachmentDescriptions) const;

    VkFramebuffer createFramebuffer(VkRenderPass renderPass, const std::vector<VkImageView>& attachments, VkExtent2D extent) const;

    VkPipelineLayout createPipelineLayout(const std::vector<VkDescriptorSetLayout>& layouts = {}, const std::vector<VkPushConstantRange>& pushConstants = {}) const;

    VkPipeline createPipeline(VkRenderPass renderPass, VkPipelineLayout layout, const GraphicsPipelineSettings& settings,
        const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
        const std::vector<VkVertexInputAttributeDescription>& attributeDesc,
        const std::vector<VkVertexInputBindingDescription>& bindingDesc) const;

    VkSampler createSampler(bool clampToEdge = false) const;

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    void copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent2D resolution) const;

    VkDescriptorSetLayout createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings) const;

    VkDescriptorPool createDescriptorPool(uint32_t count, const std::vector<VkDescriptorPoolSize>& sizes, bool allowIndividualFreeing = false) const;

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

    VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    const VkPhysicalDeviceProperties& properties() const { return m_deviceProperties; }
    const VkPhysicalDeviceFeatures& features() const { return m_deviceFeatures; }

    template<typename T>
    void destroy(T t) const
    {
        detail::destroy(*this, t);
    }

private:
    bool checkPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    void createCommandPools();

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

    VkPhysicalDeviceProperties m_deviceProperties;
    VkPhysicalDeviceFeatures m_deviceFeatures;
};

template<typename T>
void DeviceRef::destroy(T t) const
{
    if (m_device != nullptr)
        device().destroy(t);
}
