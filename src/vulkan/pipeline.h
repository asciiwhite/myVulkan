#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include <memory>

class PipelineLayout
{
public:
    void init(VkDevice device, const std::vector<VkDescriptorSetLayout>& layouts = {});
    void destroy();

    VkPipelineLayout getVkPipelineLayout() const { return m_pipelineLayout; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
};

struct PipelineSettings
{
public:
    PipelineSettings(bool alphaBlending);

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

class Pipeline
{
public:
    ~Pipeline();

    VkPipeline getVkPipeline() const { return m_pipeline; }

    static std::shared_ptr<Pipeline> getPipeline(
        VkDevice device,
        VkRenderPass renderPass,
        VkPipelineLayout layout,
        const PipelineSettings& settings,
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages,
        const VertexBuffer* vertexbuffer = nullptr);

    static void release(std::shared_ptr<Pipeline>& pipeline);

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

    using PipelineMap = std::unordered_map<size_t, std::shared_ptr<Pipeline>>;
    static PipelineMap m_createdPipelines;
};