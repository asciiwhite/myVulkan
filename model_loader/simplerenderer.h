#pragma once

#include "basicrenderer.h"
#include "mesh.h"

class SimpleRenderer : public BasicRenderer
{
private:
    bool setup() override;
    void render(const FrameData& frameData) override;
    void shutdown() override;
    void fillCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer);
    void createGUIContent() override;

    std::string meshFilename;
    Mesh m_mesh;
};