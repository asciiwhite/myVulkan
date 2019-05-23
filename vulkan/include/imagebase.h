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
    VkFormat format() const;
    VkExtent2D resolution() const;

    bool transpareny() const;
    void setTranspareny(bool);

    void setLayout(VkImageLayout layout);

    operator bool() const;
    bool operator==(const ImageBase& rhs) const;

protected:
    ImageBase() = default;
    ImageBase(const Device& device, uint8_t* pixelData, VkExtent2D resolution, VkFormat format, VkImageUsageFlags usage);
    ImageBase(const Device& device, VkExtent2D resolution, VkFormat format, VkImageUsageFlags usage);

    void swap(ImageBase& other);

private:
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    bool m_hasTransparency = false;
    VkExtent2D m_resolution = { 0,0 };
};
