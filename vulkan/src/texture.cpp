#include "texture.h"
#include "vulkanhelper.h"
#include "device.h"
#include "buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstring>
#include <algorithm>

TextureManager::TextureMap TextureManager::m_loadedTextures;

Texture TextureManager::LoadFromFile(Device& device, const std::string& filename)
{
    Texture texture;

    int texWidth, texHeight;
    stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texture.numChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        printf("Error: could not load texture %s, reason: %s\n", filename.c_str(), stbi_failure_reason());
        return texture;
    }

    const uint32_t imageSize = texWidth * texHeight * 4;

    Buffer stagingBuffer = device.createBuffer(imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data = device.mapBuffer(stagingBuffer, imageSize, 0);
    std::memcpy(data, pixels, static_cast<size_t>(imageSize));
    device.unmapBuffer(stagingBuffer);

    stbi_image_free(pixels);

    device.createImage(texWidth, texHeight,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        texture.image, texture.imageMemory);

    device.transitionImageLayout(texture.image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    device.copyBufferToImage(stagingBuffer, texture.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    device.transitionImageLayout(texture.image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    device.destroyBuffer(stagingBuffer);

    device.createImageView(texture.image, VK_FORMAT_R8G8B8A8_UNORM, texture.imageView, VK_IMAGE_ASPECT_COLOR_BIT);

    return texture;
}

Texture TextureManager::Acquire(Device& device, const std::string& textureFilename)
{
    if (m_loadedTextures.count(textureFilename) == 0)
    {
        auto newTexture = LoadFromFile(device, textureFilename);
        if (newTexture.isValid())
        {
            m_loadedTextures[textureFilename] = { 1, newTexture }; // init refcount
        }
        return newTexture;
    }

    auto& entry = m_loadedTextures.at(textureFilename);
    entry.refCount++;
    return entry.texture;
}

void TextureManager::Release(Device& device, Texture& texture)
{
    auto iter = std::find_if(m_loadedTextures.begin(), m_loadedTextures.end(), [=](const TextureMap::value_type& texturePair) { return texturePair.second.texture == texture; });
    assert(iter != m_loadedTextures.end());

    auto& entry = iter->second;
    if (--entry.refCount == 0)
    {
        device.destroyTexture(entry.texture);
        m_loadedTextures.erase(iter);
    }
}
