#pragma once

#include "vulkan/basicrenderer.h"
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

    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uniformBufferMemory = VK_NULL_HANDLE;

    Mesh m_mesh;
};