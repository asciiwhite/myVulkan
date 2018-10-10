#include "device.h"

#include "buffer.h"
#include "texture.h"

namespace detail
{
    void destroy(const Device& device, Buffer& buffer)
    {
        vkDestroyBuffer(device, buffer.buffer, nullptr);
        vkFreeMemory(device, buffer.memory, nullptr);
    }

    void destroy(const Device& device, Texture& texture)
    {
        vkDestroyImageView(device, texture.imageView, nullptr);
        vkDestroyImage(device, texture.image, nullptr);
        vkFreeMemory(device, texture.imageMemory, nullptr);
    }

    void destroy(const Device& device, VkSampler& sampler)
    {
        vkDestroySampler(device, sampler, nullptr);
    }

    void destroy(const Device& device, VkPipelineLayout& layout)
    {
        vkDestroyPipelineLayout(device, layout, nullptr);
    }

    void destroy(const Device& device, VkPipeline& pipeline)
    {
        vkDestroyPipeline(device, pipeline, nullptr);
    }

    void destroy(const Device& device, VkRenderPass& renderpass)
    {
        vkDestroyRenderPass(device, renderpass, nullptr);
    }

    void destroy(const Device& device, VkFramebuffer& framebuffer)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    void destroy(const Device& device, VkDescriptorSetLayout& layout)
    {
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }

    void destroy(const Device& device, VkDescriptorPool& pool)
    {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
}

