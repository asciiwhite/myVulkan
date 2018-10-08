#pragma once

#include "resourcemanager.h"

#include <vulkan/vulkan.h>
#include <vector>

struct GraphicsPipelineSettings
{
public:
    GraphicsPipelineSettings();

    GraphicsPipelineSettings& setPrimitiveTopology(VkPrimitiveTopology topology);
    GraphicsPipelineSettings& setAlphaBlending(bool blend);
    GraphicsPipelineSettings& setDepthTesting(bool depth);
    GraphicsPipelineSettings& setCullMode(VkCullModeFlags mode);

    VkPipelineViewportStateCreateInfo viewportState = {};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    VkPipelineDynamicStateCreateInfo dynamicState = {};

    static VkDynamicState dynamicStates[2];
};

class Device;
class VertexBuffer;

class GraphicsPipelineResourceHandler
{
public:
    using ResourceKey = size_t;
    using ResourceType = VkPipeline;

    static ResourceKey CreateResourceKey(VkRenderPass renderPass,
        VkPipelineLayout layout,
        const GraphicsPipelineSettings& settings,
        const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
        const std::vector<VkVertexInputAttributeDescription>& attributeDesc,
        const std::vector<VkVertexInputBindingDescription>& bindingDesc);

    static ResourceType CreateResource(const Device& device, VkRenderPass renderPass,
        VkPipelineLayout layout,
        const GraphicsPipelineSettings& settings,
        const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
        const std::vector<VkVertexInputAttributeDescription>& attributeDesc,
        const std::vector<VkVertexInputBindingDescription>& bindingDesc);

    static void DestroyResource(const Device& device, ResourceType& resource);
};

using GraphicsPipeline = ResourceManager<GraphicsPipelineResourceHandler>;
