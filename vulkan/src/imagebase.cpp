#include "imagebase.h"
#include "vulkanhelper.h"
#include "device.h"
#include "buffer.h"
#include "barrier.h"

#include <cstring>

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

    VkImageLayout getNewImageLayout(VkImageUsageFlags usage)
    {
        return (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    void createImage(const Device& device, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImage& image, VkDeviceMemory& imageMemory)
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &image));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));
        VK_CHECK_RESULT(vkBindImageMemory(device, image, imageMemory, 0));
    }

    void createImageView(const Device& device, VkImage image, VkFormat format, VkImageView& imageView)
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

        VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &imageView));
    }
}

ImageBase::ImageBase(const Device& device, uint8_t* pixelData, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage)
    : DeviceRef(device)
    , m_format(format)
{
    assert(format == VK_FORMAT_R8G8B8A8_UNORM);

    createImage(device, width, height, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT, m_image, m_memory);
    setLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    const uint32_t imageSize = width * height * 4;
    StagingBuffer stagingBuffer(device, imageSize);
    void* data = stagingBuffer.map();
    std::memcpy(data, pixelData, static_cast<size_t>(imageSize));
    stagingBuffer.unmap();

    device.copyBufferToImage(stagingBuffer, m_image, width, height);
    setLayout(getNewImageLayout(usage));
    createImageView(device, m_image, format, m_imageView);
}

ImageBase::ImageBase(const Device& device, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage)
    : DeviceRef(device)
    , m_format(format)
{
    createImage(device, width, height, format, usage, m_image, m_memory);
    setLayout(getNewImageLayout(usage));
    createImageView(device, m_image, format, m_imageView);
}

ImageBase::~ImageBase()
{
    destroy(m_imageView);
    destroy(m_image);
    destroy(m_memory);
}

ImageBase::operator bool() const
{
    return m_image != VK_NULL_HANDLE;
}

bool ImageBase::operator==(const ImageBase& rhs) const
{
    return m_image == rhs.m_image;
}

bool ImageBase::hasTranspareny() const
{
    return m_numChannels == 4;
}

VkImageView ImageBase::imageView() const
{
    return m_imageView;
}

VkImage ImageBase::image() const
{
    return m_image;
}

VkDeviceMemory ImageBase::memory() const
{
    return m_memory;
}

void ImageBase::swap(ImageBase& other)
{
    DeviceRef::swap(other);
    std::swap(m_image, other.m_image);
    std::swap(m_imageView, other.m_imageView);
    std::swap(m_memory, other.m_memory);
    std::swap(m_layout, other.m_layout);
    std::swap(m_format, other.m_format);
    std::swap(m_numChannels, other.m_numChannels);
}

void ImageBase::setLayout(VkImageLayout newLayout)
{
    const auto barrier = createImageMemoryBarrier(m_image, m_format, m_layout, newLayout);
    const auto sourceStage = getPipelineStageFlags(barrier.srcAccessMask);
    const auto destinationStage = getPipelineStageFlags(barrier.dstAccessMask);

    const auto commandBuffer = device().beginSingleTimeCommands();

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    device().endSingleTimeCommands(commandBuffer);
    m_layout = newLayout;
}