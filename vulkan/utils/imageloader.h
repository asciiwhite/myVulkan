#pragma once

#include "image.h"

#include <string>

class ImageLoader
{
public:
    static Texture load(const Device& device, const std::string& filename);
};
