#pragma once

#include "basicrenderer.h"
#include "mesh.h"

class SimpleRenderer : public BasicRenderer
{
private:
    bool setup() override;
    void render(uint32_t imageId) override;
    void shutdown() override;
    void fillCommandBuffers() override;

    Mesh m_mesh;
};