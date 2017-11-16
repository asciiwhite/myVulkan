#pragma once

#include <vulkan/vulkan.h>
#include <string>

class Device;

class Texture
{
public:
    bool loadFromFile(Device* device, const std::string& filename);
    void createDepthBuffer(Device* device, const VkExtent2D& extend, VkFormat format);
    void destroy();

    VkImageView getImageView() const { return m_imageView; }

private:
    void createImageView();

    Device* m_device = nullptr;

    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
};
