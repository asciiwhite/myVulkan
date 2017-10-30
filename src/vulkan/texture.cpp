#include "texture.h"
#include "vulkanhelper.h"
#include "device.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstring>

bool Texture::loadFromFile(Device* device, const std::string& filename)
{
    m_device = device;

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        printf("Error: could not load texture %s, reason: %s\n", filename.c_str(), stbi_failure_reason());
        return false;
    }

    const uint32_t imageSize = texWidth * texHeight * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    device->createBuffer(imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device->getVkDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
    std::memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device->getVkDevice(), stagingBufferMemory);

    stbi_image_free(pixels);

    device->createImage(texWidth, texHeight,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_image, m_imageMemory);

    device->transitionImageLayout(m_image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    device->copyBufferToImage(stagingBuffer, m_image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    device->transitionImageLayout(m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device->getVkDevice(), stagingBuffer, nullptr);
    vkFreeMemory(device->getVkDevice(), stagingBufferMemory, nullptr);

    device->createImageView(m_image, VK_FORMAT_R8G8B8A8_UNORM, m_imageView);

    return true;
}

void Texture::destroy()
{
    vkDestroyImageView(m_device->getVkDevice(), m_imageView, nullptr);
    m_imageView = VK_NULL_HANDLE;

    vkDestroyImage(m_device->getVkDevice(), m_image, nullptr);
    m_image = VK_NULL_HANDLE;

    vkFreeMemory(m_device->getVkDevice(), m_imageMemory, nullptr);
    m_imageMemory = VK_NULL_HANDLE;
}
