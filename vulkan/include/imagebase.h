#pragma once

#include "deviceref.h"
#include "noncopyable.h"

#include <vulkan/vulkan.h>

class Device;

class ImageBase : public DeviceRef, NonCopyable
{
public:
    ~ImageBase();

    VkImage image() const;
    VkImageView imageView() const;
    VkDeviceMemory memory() const;

    bool hasTranspareny() const;

    operator bool() const;
    bool operator==(const ImageBase& rhs) const;

protected:
    ImageBase() = default;
    ImageBase(const Device& device, uint8_t* pixelData, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
    ImageBase(const Device& device, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);

    void swap(ImageBase& other);

private:
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    int m_numChannels = 0;
};
