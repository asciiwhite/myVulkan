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
    void renderGround(VkCommandBuffer commandBuffer) const;

    VertexBuffer m_vertexBuffer;
    ShaderHandle m_shader;
    PipelineHandle m_graphicsPipeline;
    PipelineLayout m_graphicsPipelineLayout;
    DescriptorSetLayout m_cameraDescriptorSetLayout;
    DescriptorSet m_cameraUniformDescriptorSet;

    DescriptorPool m_descriptorPool;
};