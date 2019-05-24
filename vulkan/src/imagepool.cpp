#include "imagepool.h"
#include "imagebase.h"

#include <algorithm>

void ImagePool::clear()
{
    for (auto image : m_images)
        delete image;
    m_images.clear();
}

ImagePool::Images::iterator ImagePool::findMatchingImage(VkExtent2D resolution, VkFormat format)
{
    return std::find_if(m_images.begin(), m_images.end(), [=](const auto& image) {
        return  image->resolution().width == resolution.width &&
                image->resolution().height == resolution.height &&
                image->format() == format; });
}

