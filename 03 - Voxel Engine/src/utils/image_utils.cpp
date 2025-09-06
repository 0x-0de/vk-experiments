#include "image_utils.h"

#include <fstream>
#include <iostream>

bool utils::create_image_view(VkImageView* image_view, alloc::image img, VkImageAspectFlags aspect)
{
	return create_image_view(image_view, img.vk_image, img.vk_format, aspect);
}

bool utils::create_image_view(VkImageView* image_view, VkImage image, VkFormat format, VkImageAspectFlags aspect)
{
    VkImageViewCreateInfo info_image_view{};

    info_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info_image_view.image = image;
    info_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info_image_view.format = format;
    info_image_view.subresourceRange.aspectMask = aspect;
    info_image_view.subresourceRange.baseMipLevel = 0;
    info_image_view.subresourceRange.levelCount = 1;
    info_image_view.subresourceRange.baseArrayLayer = 0;
    info_image_view.subresourceRange.layerCount = 1;

    VkResult r = vkCreateImageView(get_device(), &info_image_view, nullptr, image_view);
    VERIFY(r, "Failed to create Vulkan image view.");

    return true;
}

bool utils::find_best_depth_format(VkFormat* format)
{
    std::vector<VkFormat> formats = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    formats = find_supported_formats(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    if(formats.size() == 0)
    {
        std::cerr << "[UTILS|ERR] Could not find a depth image format supported by your hardware." << std::endl;
        return false;
    }

    *format = formats[0];
    return true;
}

uint8_t utils::get_format_pixel_size(VkFormat format)
{
	switch(format)
	{
		case VK_FORMAT_UNDEFINED:
		default:
			std::cerr << "[UTILS|ERR] Cannot get image format pixel size for format " << format << "." << std::endl;
			return 0;
		case VK_FORMAT_R4G4_UNORM_PACK8:
		case VK_FORMAT_A8_UNORM:
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_SRGB: 
			return 8;
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
		case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		case VK_FORMAT_B5G6R5_UNORM_PACK16:
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		case VK_FORMAT_A1B5G5R5_UNORM_PACK16:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8_USCALED:
		case VK_FORMAT_R8G8_SSCALED:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_SRGB: 
			return 16;
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_R8G8B8_USCALED:
		case VK_FORMAT_R8G8B8_SSCALED:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_B8G8R8_SRGB:
			return 24;
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
			return 32;
		case VK_FORMAT_R32G32B32_SFLOAT:
			return 96;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 128;
	}
}

std::vector<VkFormat> utils::find_supported_formats(std::vector<VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    std::vector<VkFormat> passed;

    for(size_t i = 0; i < candidates.size(); i++)
    {
        VkFormat target = candidates[i];
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(get_selected_physical_device(), target, &properties);

        if(tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
            passed.push_back(target);
        else if(tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
            passed.push_back(target);
    }

    return passed;
}

bool utils::load_bmp_texture(const char* filepath, unsigned int* width, unsigned int* height, unsigned int* channels, char** data)
{
    //BMP File structure:

    //Bytes 0-1: "BM" signature
    //Bytes 2-5: Size of BMP file, in bytes (header + 16 bytes data)
    //Bytes 6-9: Unused
    //Bytes 10-13: Offset of the pixel array
    //Bytes 14-17: Number of bytes in the DIB header (from this point)
    //Bytes 18-21: Image width
    //Bytes 22-25: Image height
    //Bytes 26-27: Number of color planes used
    //Bytes 28-29: Number of bits per pixel
    //Bytes 30-33: Type of compression used (usually 0, being BI_RGB, or 3, being BI_BITFIELDS).
    //Bytes 34-37: Size of the raw bitmap data, in bytes
    //Bytes 38-41: Print width of the bitmap (pixels per meter)
    //Bytes 42-45: Print height of the bitmap (pixels per meter)
    //Bytes 46-49: Number of colors in the pallete
    //Bytes 50-53: Number of 'important' colors (0 means they're all important).
    //Bytes 54-57 (if compression is BI_BITFIELDS): i'll finish this later.

    char bytes[4];

    std::ifstream reader(filepath, std::ios::binary);

    if(reader.good())
    {
        uint32_t offset;

        reader.seekg(10, reader.beg);
        reader.read(bytes, 4);
        memcpy(&offset, bytes, sizeof(uint32_t));

        reader.seekg(18, reader.beg);
        reader.read(bytes, 4);
        memcpy(width, bytes, sizeof(uint32_t));
        reader.read(bytes, 4);
        memcpy(height, bytes, sizeof(uint32_t));

        uint32_t data_size;
        *channels = 0;

        reader.seekg(28, reader.beg);
        reader.read(bytes, 2);
        memcpy(channels, bytes, sizeof(uint16_t));
        reader.seekg(34, reader.beg);
        reader.read(bytes, 4);
        memcpy(&data_size, bytes, sizeof(uint32_t));

        *channels /= 8;
        
        reader.seekg(offset);

        *data = new char[data_size];
        reader.read(*data, data_size);

    #ifdef DEBUG_PRINT_SUCCESS
        std::cout << "[UTILS|INF] Loaded .bmp imge file: " << filepath << std::endl;
        std::cout << "\tSize: " << *width << "x" << *height << " px" << std::endl;
        std::cout << "\tChannels: " << *channels << std::endl;
        std::cout << "\tTotal size: " << data_size << " bytes" << std::endl;
    #endif
        return true;
    }
    else
    {
        std::cerr << "Cannot open texture file: " << filepath << std::endl;
    }

    return false;
}

bool utils::read_pixel(alloc::image* image, uint32_t x, uint32_t y, void* data)
{
	uint8_t pixel_size = get_format_pixel_size(image->vk_format) >> 3;
	VkDeviceMemory mem = alloc::get_memory_page(image->page_index);

	size_t pixel_offset = (x * image->height + y) * pixel_size;
	if(pixel_offset > image->allocation_size)
	{
		std::cerr << "[UTILS|ERR] Requested image pixel (" << x << ", " << y << ") exceeds image boundaries." << std::endl;
		return false;
	}
	
	std::cout << image->page_offset << std::endl;
	
	void* map;
	vkMapMemory(get_device(), mem, pixel_offset + image->page_offset, pixel_size, 0, &map);
		memcpy(data, map, pixel_size);
	vkUnmapMemory(get_device(), mem);
	
	return true;
}

bool utils::transition_image_layout(alloc::image* image, VkImageAspectFlags aspect, VkImageLayout old_layout, VkImageLayout new_layout)
{
    return transition_image_layout(image, alloc::get_staging_command_pool(), alloc::get_staging_queue(), aspect, old_layout, new_layout);
}

bool utils::transition_image_layout(alloc::image* image, VkCommandPool command_pool, VkQueue queue, VkImageAspectFlags aspect, VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkFormat format = image->vk_format;
    VkCommandBuffer command_buffer;
    
    VkCommandBufferAllocateInfo info_alloc{};
    info_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info_alloc.commandPool = command_pool;
    info_alloc.commandBufferCount = 1;

    vkAllocateCommandBuffers(get_device(), &info_alloc, &command_buffer);

    VkCommandBufferBeginInfo info_begin{};
    info_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &info_begin);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image->vk_image;
    barrier.subresourceRange.aspectMask = aspect;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage, dst_stage;

    if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
	else if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		
		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		
		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
    else if(old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
	else if(old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		
		src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
    else
    {
        std::cerr << "[VK|ERR] Unsupported layout transition formats." << std::endl;
        return false;
    }

    vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo info_submit{};
    info_submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info_submit.commandBufferCount = 1;
    info_submit.pCommandBuffers = &command_buffer;

    vkQueueSubmit(queue, 1, &info_submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
	
	image->vk_image_layout = new_layout;

    vkFreeCommandBuffers(get_device(), command_pool, 1, &command_buffer);
    return true;
}