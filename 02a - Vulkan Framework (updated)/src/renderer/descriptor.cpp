#include "descriptor.h"

#include <iostream>

bool descriptor::create_descriptor_pool(VkDescriptorPool* pool, std::vector<VkDescriptorType> types, uint32_t descriptor_count)
{
	VkDescriptorPoolSize* pool_sizes = new VkDescriptorPoolSize[types.size()];
	for(uint16_t i = 0; i < types.size(); i++)
	{
		pool_sizes[i] = {};
		pool_sizes[i].type = types[i];
		pool_sizes[i].descriptorCount = descriptor_count;
	}

	VkDescriptorPoolCreateInfo info_pool{};
	info_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info_pool.poolSizeCount = types.size();
	info_pool.pPoolSizes = pool_sizes;
	info_pool.maxSets = descriptor_count;
	
	VkDescriptorPool descriptor_pool;
	VkResult r = vkCreateDescriptorPool(get_device(), &info_pool, nullptr, &descriptor_pool);
	VERIFY(r, "Failed to create Vulkan descriptor pool.");
	
#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Created Vulkan descriptor pool." << std::endl;
#endif
	*pool = descriptor_pool;
	return true;
}

VkDescriptorSetLayoutBinding descriptor::create_descriptor_set_binding(uint32_t binding_index, VkDescriptorType type, VkShaderStageFlags shader_stage)
{
	VkDescriptorSetLayoutBinding info_descriptor_set_binding{};
	info_descriptor_set_binding.binding = binding_index;
	info_descriptor_set_binding.descriptorType = type;
    info_descriptor_set_binding.descriptorCount = 1;
	info_descriptor_set_binding.stageFlags = shader_stage;
	
	return info_descriptor_set_binding;
}

bool descriptor::create_descriptor_set_layout(VkDescriptorSetLayout* layout, std::vector<VkDescriptorSetLayoutBinding> bindings)
{
	VkDescriptorSetLayoutCreateInfo info_layout{};
	info_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info_layout.bindingCount = bindings.size();
	info_layout.pBindings = bindings.data();
	
	VkResult r = vkCreateDescriptorSetLayout(get_device(), &info_layout, nullptr, layout);
	VERIFY(r, "Failed to create Vulkan descriptor set layout.");

#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Created Vulkan descriptor set layout." << std::endl;
#endif	
	return true;
}

descriptor::descriptor(uint32_t descriptor_count) : descriptor_count(descriptor_count)
{	
	
}

descriptor::~descriptor()
{
	for(size_t i = 0; i < buffers.size(); i++)
	{
		alloc::free(buffers[i]);
	}

	vkDestroyDescriptorPool(get_device(), descriptor_pool, nullptr);
	vkDestroyDescriptorSetLayout(get_device(), set_layout, nullptr);
}

void descriptor::add_descriptor_binding_buffer(uint32_t size, VkShaderStageFlags shader_stage)
{
	descriptor_binding binding;
	binding.type = DESCRIPTOR_BINDING_TYPE_BUFFER;
	binding.shader_stage = shader_stage;
	binding.size = size;

	for(uint32_t i = 0; i < descriptor_count; i++)
	{
		alloc::buffer buf;
		size_t index = buffers.size();
		buffers.push_back(buf);

		alloc::new_buffer(buffers.data() + index, size, ALLOC_USAGE_UNIFORM_BUFFER);
		if(i == 0) binding.buffer_index = index;
	}

	descriptor_bindings.push_back(binding);
}

void descriptor::add_descriptor_binding_sampler(texture* texture, VkShaderStageFlags shader_stage)
{
	descriptor_binding binding;
	binding.type = DESCRIPTOR_BINDING_TYPE_IMAGE;
	binding.shader_stage = shader_stage;
	binding.texture = texture;

	descriptor_bindings.push_back(binding);
}

