#include "imageloader.h"

#include "stb_image.h"

Texture ImageLoader::load(const Device& device, const std::string& filename)
{
    int texWidth, texHeight, numChannels;
    stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &numChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        printf("Error: could not load texture %s, reason: %s\n", filename.c_str(), stbi_failure_reason());
        return Texture();
    }

    Texture texture(device, pixels, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);
//    texture.numChannels = numChannels;

    return texture;
}