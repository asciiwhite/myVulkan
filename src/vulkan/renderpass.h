#pragma once

#include <vulkan/vulkan.h>

class Device;

class RenderPass
{
public:
    bool init(Device* device, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat);
    void destroy();

    VkRenderPass getVkRenderPass() const { return m_renderPass; }

private:
    Device* m_device;
    VkRenderPass m_renderPass;
};