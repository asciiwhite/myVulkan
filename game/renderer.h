#pragma once

#include "basicrenderer.h"
#include "handles.h"
#include "vertexbuffer.h"
#include "descriptorset.h"
#include "graphicspipeline.h"
#include "buffer.h"

class Renderer : public BasicRenderer
{
private:
    bool setup() override;
    void render(uint32_t frameId) override;
    void shutdown() override;
    void fillCommandBuffers() override;

    void setupCameraDescriptorSet();
    void setupCubeVertexBuffer();
    void setupGraphicsPipeline();
    void setupComputePipeline();
    void buildComputeCommandBuffer();
    void renderGround(VkCommandBuffer commandBuffer) const;

    VertexBuffer m_vertexBuffer;
    ShaderHandle m_shader;
    PipelineHandle m_graphicsPipeline;
    PipelineLayout m_graphicsPipelineLayout;
    DescriptorSetLayout m_cameraDescriptorSetLayout;
    DescriptorSet m_cameraUniformDescriptorSet;

    VkCommandPool m_computeCommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_computeCommandBuffers;
    VkPipeline m_computePipeline;
    PipelineLayout m_computePipelineLayout;
    DescriptorSetLayout m_computeDescriptorSetLayout;
    DescriptorSet m_computeDescriptorSet;
    ShaderHandle m_computeShader;
    Buffer m_computeInputBuffer;
    struct ComputeInput
    {
        int tilesPerDim;
        float time;
    };
    ComputeInput* m_computeMappedInputBuffer = nullptr;

    VkFence m_computeFence = VK_NULL_HANDLE;
    DescriptorPool m_descriptorPool;
};