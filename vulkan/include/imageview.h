#pragma once

#include "deviceref.h"

#include <vulkan/vulkan.h>

class Device;

class ImageView : public DeviceRef
{
public:
    ~ImageView();

    VkImageView imageView() const;
    
    ImageView() = default;
    ImageView(const Device& device, VkFormat format, VkImage image);

    ImageView(ImageView&& other);
    ImageView& operator=(ImageView&& other);

protected:
    void swap(ImageView& other);

private:
    VkImageView m_imageView = VK_NULL_HANDLE;
};
