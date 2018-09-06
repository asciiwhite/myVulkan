#pragma once

#include "resourcemanager.h"

#include <vulkan/vulkan.h>
#include <string>

class Device;

struct Texture
{
public:
    bool hasTranspareny() const { return numChannels == 4; }

    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    int numChannels = 0;

    bool operator == (const Texture& rhs) const
    {
        return image == rhs.image;
    }

    operator bool() const
    {
        return image != VK_NULL_HANDLE;
    }
};

class TextureResourceHandler
{
public:
    using ResourceKey = std::string;
    using ResourceType = Texture;

    static ResourceKey CreateResourceKey(const std::string& filename);
    static ResourceType CreateResource(Device& device, const std::string& filename);
    static void DestroyResource(Device& device, ResourceType& resource);

private:
    static Texture LoadFromFile(Device& device, const std::string& filename);
};

using TextureManager = ResourceManager<TextureResourceHandler>;