bool descriptor::allocate_descriptor_sets()
{
	std::vector<VkDescriptorSetLayout> layouts(descriptor_count, set_layout);
	
	VkDescriptorSetAllocateInfo info_alloc{};
	info_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info_alloc.descriptorPool = descriptor_pool;
	info_alloc.descriptorSetCount = descriptor_count;
	info_alloc.pSetLayouts = layouts.data();
	
	descriptor_sets.resize(descriptor_count);
	VkResult r = vkAllocateDescriptorSets(get_device(), &info_alloc, descriptor_sets.data());
	VERIFY(r, "Failed to allocate descriptor sets.");

	usable = true;
#ifdef DEBUG_PRINT_SUCCESS
	if(usable) std::cout << "[VK|INF] Allocated " << descriptor_count << " descriptor sets." << std::endl;
#endif
	return usable;
}

bool descriptor::build()
{
	auto bindings = create_descriptor_bindings();

	if(!create_descriptor_set_layout(&set_layout, bindings)) return false;
	if(!create_internal_descriptor_pool(bindings)) return false;
	if(!allocate_descriptor_sets()) return false;

	configure_descriptor_sets();
	return true;
}

std::vector<VkDescriptorSetLayoutBinding> descriptor::create_descriptor_bindings()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	for(size_t i = 0; i < descriptor_bindings.size(); i++)
	{
		VkDescriptorType type;
		switch(descriptor_bindings[i].type)
		{
			case DESCRIPTOR_BINDING_TYPE_BUFFER:
				type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				break;
			case DESCRIPTOR_BINDING_TYPE_IMAGE:
				type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				break;
		}
		bindings.push_back(create_descriptor_set_binding(i, type, descriptor_bindings[i].shader_stage));
	}

	return bindings;
}

bool descriptor::create_internal_descriptor_pool(std::vector<VkDescriptorSetLayoutBinding> bindings)
{
	std::vector<VkDescriptorType> types;
	types.resize(descriptor_bindings.size());

	for(size_t i = 0; i < types.size(); i++)
	{
		types[i] = bindings[i].descriptorType;
	}

	return create_descriptor_pool(&descriptor_pool, types, descriptor_count);
}

void descriptor::configure_descriptor_sets()
{
	VkDescriptorBufferInfo info_desc_buffer;
	VkDescriptorImageInfo info_desc_image;

	for(uint32_t i = 0; i < descriptor_count; i++)
	{
		VkWriteDescriptorSet* info_write = new VkWriteDescriptorSet[descriptor_bindings.size()];

		for(uint32_t j = 0; j < descriptor_bindings.size(); j++)
		{
			info_write[j] = {};
			info_write[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			info_write[j].dstSet = descriptor_sets[i];
			info_write[j].dstBinding = j;
			info_write[j].dstArrayElement = 0;

			switch(descriptor_bindings[j].type)
			{
				case DESCRIPTOR_BINDING_TYPE_BUFFER:
					info_write[j].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

					info_desc_buffer = {};
					info_desc_buffer.buffer = buffers[descriptor_bindings[j].buffer_index + i].vk_buffer;
					info_desc_buffer.offset = 0;
					info_desc_buffer.range = descriptor_bindings[j].size;

					info_write[j].pBufferInfo = &info_desc_buffer;
					break;
				case DESCRIPTOR_BINDING_TYPE_IMAGE:
					info_write[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

					info_desc_image = {};
					info_desc_image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					info_desc_image.imageView = descriptor_bindings[j].texture->get_image_view();
					info_desc_image.sampler = descriptor_bindings[j].texture->get_sampler();

					info_write[j].pImageInfo = &info_desc_image;
					break;
			}

			info_write[j].descriptorCount = 1;
		}

		vkUpdateDescriptorSets(get_device(), descriptor_bindings.size(), info_write, 0, nullptr);
	}

#ifdef DEBUG_PRINT_SUCCESS
	std::cout << "[VK|INF] Built pipeline descriptor sets." << std::endl;
#endif
}

VkDescriptorSet descriptor::get_descriptor_set(size_t index) const
{
	return descriptor_sets[index];
}

VkDescriptorSetLayout descriptor::get_descriptor_set_layout() const
{
	return set_layout;
}

uint32_t descriptor::get_descriptor_count() const
{
	return descriptor_count;
}

void descriptor::place_data(uint32_t descriptor_set, uint32_t binding_index, VkDeviceSize offset, VkDeviceSize size, void* data)
{
	size_t buffer_index = descriptor_bindings[binding_index].buffer_index;
	alloc::map_data_to_buffer(data, buffers.data() + buffer_index + descriptor_set, offset, size);
}