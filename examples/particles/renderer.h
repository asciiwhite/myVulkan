#pragma once

#include "basicrenderer.h"
#include "shader.h"
#include "vertexbuffer.h"
#include "descriptorset.h"
#include "graphicspipeline.h"
#include "buffer.h"

class Renderer : public BasicRenderer
{
private:
    bool setup() override;
    void render(const FrameData& frameData) override;
    void shutdown() override;
    
    void setupCameraDescriptorSet();
    void setupParticleVertexBuffer();
    void setupGraphicsPipeline();
    void setupComputePipeline();
    bool createComputeCommandBuffer();
    void buildComputeCommandBuffer(VkCommandBuffer commandBuffer);
    void renderParticles(VkCommandBuffer commandBuffer) const;
    void updateParticleCount();
    void createGUIContent() override;

    std::unique_ptr<VertexBuffer> m_vertexBuffer;
    Shader m_shader;
    VkPipeline m_graphicsPipeline;
    VkPipelineLayout m_graphicsPipelineLayout;
    VkDescriptorSetLayout m_cameraDescriptorSetLayout = VK_NULL_HANDLE;
    DescriptorSet m_cameraUniformDescriptorSet;

    VkCommandPool m_computeCommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_computeCommandBuffers;
    VkPipeline m_computePipeline;
    VkPipelineLayout m_computePipelineLayout;
    VkDescriptorSetLayout m_computeDescriptorSetLayout = VK_NULL_HANDLE;
    DescriptorSet m_computeDescriptorSet;
    Shader m_computeShader;
    UniformBuffer m_computeInputBuffer;
    struct ComputeInput
    {
        int particleCount;
        float timeDeltaInSeconds;
        float particleLifetimeInSeconds;
        float particleSpeed;
        float gravityForce;
        float collisionDamping;
        glm::vec2 emitterPos;
    };
    ComputeInput* m_computeMappedInputBuffer = nullptr;

    int m_particleCount = 0u;
    int m_particlesPerSecond = 2000u;
    float m_particleLifetimeInSeconds = 4.f;
    float m_particleSpeed = 10.f;
    glm::vec2 m_emitterPosition = { 0.f, 8.0f };
    
    uint32_t m_groupCount = 0u;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
};