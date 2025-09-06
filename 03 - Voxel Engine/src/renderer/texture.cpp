#include "texture.h"

#include <iostream>

#include "vksetup.h"

#include "../utils/image_utils.h"

#define TEXTURE_IMAGE_ASPECT VK_IMAGE_ASPECT_COLOR_BIT

texture::texture(void* data, uint32_t width, uint32_t height, VkFormat image_format) : width(width), height(height), image_format(image_format)
{
    usable = alloc::new_image(&img, width, height, image_format, ALLOC_USAGE_TEXTURE);

    if(usable) usable = utils::transition_image_layout(&img, TEXTURE_IMAGE_ASPECT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	if(usable) alloc::map_to_staging(data, width * height * 4);
	if(usable) usable = alloc::copy_data_to_image(&img, data, width, height, 1, TEXTURE_IMAGE_ASPECT);
	if(usable) usable = utils::transition_image_layout(&img, TEXTURE_IMAGE_ASPECT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if(usable) usable = utils::create_image_view(&image_view, img.vk_image, image_format, VK_IMAGE_ASPECT_COLOR_BIT);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(get_selected_physical_device(), &properties);

    VkSamplerCreateInfo info_sampler{};
    info_sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info_sampler.magFilter = VK_FILTER_LINEAR;
    info_sampler.minFilter = VK_FILTER_LINEAR;
    info_sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info_sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info_sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info_sampler.anisotropyEnable = VK_TRUE;
    info_sampler.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    info_sampler.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info_sampler.unnormalizedCoordinates = VK_FALSE;
    info_sampler.compareEnable = VK_FALSE;
    info_sampler.compareOp = VK_COMPARE_OP_ALWAYS;
    info_sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info_sampler.mipLodBias = 0;
    info_sampler.minLod = 0;
    info_sampler.maxLod = 0;

    VkResult r = vkCreateSampler(get_device(), &info_sampler, nullptr, &sampler);
    VERIFY_NORETURN(r, "Failed to create Vulkan sampler (for a texture).");
    if(usable) usable = r == VK_SUCCESS;
#ifdef DEBUG_PRINT_SUCCESS
    if(usable) std::cout << "[VK|INF] Created a texture object." << std::endl;
#endif
}

texture::~texture()
{
    if(usable)
    {
        vkDestroyImageView(get_device(), image_view, nullptr);
        vkDestroySampler(get_device(), sampler, nullptr);
    }

    alloc::free(img);
}

VkImageView texture::get_image_view() const
{
    return image_view;
}

VkSampler texture::get_sampler() const
{
    return sampler;
}