#pragma once

#include "buffer.h"
#include "texture.h"
#include "shader.h"
#include "descriptorset.h"

struct GUIResources
{
    struct FrameResources
    {
        struct BufferParam
        {
            Buffer buffer;
            int count = 0;
            void* data = nullptr;
        };
        BufferParam vertices;
        BufferParam indices;
    };

    Texture image;
    Shader shader;
    VkSampler sampler = VK_NULL_HANDLE;
    DescriptorPool descriptorPool;
    DescriptorSetLayout descriptorSetLayout;
    DescriptorSet descriptorSet;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<FrameResources> frameResources;
};

struct MouseInputState
{
    bool buttonIsPressed[3];
    float position[2];
};

class Statistics;
class Device;

class GUI
{
public:
    GUI(Device &device);
    ~GUI();

    void setup(size_t resource_count, uint32_t width, uint32_t height, VkFormat colorAttachmentFormat, VkFormat swapChainDepthBufferFormat);
    void onResize(uint32_t width, uint32_t height);

    void startFrame(const Statistics& stats, const MouseInputState& mouseState);
    void draw(uint32_t resource_index, VkCommandBuffer commandBuffer, VkFramebuffer framebuffer);

private:
    Device &m_device;
    GUIResources m_resources;

    void resizeBufferIfNecessary(GUIResources::FrameResources::BufferParam& bufferParam, VkBufferUsageFlagBits usageFlags, int entryCount, int sizeOfEntry) const;
    void drawFrameData(VkCommandBuffer commandBuffer, GUIResources::FrameResources &drawingResources);
    void createTexture();
    void createDescriptorResources();
    bool createGraphicsPipeline();
};