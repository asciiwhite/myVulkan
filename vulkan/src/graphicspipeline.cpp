#include "graphicspipeline.h"
#include "vulkanhelper.h"
#include "vertexbuffer.h"
#include "../utils/hasher.h"

#include <algorithm>

void PipelineLayout::init(VkDevice device, const std::vector<VkDescriptorSetLayout>& layouts)
{
    m_device = device;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.flags = 0;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = !layouts.empty() ? layouts.data() : nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VK_CHECK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));
}

void PipelineLayout::destroy()
{
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    m_pipelineLayout = VK_NULL_HANDLE;
}

//////////////////////////////////////////////////////////////////////////

VkDynamicState PipelineSettings::dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
};

PipelineSettings::PipelineSettings()
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

PipelineSettings& PipelineSettings::setPrimitiveTopology(VkPrimitiveTopology topology)
{
    inputAssembly.topology = topology;
    return *this;
}

PipelineSettings& PipelineSettings::setAlphaBlending(bool blend)
{
    if (blend)
    {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    }
    else
    {
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    }
    return *this;
}

PipelineSettings& PipelineSettings::setCullMode(VkCullModeFlags mode)
{
    rasterizer.cullMode = mode;
    return *this;
}

//////////////////////////////////////////////////////////////////////////

GraphicsPipeline::PipelineMap GraphicsPipeline::m_createdPipelines;

PipelineHandle GraphicsPipeline::getPipeline(VkDevice device,
    VkRenderPass renderPass,
    VkPipelineLayout layout,
    const PipelineSettings& settings,
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages,
    const VertexBuffer* vertexbuffer)
{
    Hasher hasher;
    hasher.add(renderPass);
    hasher.add(layout);
    hasher.add(settings);
    hasher.add(shaderStages);
    hasher.add(vertexbuffer->getAttributeDescriptions());
    hasher.add(vertexbuffer->getBindingDescriptions());
    const auto pipelineHash = hasher.get();

    if (m_createdPipelines.count(pipelineHash) == 0)
    {
        auto newPipeline = std::make_shared<GraphicsPipeline>();
        if (newPipeline->init(device, renderPass, layout, settings, shaderStages, vertexbuffer))
        {
            m_createdPipelines[pipelineHash] = newPipeline;
        }
        else
        {
            newPipeline.reset();
        }
        return newPipeline;
    }

    return m_createdPipelines[pipelineHash];
}

void GraphicsPipeline::release(PipelineHandle& pipeline)
{
    auto iter = std::find_if(m_createdPipelines.begin(), m_createdPipelines.end(), [=](const PipelineMap::value_type& pipelinePair) { return pipelinePair.second == pipeline; });
    assert(iter != m_createdPipelines.end());

    pipeline.reset();

    if (iter->second.unique())
    {
        m_createdPipelines.erase(iter);
    }
}

bool GraphicsPipeline::init(VkDevice device,
    VkRenderPass renderPass,
    VkPipelineLayout layout,
    const PipelineSettings& settings,
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages,
    const VertexBuffer* vertexbuffer)
{
    assert(!shaderStages.empty());

    m_device = device;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = vertexbuffer ? static_cast<uint32_t>(vertexbuffer->getBindingDescriptions().size()) : 0;
    vertexInputInfo.pVertexBindingDescriptions = vertexbuffer ? &vertexbuffer->getBindingDescriptions()[0] : nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = vertexbuffer ? static_cast<uint32_t>(vertexbuffer->getAttributeDescriptions().size()) : 0;
    vertexInputInfo.pVertexAttributeDescriptions = vertexbuffer ? &vertexbuffer->getAttributeDescriptions()[0] : nullptr;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = &shaderStages[0];
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &settings.inputAssembly;
    pipelineInfo.pViewportState = &settings.viewportState;
    pipelineInfo.pRasterizationState = &settings.rasterizer;
    pipelineInfo.pMultisampleState = &settings.multisampling;
    pipelineInfo.pDepthStencilState = &settings.depthStencil;
    pipelineInfo.pColorBlendState = &settings.colorBlending;
    pipelineInfo.pDynamicState = &settings.dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));

    return true;
}

GraphicsPipeline::~GraphicsPipeline()
{
    destroy();
}

void GraphicsPipeline::destroy()
{
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    m_pipeline = VK_NULL_HANDLE;
}
