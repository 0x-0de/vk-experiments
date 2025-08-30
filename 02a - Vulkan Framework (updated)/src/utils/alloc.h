#ifndef _VK_ALLOC_H_
#define _VK_ALLOC_H_

#include "../../lib/vulkan/vulkan.hpp"

#define ALLOC_USAGE_STAGED_VERTEX_BUFFER 0
#define ALLOC_USAGE_STAGED_INDEX_BUFFER 1
#define ALLOC_USAGE_UNIFORM_BUFFER 2
#define ALLOC_USAGE_TEXTURE 3
#define ALLOC_USAGE_DEPTH_ATTACHMENT 4
#define ALLOC_USAGE_COLOR_ATTACHMENT 5
#define ALLOC_USAGE_COLOR_ATTACHMENT_CPU_VISIBLE 6

#define ALLOC_DEFAULT_BUFFER_SHARING_MODE VK_SHARING_MODE_EXCLUSIVE

namespace alloc
{
	struct buffer
	{
		VkBuffer vk_buffer;
		
		uint16_t page_index;
		uint32_t page_offset;
		size_t allocation_size;
	};

	struct image
	{
		VkImage vk_image;
		VkFormat vk_format;
		
		uint16_t width, height;

		uint16_t page_index;
		uint32_t page_offset;
		size_t allocation_size;
	};
	
	bool copy_buffer(VkBuffer src, VkBuffer dst, VkCommandPool pool, VkQueue queue, VkDeviceSize data_size);

	bool copy_data_to_image(alloc::image* img, void* data, uint32_t width, uint32_t height, uint32_t depth, VkImageAspectFlags aspect);
	bool copy_data_to_image(alloc::image* img, void* data, uint32_t width, uint32_t height, uint32_t depth, VkImageAspectFlags aspect, VkCommandPool pool, VkQueue queue);
	
	bool create_buffer(VkBuffer* buffer, VkDeviceMemory* memory, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties);
	bool create_buffer(VkBuffer* buffer, VkDeviceMemory* memory, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties, VkSharingMode sharing_mode);
	
	void deinit();
	
	void destroy_buffer(VkBuffer* buffer, VkDeviceMemory* memory);
	
	void free(buffer buf);
	void free(image buf);
	
	VkDeviceMemory get_memory_page(uint16_t index);

	VkBuffer get_staging_buffer();
	VkCommandPool get_staging_command_pool();
	VkQueue get_staging_queue();
	
	void init(VkQueue queue, VkCommandPool pool);

	void map_data_to_buffer(void* data, alloc::buffer* buffer, size_t offset, size_t size);
	void map_data_to_buffer(void* data, alloc::buffer* buffer);

	void map_data_to_memory(void* data, VkDeviceMemory memory, size_t offset, size_t size);

	void map_to_staging(void* data, size_t size);

	bool new_buffer(buffer* buffer, void* data, VkDeviceSize size, uint32_t usage);
	bool new_buffer(buffer* buffer, VkDeviceSize size, uint32_t usage);

	bool new_image(image* image, uint16_t width, uint16_t height, VkFormat image_format, uint32_t usage);
}

#endif