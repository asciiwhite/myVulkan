#include "framebuffer.h"
#include "vulkanhelper.h"

bool Framebuffer::init(VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView>& attachments, VkExtent2D extent)
{
    m_device = device;

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = &attachments.front();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_framebuffer));

    return true;
}

void Framebuffer::destroy()
{ 
    vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
    m_framebuffer = VK_NULL_HANDLE;
}
