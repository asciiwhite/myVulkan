#pragma once

#include "vulkan/basicrenderer.h"
#include "vulkan/mesh.h"

class SimpleRenderer : public BasicRenderer
{
private:
    bool setup() override;
    void shutdown() override;
    void fillCommandBuffers() override;

    Mesh m_mesh;
};