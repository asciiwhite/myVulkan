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

    void recreateDoFPipeline();

    VkDescriptorSetLayout m_cameraDescriptorSetLayout = VK_NULL_HANDLE;
    DescriptorSet m_cameraUniformDescriptorSet;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    std::string meshFilename;
    std::unique_ptr<Mesh> m_mesh;

    enum eBlitTechnique {
        COC,
        BOKEH,
        DOWNSAMPLE,
        COMBINE_COC,
        COMBINE_DOF,
        COPY_SWAPCHAIN,
        COUNT
    };

    struct BlitPass {
        Shader shader;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
    };

    BlitPass m_blitPasses[eBlitTechnique::COUNT];

    struct BlitPassDescription {
        BlitPass* blitPass = nullptr;
        DescriptorSet destriptorSet;
        VkFramebuffer frameBuffer = VK_NULL_HANDLE;
        VkExtent2D frameBufferExtent = { 0,0 };
        VkFormat frameBufferFormat = VK_FORMAT_UNDEFINED;
    };
    
    struct DofParameter
    {
        float nearPlane = 0.f;
        float farPlane = 1.f;
        float focusDistance = 1.3f;
        float focusRange = 1.2f;
        float bokehRadius = 4.0f;
    };

    using ColorImageHandle = ImagePool::ImageHandle<ColorAttachment>;
    using DepthImageHandle = ImagePool::ImageHandle<DepthStencilAttachment>;

    std::pair<Renderer::ColorImageHandle, Renderer::DepthImageHandle> renderScenePass(VkCommandBuffer commandBuffer, VkExtent2D extend);
    bool createPlitPasses();
    void destroyPlitPasses();
    void setupBlitPipelines();
    void destroyBlitPipelines();
    void addBlitPipeline(VkExtent2D extent, VkFormat format, eBlitTechnique blitTechnique);
    ColorImageHandle renderBlitPass(VkCommandBuffer commandBuffer, BlitPassDescription& passDescr, const std::vector<VkImageView>& attachments);
    void blitAttachment(VkCommandBuffer commandBuffer, const std::vector<VkImageView>& attachments, BlitPassDescription& blitPass);
    bool createBlitPass(BlitPass& pass, VkRenderPass renderPass, const char* fragmentShaderFilename, const std::vector<VkDescriptorSetLayoutBinding>& additionalBindings = {}, bool alphaBlend = false);

    std::vector<BlitPassDescription> m_blitPassDescriptions;
    VkRenderPass m_sceneRenderPass = VK_NULL_HANDLE;
    VkRenderPass m_colorBlitRenderPass = VK_NULL_HANDLE;
    VkRenderPass m_cocBlitRenderPass = VK_NULL_HANDLE;
    VkRenderPass m_combineBlitRenderPass = VK_NULL_HANDLE;
    VkFramebuffer m_sceneFrameBuffer = VK_NULL_HANDLE;
    VkSampler m_clampToEdgeSampler = VK_NULL_HANDLE;
    VkFormat m_cocBufferFormat = VK_FORMAT_R16_SFLOAT;
    bool m_enableDoF = true;
    bool m_showCoC = false;
    UniformBuffer m_doFParameterUB;
    DofParameter m_doFParameter;

    ImagePool m_imagePool;
};
