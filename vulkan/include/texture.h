#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>

class Device;

struct Texture
{
public:
    bool hasTranspareny() const { return numChannels == 4; }
    bool isValid() const { return image != VK_NULL_HANDLE; }

    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    int numChannels = 0;

    bool operator ==(const Texture& rhs) const
    {
        return image == rhs.image;
    }
};

class TextureManager
{
public:
    static Texture Acquire(Device& device, const std::string& textureFilename);
    static void Release(Device& device, Texture& texture);

private:
    static Texture LoadFromFile(Device& device, const std::string& filename);

    struct TextureData
    {
        uint64_t refCount;
        Texture texture;
    };

    using TextureMap = std::unordered_map<std::string, TextureData>;
    static TextureMap m_loadedTextures;
};
