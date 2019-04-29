#pragma once

#include "image.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class Device;
class ImageBase;

template<typename Into, typename From>
std::unique_ptr<Into> static_cast_unique_ptr(std::unique_ptr<From>&& ptr)
{
    return std::unique_ptr<Into>{static_cast<Into*>(ptr.release())};
}

class ImagePool
{
public:
    template<typename ImageType>
    using ImageHandle = std::unique_ptr<ImageType>;
    
    template<typename ImageType>
    static ImageHandle<ImageType> Aquire(
        const Device& device,
        VkExtent2D resolution,
        VkFormat format)
    {
        const auto imageIter = findMatchingImage(resolution, format);
        if (imageIter != m_images.end())
        {
            auto matchedImage = static_cast_unique_ptr<ImageType>(std::move(*imageIter));
            m_images.erase(imageIter);
            return matchedImage;
        }

        return ImageHandle<ImageType>(new ImageType(device, resolution, format));
    }

    template<typename ImageType>
    static void Release(ImageHandle<ImageType>&& image)
    {
        m_images.push_back(static_cast_unique_ptr<ImageType>(std::move(image)));
    }

    static void clear();

private:

    using Images = std::vector<std::unique_ptr<ImageBase>>;
    static Images m_images;

    Images::iterator static findMatchingImage(VkExtent2D resolution, VkFormat format);
};

