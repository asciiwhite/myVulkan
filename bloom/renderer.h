#pragma once

#include "basicrenderer.h"
#include "shader.h"
#include "vertexbuffer.h"
#include "descriptorset.h"
#include "graphicspipeline.h"
#include "mesh.h"

class Renderer : public BasicRenderer
{
private:
    bool setup() override;
    void render(const FrameData& frameData) override;
    void shutdown() override;

    bool postResize() override;
    void setupCameraDescriptorSet();
    void createGUIContent() override;

    void recreateBlitPipeline();

    VkDescriptorSetLayout m_cameraDescriptorSetLayout = VK_NULL_HANDLE;
    DescriptorSet m_cameraUniformDescriptorSet;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    std::string meshFilename;
    std::unique_ptr<Mesh> m_mesh;

    enum eBlitTechnique {
        COPY,
        COPY_SWAPCHAIN,
        BOX_3x3,
        BOX_4x4,
        BOX_3x3_ADD,
        PREFILTER,
        COUNT
    };

    struct BlitPass {
        Shader shader;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    };

    BlitPass m_blitPasses[eBlitTechnique::COUNT];

    struct BlitPassDescription {
        BlitPass* blitPass = nullptr;
        DescriptorSet destriptorSet;
        VkFramebuffer frameBuffer = VK_NULL_HANDLE;
        VkExtent2D frameBufferExtent = { 0,0 };
    };
    
    struct BloomParameter
    {
        float preFilterThreshold = 0.5f;
        float intensity = 1.5f;
    };

    void createPlitPasses();
    void destroyPlitPasses();
    void setupBlitPipelines();
    void destroyBlitPipelines();
    void addBlitPipeline(VkExtent2D extent, eBlitTechnique blitTechnique);
    void blitAttachment(VkCommandBuffer commandBuffer, const std::vector<VkImageView>& attachments, BlitPassDescription& blitPass);
    bool createBlitPass(BlitPass& pass, VkRenderPass renderPass, const char* fragmentShaderFilename, const std::vector<VkDescriptorSetLayoutBinding>& additionalBindings = {}, bool alphaBlend = false);

    std::vector<BlitPassDescription> m_blitPassDescriptions;
    VkRenderPass m_sceneRenderPass = VK_NULL_HANDLE;
    VkRenderPass m_blitRenderPass = VK_NULL_HANDLE;
    VkFramebuffer m_sceneFrameBuffer = VK_NULL_HANDLE;
    VkSampler m_clampToEdgeSampler = VK_NULL_HANDLE;
    bool m_enableBloom = true;
    bool m_showDebug = false;
    bool m_useBoxFilter = true;
    bool m_useUpsampling = true;
    bool m_useDownsampling = true;
    int m_numDownsampleLoops = 4;
    const int m_maxDownsampleLoops = 6;
    UniformBuffer m_bloomParameterUB;
    BloomParameter m_bloomParameter;


};
