#include "alloc.h"

#define MB (1<<20)
#define DEFAULT_PAGE_SIZE 128*MB
#define STAGING_MEMORY_SIZE 128*MB

#define PAGE_MEMORY_TYPE_DEVICE_LOCAL VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
#define PAGE_MEMORY_TYPE_HOST_AVAILABLE VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT

#define USAGE_STAGED_VERTEX_BUFFER (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
#define USAGE_STAGED_INDEX_BUFFER (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
#define USAGE_STAGED_SAMPLED_IMAGE (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
#define USAGE_COLOR_ATTACHMENT VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
#define USAGE_DEPTH_ATTACHMENT VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT

#define USAGE_UNIFORM_BUFFER VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT

#include <iostream>

#include "../renderer/vksetup.h"

#include "image_utils.h"

static uint32_t requested_allocation_type;

struct freelist_node
{
	size_t offset, size;
};

struct mem_page
{
	VkDeviceMemory memory;
	std::vector<freelist_node> freelist;
	
	uint32_t memory_type_index;
};

static std::vector<mem_page> mem_pages;

std::string requested_allocation_to_string(uint32_t usage)
{
	switch(usage)
	{
		case ALLOC_USAGE_STAGED_VERTEX_BUFFER:
			return "ALLOC_USAGE_STAGED_VERTEX_BUFFER";
		case ALLOC_USAGE_STAGED_INDEX_BUFFER:
			return "ALLOC_USAGE_STAGED_INDEX_BUFFER";
		case ALLOC_USAGE_UNIFORM_BUFFER:
			return "ALLOC_USAGE_UNIFORM_BUFFER";
		case ALLOC_USAGE_TEXTURE:
			return "ALLOC_USAGE_TEXTURE";
		case ALLOC_USAGE_DEPTH_ATTACHMENT:
			return "ALLOC_USAGE_DEPTH_ATTACHMENT";
		case ALLOC_USAGE_COLOR_ATTACHMENT:
			return "ALLOC_USAGE_COLOR_ATTACHMENT";
		case ALLOC_USAGE_COLOR_ATTACHMENT_CPU_VISIBLE:
			return "ALLOC_USAGE_COLOR_ATTACHMENT_CPU_VISIBLE";
		default:
			return "UNKNOWN OR INVALID ALLOC USAGE";
	}
}

std::string memory_type_filter_to_string(uint32_t type_filter)
{
	std::string str = "";

	for(size_t i = 0; i < 32; i++)
	{
		if((type_filter >> i) & 1)
			str += std::to_string(i) + " ";
	}

	return str;
}

std::string memory_heap_flags_to_string(VkMemoryHeapFlags flags)
{
	std::string str = "";

	if(flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) str += "| MEMORY_HEAP_DEVICE_LOCAL ";
	if((flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) || (flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR)) str += "| MEMORY_HEAP_MULTI_INSTANCE ";
	if(flags & VK_MEMORY_HEAP_TILE_MEMORY_BIT_QCOM) str += "| MEMORY_HEAP_TILE_MEMORY_QCOM ";

	return str;
}

std::string memory_type_flags_to_string(VkMemoryPropertyFlags flags)
{
	std::string str = "";

	if(flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) str += "| MEMORY_PROPERTY_DEVICE_LOCAL ";
	if(flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) str += "| MEMORY_PROPERTY_HOST_VISIBLE ";
	if(flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) str += "| MEMORY_PROPERTY_HOST_COHERENT ";
	if(flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) str += "| MEMORY_PROPERTY_HOST_CACHED ";
	if(flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) str += "| MEMORY_PROPERTY_LAZILY_ALLOCATED ";
	if(flags & VK_MEMORY_PROPERTY_PROTECTED_BIT) str += "| MEMORY_PROPERTY_PROTECTED ";
	if(flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) str += "| MEMORY_PROPERTY_DEVICE_COHERENT_AMD ";
	if(flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) str += "| MEMORY_PROPERTY_DEVICE_UNCACHED_AMD ";
	if(flags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV) str += "| MEMORY_PROPERTY_RDMA_CAPABLE_NV ";

	return str;
}

