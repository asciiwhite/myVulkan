#pragma once

#include "basicrenderer.h"
#include "texturearraymesh.h"

class SimpleRenderer : public BasicRenderer
{
private:
    bool setup() override;
    void render(const FrameData& frameData) override;
    void shutdown() override;
    void fillCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer);
    void createGUIContent() override;

    TextureArrayMesh m_mesh;
};