#ifndef _IMAGE_UTILS_H_
#define _IMAGE_UTILS_H_

#include "../renderer/vksetup.h"

#include "alloc.h"

namespace utils
{
    bool load_bmp_texture(const char* filepath, unsigned int* width, unsigned int* height, unsigned int* channels, char** data);
    bool transition_image_layout(alloc::image* image, VkCommandPool command_pool, VkQueue queue, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
}

#endif