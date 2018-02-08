#pragma once

#include "basicrenderer.h"
#include "handles.h"
#include "vertexbuffer.h"
#include "descriptorset.h"
#include "pipeline.h"
#include "buffer.h"

class Renderer : public BasicRenderer
{
private:
    bool setup() override;
    void shutdown() override;
    void fillCommandBuffers() override;

    void setupCamera();
    void setupGroundHeightUniform();
    void renderGround(VkCommandBuffer commandBuffer) const;

    VertexBuffer m_vertexBuffer;
    ShaderHandle m_shader;
    PipelineHandle m_pipeline;
    PipelineLayout m_pipelineLayout;

    Buffer m_groundUniformBuffer;

    DescriptorSetLayout m_groundDescriptorSetLayout;
    DescriptorSet m_groundUniformDescriptorSet;

    DescriptorSetLayout m_cameraDescriptorSetLayout;
    DescriptorSet m_cameraUniformDescriptorSet;

    DescriptorPool m_descriptorPool;
};