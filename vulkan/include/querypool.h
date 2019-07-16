#pragma once

#include "deviceref.h"

#include <vulkan/vulkan.h>
#include <chrono>

class QueryPool : public DeviceRef
{
public:
    QueryPool(const Device& device);

    void init();
    void destroy();

    void begin(VkCommandBuffer commandBuffer) const;
    void end(VkCommandBuffer commandBuffer) const;

    std::chrono::microseconds duration() const;

private:
    VkQueryPool m_queryPool = VK_NULL_HANDLE;
};
