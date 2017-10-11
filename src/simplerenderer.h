#pragma once

#include "vulkan/basicrenderer.h"
#include "vulkan/shader.h"
#include "vulkan/descriptorset.h"
#include "vulkan/texture.h"
#include "vulkan/pipeline.h"
#include "vulkan/vertexbuffer.h"

class SimpleRenderer : public BasicRenderer
{
private:
    bool setup() override;
    void shutdown() override;
    void fillCommandBuffers() override;

    DescriptorSet m_descriptorSet;
    PipelineLayout m_pipelineLayout;
    Pipeline m_pipeline;
    VertexBuffer m_vertexBuffer;
    Shader m_shader;
    Texture m_texture;
    VkSampler m_sampler = VK_NULL_HANDLE;
};