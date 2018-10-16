#include "graphicspipeline.h"
#include "vulkanhelper.h"
#include "device.h"
#include "../utils/hasher.h"

namespace std
{
    template<>
    struct hash<GraphicsPipelineSettings> : public BitwiseHash<GraphicsPipelineSettings>
    {};

    template<>
    struct hash<VkPipelineShaderStageCreateInfo> : public BitwiseHash<VkPipelineShaderStageCreateInfo>
    {};

    template<>
    struct hash<VkVertexInputBindingDescription> : public BitwiseHash<VkVertexInputBindingDescription>
    {};

    template<>
    struct hash<VkVertexInputAttributeDescription> : public BitwiseHash<VkVertexInputAttributeDescription>
    {};
}

VkDynamicState GraphicsPipelineSettings::dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
};

GraphicsPipelineSettings::GraphicsPipelineSettings()
{
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;

    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.flags = 0;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;
    rasterizer.lineWidth = 1.0f;

    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.minSampleShading = 1.f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.flags = 0;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.flags = 0;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.f;
    depthStencil.maxDepthBounds = 1.f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.flags = 0;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
}

GraphicsPipelineSettings& GraphicsPipelineSettings::setPrimitiveTopology(VkPrimitiveTopology topology)
{
    inputAssembly.topology = topology;
    return *this;
}

GraphicsPipelineSettings& GraphicsPipelineSettings::setAlphaBlending(bool blend)
{
    if (blend)
    {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    }
    else
    {
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    }
    return *this;
}

GraphicsPipelineSettings& GraphicsPipelineSettings::setDepthTesting(bool depth)
{
    depthStencil.depthTestEnable = depth ? VK_TRUE : VK_FALSE;
    return *this;
}

GraphicsPipelineSettings& GraphicsPipelineSettings::setCullMode(VkCullModeFlags mode)
{
    rasterizer.cullMode = mode;
    return *this;
}

//////////////////////////////////////////////////////////////////////////

size_t GraphicsPipelineResourceHandler::CreateResourceKey(VkRenderPass renderPass,
    VkPipelineLayout layout,
    const GraphicsPipelineSettings& settings,
    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
    const std::vector<VkVertexInputAttributeDescription>& attributeDesc,
    const std::vector<VkVertexInputBindingDescription>& bindingDesc)
{
    return Hasher::hashme(renderPass, layout, settings, shaderStages, attributeDesc, bindingDesc);
}

VkPipeline GraphicsPipelineResourceHandler::CreateResource(const Device& device, VkRenderPass renderPass,
    VkPipelineLayout layout,
    const GraphicsPipelineSettings& settings,
    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
    const std::vector<VkVertexInputAttributeDescription>& attributeDesc,
    const std::vector<VkVertexInputBindingDescription>& bindingDesc)
{
    return device.createPipeline(renderPass, layout, settings, shaderStages, attributeDesc, bindingDesc);
}

void GraphicsPipelineResourceHandler::DestroyResource(const Device& device, VkPipeline& pipeline)
{
    device.destroy(pipeline);
}
