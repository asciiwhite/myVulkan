#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
#include <memory>

class Device;

class Texture
{
public:
    Texture() {};
    ~Texture();

    bool loadFromFile(Device* device, const std::string& filename);
    void createDepthBuffer(Device* device, const VkExtent2D& extend, VkFormat format);
    void destroy();

    VkImageView getImageView() const { return m_imageView; }
    bool hasTranspareny() const;

    static std::shared_ptr<Texture> getTexture(Device& device, const std::string& textureFilename);
    static void release(std::shared_ptr<Texture>& texture);

private:
    void createImageView();

    Device* m_device = nullptr;

    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    int m_numChannels = 0;

    using TextureMap = std::unordered_map<std::string, std::shared_ptr<Texture>>;
    static TextureMap m_loadedTextures;    
};
