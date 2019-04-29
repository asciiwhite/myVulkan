#pragma once

#include "imagebase.h"
#include "buffer.h"

enum class ImageUsage
{
    Texture                 = int(VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT),
    ColorAttachment         = int(VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
    DepthStencilAttachment  = int(VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT),
};

constexpr ImageUsage operator|(ImageUsage a, ImageUsage b)
{
    return ImageUsage(uint32_t(a) | uint32_t(b));
}

template<ImageUsage Usage, MemoryType Memory>
class Image : public ImageBase
{
protected:
    template<typename T>
    static constexpr bool has(T a, T b) {
        return (uint32_t(a) & uint32_t(b)) == uint32_t(b);
    }

    static constexpr bool is_compatible(ImageUsage U, MemoryType M)
    {
        return has(U, Usage) && has(M, Memory);
    }

public:
    static constexpr ImageUsage usage = Usage;
    static constexpr MemoryType memory = Memory;

    Image() = default;

    template<typename = void>
    Image(const Device& device, VkExtent2D resolution, VkFormat format)
        : ImageBase(device, resolution, format, VkImageUsageFlagBits(Usage))
    {
    }

    template<typename = void>
    Image(const Device& device, uint8_t *pixelData, VkExtent2D resolution, VkFormat format)
        : ImageBase(device, pixelData, resolution, format, VkImageUsageFlagBits(Usage))
    {
    }

    template<ImageUsage U, MemoryType M>
    Image(Image<U, M>&& other)
    {
        static_assert(is_compatible(U, M));
        swap(other);
    }

    template<ImageUsage U, MemoryType M>
    Image& operator=(Image<U, M>&& other)
    {
        static_assert(is_compatible(U, M));
        swap(other);
        return *this;
    }
};

using Texture = Image<ImageUsage::Texture, MemoryType::DeviceLocal>;
using ColorAttachment = Image<ImageUsage::ColorAttachment | ImageUsage::Texture, MemoryType::DeviceLocal>;
using DepthStencilAttachment= Image<ImageUsage::DepthStencilAttachment | ImageUsage::Texture, MemoryType::DeviceLocal>;
