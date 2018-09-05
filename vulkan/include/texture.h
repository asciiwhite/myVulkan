#pragma once

#include "handles.h"

#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>

class Device;

class Texture
{
public:
    ~Texture();

    void createDepthBuffer(Device* device, const VkExtent2D& extend, VkFormat format);
    void destroy();

    VkImageView getImageView() const { return m_imageView; }
    bool hasTranspareny() const;

    static TextureHandle getTexture(Device& device, const std::string& textureFilename);
    static void release(TextureHandle& texture);

private:
    bool loadFromFile(Device* device, const std::string& filename);

    Device* m_device = nullptr;

    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    int m_numChannels = 0;

    using TextureMap = std::unordered_map<std::string, TextureHandle>;
    static TextureMap m_loadedTextures;    
};
