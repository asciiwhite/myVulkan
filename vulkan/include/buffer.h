#pragma once

#include <vulkan/vulkan.h>

struct Buffer
{
public:
    operator VkBuffer() const { return buffer; }
    bool isValid() const { return buffer != VK_NULL_HANDLE; }

    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};