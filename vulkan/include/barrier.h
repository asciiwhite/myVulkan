#pragma once

#include <vulkan/vulkan.h>

VkPipelineStageFlags getPipelineStageFlags(VkAccessFlags access);

VkImageMemoryBarrier createImageMemoryBarrier(VkImage image, VkFormat imageFormat, VkImageLayout oldLayout, VkImageLayout newLayout);