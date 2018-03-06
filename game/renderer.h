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

    void setupCameraDescriptorSet();
    void setupCubeVertexBuffer();
    void setupPipeline();
    void renderGround(VkCommandBuffer commandBuffer) const;

    VertexBuffer m_vertexBuffer;
    ShaderHandle m_shader;
    PipelineHandle m_pipeline;
    PipelineLayout m_pipelineLayout;

    DescriptorSetLayout m_cameraDescriptorSetLayout;
    DescriptorSet m_cameraUniformDescriptorSet;

    DescriptorPool m_descriptorPool;
};