#pragma once

#include "basicrenderer.h"
#include "shader.h"
#include "vertexbuffer.h"
#include "descriptorset.h"
#include "graphicspipeline.h"
#include "buffer.h"

class Renderer : public BasicRenderer
{
private:
    bool setup() override;
    void render(const FrameData& frameData) override;
    void shutdown() override;
    void fillCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer);

    void setupCameraDescriptorSet();
    void setupParticleVertexBuffer();
    void setupGraphicsPipeline();
    void setupComputePipeline();
    bool createComputeCommandBuffer();
    void buildComputeCommandBuffer(VkCommandBuffer commandBuffer);
    void renderParticles(VkCommandBuffer commandBuffer) const;
    void createGUIContent() override;

    VertexBuffer m_vertexBuffer;
    Shader m_shader;
    VkPipeline m_graphicsPipeline;
    VkPipelineLayout m_graphicsPipelineLayout;
    DescriptorSetLayout m_cameraDescriptorSetLayout;
    DescriptorSet m_cameraUniformDescriptorSet;

    VkCommandPool m_computeCommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_computeCommandBuffers;
    VkPipeline m_computePipeline;
    VkPipelineLayout m_computePipelineLayout;
    DescriptorSetLayout m_computeDescriptorSetLayout;
    DescriptorSet m_computeDescriptorSet;
    Shader m_computeShader;
    Buffer m_computeInputBuffer;
    struct ComputeInput
    {
        int particleCount;
        float timeDelta;
    };
    ComputeInput* m_computeMappedInputBuffer = nullptr;

    DescriptorPool m_descriptorPool;
};