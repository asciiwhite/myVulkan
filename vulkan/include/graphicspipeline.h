#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

struct PipelineSettings
{
public:
    PipelineSettings();

    PipelineSettings& setPrimitiveTopology(VkPrimitiveTopology topology);
    PipelineSettings& setAlphaBlending(bool blend);
    PipelineSettings& setCullMode(VkCullModeFlags mode);

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

class GraphicsPipeline
{
public:
    static VkPipeline Acquire(
        Device& device,
        VkRenderPass renderPass,
        VkPipelineLayout layout,
        const PipelineSettings& settings,
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages,
        const VertexBuffer* vertexbuffer = nullptr);

    static void Release(Device& device, VkPipeline& pipeline);

private:

    struct PipelineData
    {
        uint64_t refCount;
        VkPipeline pipeline;
    };

    using PipelineMap = std::unordered_map<size_t, PipelineData>;
    static PipelineMap m_createdPipelines;
};