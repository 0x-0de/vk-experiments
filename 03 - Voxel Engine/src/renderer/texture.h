#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "../utils/alloc.h"

class texture
{
    public:
        texture(void* data, uint32_t width, uint32_t height, VkFormat image_format);
        ~texture();

        VkImageView get_image_view() const;
        VkSampler get_sampler() const;
    private:
        uint32_t width, height;
        VkFormat image_format;

        VkImageView image_view;
        VkSampler sampler;

        alloc::image img;
        bool usable;
};

#endif