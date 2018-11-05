#include "simplerenderer.h"
#include "vulkanhelper.h"
#include "imgui.h"
#include "objfileloader.h"

#include <array>

bool SimpleRenderer::setup()
{
    meshFilename = "data/meshes/bunny.obj";
    
    auto meshLoadFunc = [&]()
    {
        MeshDescription meshDesc;
        if (ObjFileLoader::read(meshFilename, meshDesc))
        {
            m_mesh.reset(new Mesh(m_device));
            if (!m_mesh->init(meshDesc, m_cameraUniformBuffer, m_renderPass))
                m_mesh.reset();
            else
                setCameraFromBoundingBox(meshDesc.boundingBox.min, meshDesc.boundingBox.max, glm::vec3(0, 1, 1));
        }

        return [&](auto commandBuffer)
        {
            m_mesh->render(commandBuffer);
        };
    };

    m_meshDrawFunc = meshLoadFunc();

    return m_mesh.get() != nullptr;
}

void SimpleRenderer::shutdown()
{
    m_mesh.reset();
}

void SimpleRenderer::render(const FrameData& frameData)
{
    fillCommandBuffer(frameData.resources.graphicsCommandBuffer, frameData.framebuffer, m_meshDrawFunc);
    submitCommandBuffer(frameData.resources.graphicsCommandBuffer, m_swapChain.getImageAvailableSemaphore(), nullptr, nullptr);
}

void SimpleRenderer::createGUIContent()
{
    const auto baseFileName = meshFilename.substr(meshFilename.find_last_of("/\\") + 1, meshFilename.size());

    ImGui::Begin("Mesh loader", nullptr, ImGuiWindowFlags_NoResize);
    ImGui::Text("File: %s", baseFileName.c_str());
    ImGui::Text("#vertices: %u", m_mesh->numVertices());
    ImGui::Text("#triangles: %u", m_mesh->numTriangles());
    ImGui::Text("#shapes: %u", m_mesh->numShapes());
    ImGui::End();
}