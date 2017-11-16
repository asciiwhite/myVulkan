#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class Framebuffer
{
public:
    bool init(VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView>& attachments, VkExtent2D extent);
    void destroy();

    VkFramebuffer getVkFramebuffer() const { return m_framebuffer; }

private:
    VkDevice m_device;
    VkFramebuffer m_framebuffer;
};