void print_physical_device_memory_properties(VkPhysicalDeviceMemoryProperties properties)
{
	std::cout << "[ALLOC|INF] Memory properties for physical device:" << std::endl;
	std::cout << "\tNumber of memory types: " << properties.memoryTypeCount << std::endl;
	std::cout << "\tNumber of memory heaps: " << properties.memoryHeapCount << std::endl;
	std::cout << "\n\tMemory heaps: " << std::endl;
	for(size_t i = 0; i < properties.memoryHeapCount; i++)
	{
		std::cout << "\t\tHeap " << i << ": size is " << properties.memoryHeaps[i].size << " bytes." << std::endl;
		std::cout << "\t\tHeap " << i << " flags: " << memory_heap_flags_to_string(properties.memoryHeaps[i].flags) << std::endl;
	}
	std::cout << "\n\tMemory types: " << std::endl;
	for(size_t i = 0; i < properties.memoryTypeCount; i++)
	{
		std::cout << "\t\tType " << i << ": heap index is " << properties.memoryTypes[i].heapIndex << "." << std::endl;
		std::cout << "\t\tType " << i << " flags: " << memory_type_flags_to_string(properties.memoryTypes[i].propertyFlags) << std::endl;
	}
}

//Returns the index of a suitable memory type.
//Every memory type also contains a memory heap, so we don't need to look for one of those seperately.
static uint32_t find_suitable_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(get_selected_physical_device(), &memory_properties);
	
	for(uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		if((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	
	std::cerr << "[ALLOC|ERR] Couldn't find suitable memory type for requested allocation." << std::endl;
	std::cerr << "\tRequested allocation type: " << requested_allocation_to_string(requested_allocation_type) << "." << std::endl;
	std::cerr << "\tAllowed memory types: " << memory_type_filter_to_string(type_filter) << "." << std::endl;
	std::cerr << "\tRequested memory properties: " << memory_type_flags_to_string(properties) << "." << std::endl;
	std::cerr << "Note: available memory types and heaps are as follows:" << std::endl;
	print_physical_device_memory_properties(memory_properties);

	return UINT32_MAX;
}

void fill_memory(VkDeviceSize size, uint32_t offset, uint32_t page, uint32_t freelist_index)
{
	if(offset == mem_pages[page].freelist[freelist_index].offset)
	{
		mem_pages[page].freelist[freelist_index].offset += size;
		mem_pages[page].freelist[freelist_index].size -= size;
		
		if(mem_pages[page].freelist[freelist_index].size == 0)
		{
			mem_pages[page].freelist.erase(mem_pages[page].freelist.begin() + freelist_index);
		}
	}
	else
	{
		uint32_t end = mem_pages[page].freelist[freelist_index].offset + mem_pages[page].freelist[freelist_index].size;
		mem_pages[page].freelist[freelist_index].size = offset - mem_pages[page].freelist[freelist_index].offset;

		if(offset + size < end)
		{
			freelist_node fn;

			fn.offset = offset + size;
			fn.size = end - fn.offset;

			mem_pages[page].freelist.insert(mem_pages[page].freelist.begin() + freelist_index + 1, fn);
		}
	}
}

void debug_print_page_freelist(size_t page_index)
{
	std::cout << "[ALLOC|INF] Free list for page " << page_index << ":" << std::endl;
	for(size_t i = 0; i < mem_pages[page_index].freelist.size(); i++)
	{
		freelist_node f = mem_pages[page_index].freelist[i];
		std::cout << "  [" << f.offset << ", " << f.size << "]" << std::endl;
	}
	std::cout << "[ALLOC|INF] End freelist." << std::endl;
}

bool create_new_memory_page(uint32_t memory_type_index, size_t size)
{
	mem_page page;
	
	VkMemoryAllocateInfo info_alloc{};
	info_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info_alloc.allocationSize = size;
	info_alloc.memoryTypeIndex = memory_type_index;
		
	VkResult r = vkAllocateMemory(get_device(), &info_alloc, nullptr, &(page.memory));
	VERIFY(r, "Failed to allocate Vulkan memory (when creating new memory page).");
	
	page.freelist.resize(1);
	
	page.freelist[0].offset = 0;
	page.freelist[0].size = size;
	
	page.memory_type_index = memory_type_index;
	
	mem_pages.push_back(page);
	
	debug_print_page_freelist(mem_pages.size() - 1);
	return true;
}

bool create_new_memory_page(uint32_t memory_type_index)
{
	return create_new_memory_page(memory_type_index, DEFAULT_PAGE_SIZE);
}

bool find_page_space(VkMemoryRequirements memory_requirements, uint32_t memory_properties, size_t* page_index, size_t* page_memory_offset, size_t* freelist_index)
{
	uint32_t memory_type_index = find_suitable_memory_type(memory_requirements.memoryTypeBits, memory_properties);
	if(memory_type_index == UINT32_MAX) return false;

	if(memory_requirements.size >= DEFAULT_PAGE_SIZE)
	{
		size_t pi = mem_pages.size();
		if(!create_new_memory_page(memory_type_index, memory_requirements.size)) return false;
		*page_index = pi;
		*page_memory_offset = 0;
		*freelist_index = 0;
		return true;
	}
	
	for(size_t i = 0; i < mem_pages.size(); i++)
	{
		std::cout << "Scanning page " << i << "..." << std::endl;
		if(mem_pages[i].memory_type_index != memory_type_index)
		{
			std::cout << "Skipped." << std::endl;
			continue;
		}
		for(size_t j = 0; j < mem_pages[i].freelist.size(); j++)
		{
			if(mem_pages[i].freelist[j].size >= memory_requirements.size)
			{
				if(mem_pages[i].freelist[j].offset % memory_requirements.alignment == 0) //Freelist node offset happens to match alignment.
				{
					*page_index = i;
					*page_memory_offset = mem_pages[i].freelist[j].offset;
					*freelist_index = j;
					return true;
				}
				else
				{
					uint32_t next_alignment = (mem_pages[i].freelist[j].offset / memory_requirements.alignment) + 1;
					next_alignment *= memory_requirements.alignment;
					uint32_t diff = next_alignment - mem_pages[i].freelist[j].offset;

					if(mem_pages[i].freelist[j].size - diff >= memory_requirements.size)
					{
						*page_index = i;
						*page_memory_offset = next_alignment;
						*freelist_index = j;
						return true;
					}
				}
			}
		}
	}
	
	size_t pi = mem_pages.size();
	if(!create_new_memory_page(memory_type_index)) return false;
	*page_index = pi;
	*page_memory_offset = 0;
	*freelist_index = 0;
	
	return true;
}

bool allocate_buffer(alloc::buffer* buf, void* data, uint32_t memory_type, VkDeviceSize size, VkBufferUsageFlags usage)
{
	alloc::buffer b;
	
	VkBufferCreateInfo info_buffer{};
	info_buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info_buffer.size = size;
	info_buffer.usage = usage;
	info_buffer.sharingMode = ALLOC_DEFAULT_BUFFER_SHARING_MODE;
	
	VkResult r = vkCreateBuffer(get_device(), &info_buffer, nullptr, &(b.vk_buffer));
	VERIFY(r, "Failed to create Vulkan buffer (during allocation).");
	
	VkMemoryRequirements mem_req;
	vkGetBufferMemoryRequirements(get_device(), b.vk_buffer, &mem_req);
	
	std::cout << "Attempting an allocation of " << size << " (" << mem_req.size << ") bytes for a buffer." << std::endl;
	
	size_t page, memory_offset, freelist_index;
	if(!find_page_space(mem_req, memory_type, &page, &memory_offset, &freelist_index)) return false;
	
	std::cout << "Space found: page " << page << ", offset: " << memory_offset << " bytes." << std::endl;
	
	b.allocation_size = mem_req.size;
	b.page_index = page;
	b.page_offset = memory_offset;
	vkBindBufferMemory(get_device(), b.vk_buffer, mem_pages[page].memory, memory_offset);
	
	fill_memory(mem_req.size, memory_offset, page, freelist_index);
	
	debug_print_page_freelist(page);
	
	*buf = b;
	return true;
}

bool allocate_image(alloc::image* image, uint16_t width, uint16_t height, VkFormat image_format, uint32_t memory_type, VkImageUsageFlags usage)
{
	alloc::image img;

	VkImageCreateInfo info_image{};
    info_image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info_image.imageType = VK_IMAGE_TYPE_2D;
    info_image.extent.width = width;
    info_image.extent.height = height;
    info_image.extent.depth = 1;
    info_image.mipLevels = 1;
    info_image.arrayLayers = 1;
    info_image.format = image_format;
    info_image.tiling = VK_IMAGE_TILING_OPTIMAL; //Optimal is for efficient access from the shader, while linear can be used if you want to read/access image data from the CPU.

    //Either way, the initial layout will be unusable by the GPU. Undefined tells Vulkan that we don't care about the memory here, so the first layout transition will discard whatever data is sitting at this location in memory.
	info_image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    info_image.usage = usage;
    info_image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info_image.samples = VK_SAMPLE_COUNT_1_BIT; //Only relevant for multisampling - in all other cases, we just use one sample.
    info_image.flags = 0;

	VkResult r = vkCreateImage(get_device(), &info_image, nullptr, &(img.vk_image));
    VERIFY(r, "Failed to create a Vulkan image.");

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(get_device(), img.vk_image, &mem_req);

	std::cout << "Attempting an allocation of " << mem_req.size << " bytes for an image." << std::endl;

	size_t page, memory_offset, freelist_index;
	if(!find_page_space(mem_req, memory_type, &page, &memory_offset, &freelist_index)) return false;

	std::cout << "Space found: page " << page << ", offset: " << memory_offset << " bytes." << std::endl;

	img.allocation_size = mem_req.size;
	img.page_index = page;
	img.page_offset = memory_offset;
	img.vk_format = image_format;
	img.width = width;
	img.height = height;
	vkBindImageMemory(get_device(), img.vk_image, mem_pages[page].memory, memory_offset);

	fill_memory(mem_req.size, memory_offset, page, freelist_index);

	debug_print_page_freelist(page);

	*image = img;
	return true;
}

bool allocate_host_buffer(alloc::buffer* buf, void* data, VkDeviceSize size, VkBufferUsageFlags usage)
{
	if(!allocate_buffer(buf, data, PAGE_MEMORY_TYPE_HOST_AVAILABLE, size, usage)) return false;

	uint16_t page = buf->page_index;
	uint32_t offset = buf->page_offset;
	
	alloc::map_data_to_memory(data, mem_pages[page].memory, offset, size);
	
	return true;
}

static VkBuffer alloc_stage_buffer;
static VkDeviceMemory alloc_stage_memory;

static VkQueue staging_queue;
static VkCommandPool staging_command_pool;

bool stage_buffer(alloc::buffer* buf, void* data, VkDeviceSize size, VkBufferUsageFlags usage)
{
	if(size >= STAGING_MEMORY_SIZE)
	{
		std::cerr << "[ALLOC|ERR] Requested staging memory too large.\n\tMax size is " << STAGING_MEMORY_SIZE << " bytes.\n\tRequested size is " << size << " bytes." << std::endl;
		return false;
	}
	
	if(!allocate_buffer(buf, data, PAGE_MEMORY_TYPE_DEVICE_LOCAL, size, usage)) return false;
	
	alloc::map_data_to_memory(data, alloc_stage_memory, 0, size);
	alloc::copy_buffer(alloc_stage_buffer, buf->vk_buffer, staging_command_pool, staging_queue, size);
	
	//debug_print_page_freelist(page);
	
	return true;
}

bool alloc::copy_buffer(VkBuffer src, VkBuffer dst, VkCommandPool pool, VkQueue queue, VkDeviceSize data_size)
{
	VkCommandBufferAllocateInfo info_alloc{};
	info_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info_alloc.commandPool = pool;
	info_alloc.commandBufferCount = 1;
	
	VkCommandBuffer cmd_buffer;
	vkAllocateCommandBuffers(get_device(), &info_alloc, &cmd_buffer);
	
	VkCommandBufferBeginInfo info_begin{};
	info_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	vkBeginCommandBuffer(cmd_buffer, &info_begin);
	
	VkBufferCopy copy_region{};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = data_size;
	
	vkCmdCopyBuffer(cmd_buffer, src, dst, 1, &copy_region);
	
	vkEndCommandBuffer(cmd_buffer);
	
	VkSubmitInfo info_submit{};
	info_submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info_submit.commandBufferCount = 1;
	info_submit.pCommandBuffers = &cmd_buffer;

	vkQueueSubmit(queue, 1, &info_submit, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);
	
	vkFreeCommandBuffers(get_device(), pool, 1, &cmd_buffer);
	
	return true;
}

bool alloc::copy_data_to_image(alloc::image* img, void* data, uint32_t width, uint32_t height, uint32_t depth, VkImageAspectFlags aspect)
{
	return copy_data_to_image(img, data, width, height, depth, aspect, staging_command_pool, staging_queue);
}

bool alloc::copy_data_to_image(alloc::image* img, void* data, uint32_t width, uint32_t height, uint32_t depth, VkImageAspectFlags aspect, VkCommandPool pool, VkQueue queue)
{
	VkCommandBufferAllocateInfo info_alloc{};
	info_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info_alloc.commandPool = pool;
	info_alloc.commandBufferCount = 1;
	
	VkCommandBuffer cmd_buffer;
	vkAllocateCommandBuffers(get_device(), &info_alloc, &cmd_buffer);
	
	VkCommandBufferBeginInfo info_begin{};
	info_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	vkBeginCommandBuffer(cmd_buffer, &info_begin);

	VkBufferImageCopy copy_region{};
	copy_region.bufferOffset = 0;
	
	//Indicating that the pixels are tightly packed.
	copy_region.bufferRowLength = 0;
	copy_region.bufferImageHeight = 0;

	copy_region.imageSubresource.aspectMask = aspect;
	copy_region.imageSubresource.mipLevel = 0;
	copy_region.imageSubresource.baseArrayLayer = 0;
	copy_region.imageSubresource.layerCount = 1;

	copy_region.imageOffset = {0, 0, 0};
	copy_region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(cmd_buffer, alloc_stage_buffer, img->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

	vkEndCommandBuffer(cmd_buffer);
	
	VkSubmitInfo info_submit{};
	info_submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info_submit.commandBufferCount = 1;
	info_submit.pCommandBuffers = &cmd_buffer;

	vkQueueSubmit(queue, 1, &info_submit, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);
	
	vkFreeCommandBuffers(get_device(), pool, 1, &cmd_buffer);

	return true;
}

bool alloc::create_buffer(VkBuffer* buffer, VkDeviceMemory* memory, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties)
{
	return create_buffer(buffer, memory, size, usage, memory_properties, VK_SHARING_MODE_EXCLUSIVE);
}

bool alloc::create_buffer(VkBuffer* buffer, VkDeviceMemory* memory, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties, VkSharingMode sharing_mode)
{	
	VkBufferCreateInfo info_buffer{};
	info_buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info_buffer.size = size;
	info_buffer.usage = usage;
	info_buffer.sharingMode = sharing_mode;
	
	std::cout << "Buffer usage: " << usage << std::endl;
	
	VkResult r = vkCreateBuffer(get_device(), &info_buffer, nullptr, buffer);
	VERIFY(r, "Failed to create Vulkan buffer.");

	VkMemoryRequirements mem_req;
	vkGetBufferMemoryRequirements(get_device(), *buffer, &mem_req);
	
	std::cout << "Buffer required size: " << mem_req.size << " bytes." << std::endl;
	
	VkMemoryAllocateInfo info_alloc{};
	info_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info_alloc.allocationSize = mem_req.size;
	info_alloc.memoryTypeIndex = find_suitable_memory_type(mem_req.memoryTypeBits, memory_properties);
		
	r = vkAllocateMemory(get_device(), &info_alloc, nullptr, memory);
	VERIFY(r, "Failed to allocate Vulkan memory.");
	
	vkBindBufferMemory(get_device(), *buffer, *memory, 0);
	
	return true;
}

void alloc::deinit()
{
	while(mem_pages.size() > 0)
	{
		vkFreeMemory(get_device(), mem_pages[0].memory, nullptr);
		mem_pages.erase(mem_pages.begin());
	}
	
	destroy_buffer(&alloc_stage_buffer, &alloc_stage_memory);
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[ALLOC|INF] Deinitialized memory allocator." << std::endl;
#endif
}

void alloc::destroy_buffer(VkBuffer* buffer, VkDeviceMemory* memory)
{
	vkDestroyBuffer(get_device(), *buffer, nullptr);
	vkFreeMemory(get_device(), *memory, nullptr);
}

void alloc::free(buffer buf)
{
	std::cout << "[ALLOC|INF] Freeing allocated buffer of " << buf.allocation_size << " bytes." << std::endl;
	
	vkDestroyBuffer(get_device(), buf.vk_buffer, nullptr);
	
	uint16_t page = buf.page_index;
	uint32_t offset = buf.page_offset;
	
	std::vector<freelist_node>& fl = mem_pages[page].freelist;
	
	size_t free_insert_location = 0;
	for(size_t i = 0; i < fl.size(); i++)
	{
		if(fl[i].offset > offset)
			break;
		free_insert_location++;
	}
	
	freelist_node node;
	
	node.offset = offset;
	node.size = buf.allocation_size;
	
	fl.insert(fl.begin() + free_insert_location, node);
	
	for(size_t i = 1; i < fl.size(); i++)
	{
		size_t previous_end = fl[i - 1].offset + fl[i - 1].size;
		if(previous_end == fl[i].offset)
		{
			fl[i - 1].size += fl[i].size;
			fl.erase(fl.begin() + i);
			i--;
		}
	}
	
	debug_print_page_freelist(page);
}

void alloc::free(image img)
{
	std::cout << "[ALLOC|INF] Freeing allocated image of " << img.allocation_size << " bytes." << std::endl;
	
	vkDestroyImage(get_device(), img.vk_image, nullptr);
	
	uint16_t page = img.page_index;
	uint32_t offset = img.page_offset;
	
	std::vector<freelist_node>& fl = mem_pages[page].freelist;
	
	size_t free_insert_location = 0;
	for(size_t i = 0; i < fl.size(); i++)
	{
		if(fl[i].offset > offset)
			break;
		free_insert_location++;
	}
	
	freelist_node node;
	
	node.offset = offset;
	node.size = img.allocation_size;
	
	fl.insert(fl.begin() + free_insert_location, node);
	
	for(size_t i = 1; i < fl.size(); i++)
	{
		size_t previous_end = fl[i - 1].offset + fl[i - 1].size;
		if(previous_end == fl[i].offset)
		{
			fl[i - 1].size += fl[i].size;
			fl.erase(fl.begin() + i);
			i--;
		}
	}
	
	debug_print_page_freelist(page);
}

VkDeviceMemory alloc::get_memory_page(uint16_t index)
{
	return mem_pages[index].memory;
}

VkBuffer alloc::get_staging_buffer()
{
	return alloc_stage_buffer;
}

VkCommandPool alloc::get_staging_command_pool()
{
	return staging_command_pool;
}

VkQueue alloc::get_staging_queue()
{
	return staging_queue;
}

void alloc::init(VkQueue queue, VkCommandPool pool)
{
	staging_queue = queue;
	staging_command_pool = pool;
	
	create_buffer(&alloc_stage_buffer, &alloc_stage_memory, STAGING_MEMORY_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[ALLOC|INF] Memory allocator initialized." << std::endl;
#endif
}

void alloc::map_data_to_buffer(void* data, alloc::buffer* buffer, size_t offset, size_t size)
{
	size_t os = buffer->page_offset + offset;

	VkDeviceMemory mem = mem_pages[buffer->page_index].memory;
	map_data_to_memory(data, mem, os, size);
}

void alloc::map_data_to_buffer(void* data, alloc::buffer* buffer)
{
	size_t offset = buffer->page_offset;
	size_t size = buffer->allocation_size;

	map_data_to_buffer(data, buffer, offset, size);
}

void alloc::map_data_to_memory(void* data, VkDeviceMemory memory, size_t offset, size_t size)
{
	void* map;
	vkMapMemory(get_device(), memory, offset, size, 0, &map);
		memcpy(map, data, size);
	vkUnmapMemory(get_device(), memory);
}

void alloc::map_to_staging(void* data, size_t size)
{
	map_data_to_memory(data, alloc_stage_memory, 0, size);
}

bool alloc::new_buffer(buffer* buf, void* data, VkDeviceSize size, uint32_t usage)
{
	switch(usage)
	{
		case ALLOC_USAGE_STAGED_VERTEX_BUFFER:
			return stage_buffer(buf, data, size, USAGE_STAGED_VERTEX_BUFFER);
		case ALLOC_USAGE_STAGED_INDEX_BUFFER:
			return stage_buffer(buf, data, size, USAGE_STAGED_INDEX_BUFFER);
		case ALLOC_USAGE_UNIFORM_BUFFER:
			return allocate_host_buffer(buf, data, size, USAGE_UNIFORM_BUFFER);
		//case ALLOC_USAGE_IMAGE:
			//return false;
		default:
			std::cerr << "[ALLOC|ERR] Buffer usage not supported: " << usage << std::endl;
			break;
	}
	
	return false;
}

bool alloc::new_buffer(buffer* buffer, VkDeviceSize size, uint32_t usage)
{
	requested_allocation_type = usage;
	char* data = new char[size];

	for(VkDeviceSize i = 0; i < size; i++)
	{
		data[i] = 0;
	}

	bool ret = new_buffer(buffer, data, size, usage);
	delete[] data;
	return ret;
}

bool alloc::new_image(image* image, uint16_t width, uint16_t height, VkFormat image_format, uint32_t usage)
{
	requested_allocation_type = usage;

	switch(usage)
	{
		case ALLOC_USAGE_TEXTURE:
			return allocate_image(image, width, height, image_format, PAGE_MEMORY_TYPE_DEVICE_LOCAL, USAGE_STAGED_SAMPLED_IMAGE);
		case ALLOC_USAGE_DEPTH_ATTACHMENT:
			return allocate_image(image, width, height, image_format, PAGE_MEMORY_TYPE_DEVICE_LOCAL, USAGE_DEPTH_ATTACHMENT);
		case ALLOC_USAGE_COLOR_ATTACHMENT:
			return allocate_image(image, width, height, image_format, PAGE_MEMORY_TYPE_DEVICE_LOCAL, USAGE_COLOR_ATTACHMENT);
		case ALLOC_USAGE_COLOR_ATTACHMENT_CPU_VISIBLE:
			return allocate_image(image, width, height, image_format, PAGE_MEMORY_TYPE_HOST_AVAILABLE, USAGE_COLOR_ATTACHMENT);
		default:
			std::cerr << "[ALLOC|ERR] Image usage not supported: " << usage << std::endl;
			break;
	}

	return false;
}