#include "image_utils.h"

#include <fstream>
#include <iostream>

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

bool utils::transition_image_layout(alloc::image* image, VkFormat format, VkImageAspectFlags aspect, VkImageLayout old_layout, VkImageLayout new_layout)
{
    return transition_image_layout(image, alloc::get_staging_command_pool(), alloc::get_staging_queue(), format, aspect, old_layout, new_layout);
}

bool utils::transition_image_layout(alloc::image* image, VkCommandPool command_pool, VkQueue queue, VkFormat format, VkImageAspectFlags aspect, VkImageLayout old_layout, VkImageLayout new_layout)
{
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
    else if(old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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

    vkFreeCommandBuffers(get_device(), command_pool, 1, &command_buffer);
    return true;
}