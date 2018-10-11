#pragma once

#include <vulkan/vulkan.h>

class Device;

struct Texture;

namespace detail
{
    void destroy(const Device& device, Texture& texture);

    void destroy(const Device& device, VkBuffer buffer);
    void destroy(const Device& device, VkDeviceMemory memory);
    void destroy(const Device& device, VkSampler sampler);
    void destroy(const Device& device, VkPipelineLayout layout);
    void destroy(const Device& device, VkPipeline pipeline);
    void destroy(const Device& device, VkRenderPass renderpass);
    void destroy(const Device& device, VkFramebuffer framebuffer);
    void destroy(const Device& device, VkDescriptorSetLayout layout);
    void destroy(const Device& device, VkDescriptorPool pool);
}
