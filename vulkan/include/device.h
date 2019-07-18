#pragma once

#include "devicedestroy.h"
#include "deviceref.h"
#include "types.h"
#include "queue.h"

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

    VkPhysicalDevice vkPysicalDevice() const { return m_physicalDevice; };
    const Queue& presentationQueue() const { return m_presentQueue; };
    const Queue& graphicsQueue() const { return m_graphicsQueue; };
    const Queue& computeQueue() const { return m_computeQueue; };

    const VkPhysicalDeviceProperties& properties() const { return m_deviceProperties; }
    const VkPhysicalDeviceFeatures& features() const { return m_deviceFeatures; }

    CommandBufferPtr createCommandBuffer() const;
    CommandBufferPtr createComputeCommandBuffer() const;

    template<typename T>
    void destroy(T t) const
    {
        detail::destroy(*this, t);
    }

private:
    struct QueueFamilyIds
    {
        uint32_t graphics = UINT32_MAX;
        uint32_t compute = UINT32_MAX;
        uint32_t present = UINT32_MAX;
    };

    bool checkPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, QueueFamilyIds& queueFamilyIds);
    void createCommandPools();

    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;
    VkCommandPool m_computeCommandPool = VK_NULL_HANDLE;

    Queue m_presentQueue;
    Queue m_graphicsQueue;
    Queue m_computeQueue;

    VkPhysicalDeviceProperties m_deviceProperties;
    VkPhysicalDeviceFeatures m_deviceFeatures;
};

template<typename T>
void DeviceRef::destroy(T t) const
{
    if (m_device != nullptr)
        device().destroy(t);
}
