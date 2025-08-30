#ifndef _IMAGE_UTILS_H_
#define _IMAGE_UTILS_H_

#include "../renderer/vksetup.h"

#include "alloc.h"

namespace utils
{
    bool create_image_view(VkImageView* image_view, VkImage image, VkFormat format, VkImageAspectFlags aspect);
	
	uint8_t get_format_pixel_size(VkFormat format);

    bool find_best_depth_format(VkFormat* format);
    std::vector<VkFormat> find_supported_formats(std::vector<VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    bool load_bmp_texture(const char* filepath, unsigned int* width, unsigned int* height, unsigned int* channels, char** data);

    bool transition_image_layout(alloc::image* image, VkFormat format, VkImageAspectFlags aspect, VkImageLayout old_layout, VkImageLayout new_layout);
    bool transition_image_layout(alloc::image* image, VkCommandPool command_pool, VkQueue queue, VkFormat format, VkImageAspectFlags aspect, VkImageLayout old_layout, VkImageLayout new_layout);
	
	bool read_pixel(alloc::image* image, uint32_t x, uint32_t y, void* data);
}

#endif