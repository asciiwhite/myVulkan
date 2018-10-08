#include "texture.h"
#include "vulkanhelper.h"
#include "device.h"
#include "buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::string TextureResourceHandler::CreateResourceKey(const std::string& filename)
{
    return filename;
}

Texture TextureResourceHandler::CreateResource(const Device& device, const std::string& filename)
{
    return LoadFromFile(device, filename);
}

void TextureResourceHandler::DestroyResource(const Device& device, Texture& texture)
{
    device.destroyTexture(texture);
}

Texture TextureResourceHandler::LoadFromFile(const Device& device, const std::string& filename)
{
    int texWidth, texHeight, numChannels;
    stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &numChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        printf("Error: could not load texture %s, reason: %s\n", filename.c_str(), stbi_failure_reason());
        return Texture();
    }

    Texture texture = device.createImageFromData(texWidth, texHeight, pixels, VK_FORMAT_R8G8B8A8_UNORM);
    texture.numChannels = numChannels;  

    return texture;
}
