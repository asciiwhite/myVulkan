#pragma once

#include "handles.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

class PipelineLayout
{
public:
    void init(VkDevice device, const std::vector<VkDescriptorSetLayout>& layouts = {}, const std::vector<VkPushConstantRange>& pushConstants = {});
    void destroy();

    VkPipelineLayout getVkPipelineLayout() const { return m_pipelineLayout; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
};

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

class VertexBuffer;
class GraphicsPipeline;

using PipelineHandle = std::shared_ptr<GraphicsPipeline>;

class GraphicsPipeline
{
public:
    ~GraphicsPipeline();

    VkPipeline getVkPipeline() const { return m_pipeline; }

    static PipelineHandle getPipeline(
        VkDevice device,
        VkRenderPass renderPass,
        VkPipelineLayout layout,
        const PipelineSettings& settings,
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages,
        const VertexBuffer* vertexbuffer = nullptr);

    static void release(PipelineHandle& pipeline);

private:
    bool init(VkDevice device,
        VkRenderPass renderPass,
        VkPipelineLayout layout,
        const PipelineSettings& settings,
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages,
        const VertexBuffer* vertexbuffer = nullptr);

    void destroy();

    VkDevice m_device = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    using PipelineMap = std::unordered_map<size_t, PipelineHandle>;
    static PipelineMap m_createdPipelines;
};