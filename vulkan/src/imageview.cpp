#include "imageview.h"
#include "device.h"

#include "vulkanhelper.h"

namespace
{
    VkImageAspectFlags getImageAspect(VkFormat format)
    {
        switch (format)
        {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        default:
            return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }

    VkImageView createImageView(const Device& device, VkImage image, VkFormat format)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.pNext = nullptr;
        viewInfo.format = format;
        viewInfo.components = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        viewInfo.subresourceRange.aspectMask = getImageAspect(format);
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.flags = 0;
        viewInfo.image = image;

        VkImageView imageView;
        VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &imageView));
        return imageView;
    }
}

ImageView::ImageView(const Device& device, VkFormat format, VkImage image)
    : DeviceRef(device)
    , m_imageView(createImageView(device, image, format))
{    
}

ImageView::~ImageView()
{
    destroy(m_imageView);
}

ImageView::ImageView(ImageView&& other)
{
    swap(other);
}

ImageView& ImageView::operator=(ImageView&& other)
{
    swap(other);
    return *this;
}

VkImageView ImageView::imageView() const
{
    return m_imageView;
}

void ImageView::swap(ImageView& other)
{
    DeviceRef::swap(other);
    std::swap(m_imageView, other.m_imageView);
}
