#pragma once

#include <vulkan/vulkan.h>

VkPipelineStageFlags getPipelineStageFlags(VkAccessFlags access);

VkImageMemoryBarrier createImageMemoryBarrier(VkImage image, VkFormat imageFormat, VkImageLayout oldLayout, VkImageLayout newLayout);

VkBufferMemoryBarrier createBufferMemoryBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);