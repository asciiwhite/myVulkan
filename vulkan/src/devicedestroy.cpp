#include "device.h"

namespace detail
{
    void destroy(const Device& device, Buffer& buffer)
    {
        device.destroyBuffer(buffer);
    }

    void destroy(const Device& device, VkSampler& sampler)
    {
        device.destroySampler(sampler);
    }

    void destroy(const Device& device, VkPipelineLayout& layout)
    {
        device.destroyPipelineLayout(layout);
    }

    void destroy(const Device& device, VkPipeline& pipeline)
    {
        device.destroyPipeline(pipeline);
    }

    void destroy(const Device& device, Texture& texture)
    {
        device.destroyTexture(texture);
    }

    void destroy(const Device& device, VkRenderPass& renderpass)
    {
        device.destroyRenderPass(renderpass);
    }

    void destroy(const Device& device, VkFramebuffer& framebuffer)
    {
        device.destroyFramebuffer(framebuffer);
    }

    void destroy(const Device& device, VkDescriptorSetLayout& layout)
    {
        device.destroyDescriptorSetLayout(layout);
    }

    void destroy(const Device& device, VkDescriptorPool& pool)
    {
        device.destroyDescriptorPool(pool);
    }
}

