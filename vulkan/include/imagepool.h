#pragma once

#include "image.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class Device;
class ImageBase;

class ImagePool
{
private:
    class deleter;

public:
    template<typename ImageType>
    using ImageHandle = std::unique_ptr<ImageType, deleter>;
    
    template<typename ImageType>
    ImageHandle<ImageType> aquire(
        const Device& device,
        VkExtent2D resolution,
        VkFormat format)
    {
        const auto imageIter = findMatchingImage(resolution, format);
        ImageBase* imagePtr = nullptr;
        if (imageIter != m_images.end())
        {
            imagePtr = *imageIter;
            m_images.erase(imageIter);
        }
        else
        {
            imagePtr = new ImageType(device, resolution, format);
        }

        return ImageHandle<ImageType>(imagePtr, deleter(this));
    }

    template<typename ImageType>
    void release(ImageHandle<ImageType>&& image)
    {
        m_images.push_back(image.release());
    }
 
    void clear();

private:
    using Images = std::vector<ImageBase*>;
    Images m_images;

    Images::iterator findMatchingImage(VkExtent2D resolution, VkFormat format);

    class deleter
    {
    public:
        using pointer = ImageBase * ;

        deleter(ImagePool* pool = nullptr) : m_pool(pool)
        {
        }

        void operator()(ImageBase* ptr)
        {
            m_pool->m_images.emplace_back(ptr);
        }

    private:
        ImagePool* m_pool;
    };
};

