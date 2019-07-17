#pragma once

#include "buffer.h"
#include "image.h"
#include "shader.h"
#include "descriptorset.h"
#include "deviceref.h"

struct GUIResources
{
    struct FrameResources
    {
        CPUAttributeBuffer vertices;
        CPUIndexBuffer indices;
    };

    Texture image;
    Shader shader;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    DescriptorSet descriptorSet;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    std::vector<FrameResources> frameResources;
};

class CommandBuffer;
class Statistics;
class Device;
struct MouseInputState;

class GUI : public DeviceRef
{
public:
    GUI(Device &device);
    ~GUI();

    void setup(size_t resource_count, uint32_t width, uint32_t height, VkRenderPass renderPass);
    void onResize(uint32_t width, uint32_t height);

    void startFrame(const Statistics& stats, const MouseInputState& mouseState);
    void draw(uint32_t resource_index, CommandBuffer& commandBuffer);

private:
    GUIResources m_resources;

    template<BufferUsage Usage>
    void resizeBufferIfNecessary(Buffer<Usage, MemoryType::CpuVisible>& buffer, int entryCount, int sizeOfEntry) const
    {
        const auto bufferSize = sizeOfEntry * entryCount;
        if (buffer.size() < bufferSize)
        {
            buffer = Buffer<Usage, MemoryType::CpuVisible>(device(), static_cast<uint64_t>(bufferSize));
        }
    }

    void drawFrameData(VkCommandBuffer commandBuffer, GUIResources::FrameResources &drawingResources);
    void createTexture();
    void createDescriptorResources();
    bool createGraphicsPipeline(VkRenderPass renderPass);
};
