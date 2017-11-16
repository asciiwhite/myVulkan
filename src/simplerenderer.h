#pragma once

#include "vulkan/basicrenderer.h"
#include "vulkan/shader.h"
#include "vulkan/descriptorset.h"
#include "vulkan/pipeline.h"
#include "vulkan/mesh.h"

class SimpleRenderer : public BasicRenderer
{
public:
    void update() override;

private:
    bool setup() override;
    void shutdown() override;
    void fillCommandBuffers() override;
    void resized() override;
    void updateMVP();

    DescriptorSet m_descriptorSet;
    PipelineLayout m_pipelineLayout;
    Pipeline m_pipeline;
    Shader m_shader;
    VkSampler m_sampler = VK_NULL_HANDLE;

    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uniformBufferMemory = VK_NULL_HANDLE;

    Mesh m_mesh;
};